# Monitor de Seguridad Alpaca ESP32 para Observatorio

## Manual de Usuario v2.0.0

---

## Indice

1. [Introduccion](#1-introduccion)
2. [Lista de Materiales](#2-lista-de-materiales)
3. [Conexiones de Hardware](#3-conexiones-de-hardware)
4. [Flasheo del Firmware](#4-flasheo-del-firmware)
5. [Primer Arranque y Portal Cautivo](#5-primer-arranque-y-portal-cautivo)
6. [Interfaz Web de Configuracion](#6-interfaz-web-de-configuracion)
7. [Logica de Seguridad](#7-logica-de-seguridad)
8. [Integracion ASCOM Alpaca](#8-integracion-ascom-alpaca)
9. [MQTT - Notificaciones Remotas](#9-mqtt---notificaciones-remotas)
10. [Actualizacion de Firmware por OTA](#10-actualizacion-de-firmware-por-ota)
11. [Respaldo y Restauracion](#11-respaldo-y-restauracion)
12. [Solucion de Problemas](#12-solucion-de-problemas)
13. [Referencia de la API REST](#13-referencia-de-la-api-rest)
14. [Especificaciones Tecnicas](#14-especificaciones-tecnicas)
15. [Glosario](#15-glosario)
16. [Historial de Versiones](#16-historial-de-versiones)

---

## 1. Introduccion

### 1.1 Que es este proyecto

Este proyecto es un **monitor de seguridad para observatorios astronomicos** basado en el microcontrolador ESP32. Implementa el protocolo **ASCOM Alpaca**, lo que permite que cualquier software de astronomia compatible (N.I.N.A., SGPro, ACP, etc.) pueda consultar las condiciones meteorologicas y el estado de seguridad del observatorio a traves de la red WiFi local.

El dispositivo lee sensores ambientales, evalua si las condiciones son seguras para observar y, opcionalmente, controla un rele para abrir o cerrar el techo del observatorio de forma automatica.

No se requiere programacion. El firmware se distribuye como archivos binarios precompilados listos para grabar con una herramienta gratuita de Windows.
El proyecto original fue desarrollado a raíz de la aparición de la especificación de Alpaca, cuyos ejemplos aparecían en Python para un entorno raspberry, y fue pionero en convertir el protocolo para que pudiera ser compilado den C++ de un ESP32. Esa version aun está disponible en:
[GitHub - agnunez/AlpacaSafetyMonitor: Ascom Alpaca driver for a SafetyMonitor coded enterely in a ESP32 using Wifi form any platform](https://github.com/agnunez/AlpacaSafetyMonitor)
Esta nueva versión se distribuye sólo en binarios en el canal @AstroRastro de Telegram ya que las IA usan el codigo fuente de repositorios como el anterior, sin dar ningun tipo de referencia a su autoría ni a sus licencias de codigo abierto. Para cualquier duda o soporte, acudan a dicho canal de Telegram (@agnuca 2026)

### 1.2 Que puede hacer

- **Monitorizar condiciones ambientales**: temperatura, humedad relativa, presion atmosferica, punto de rocio (sensor BME280)
- **Medir temperatura del cielo por infrarrojo**: temperatura del cielo, cobertura nubosa estimada, turbulencia (sensor MLX90614)
- **Evaluar seguridad automaticamente**: logica de 4 estados con histeresis, pesos configurables y analisis de tendencias
- **Controlar rele de techo**: apertura/cierre automatico segun estado de seguridad, modo pin unico o doble
- **Interfaz web completa**: 9 pestanas de configuracion accesibles desde cualquier navegador
- **Portal cautivo WiFi**: configuracion inicial sin necesidad de conocer la IP
- **ASCOM Alpaca**: dispositivos SafetyMonitor y ObservingConditions detectables por UDP
- **MQTT**: notificaciones remotas de cambios de estado y valores de sensores
- **Actualizacion OTA**: actualizar firmware por WiFi sin cable USB
- **Respaldo/restauracion**: exportar e importar toda la configuracion como archivo JSON

### 1.3 Arquitectura general del sistema

```
  ┌─────────────┐     I2C     ┌───────────────┐     WiFi     ┌─────────────────┐
  │   BME280    │────────────>│               │<────────────>│  PC con N.I.N.A │
  │ (temp/hum/  │             │   ESP32       │              │  SGPro, ACP...  │
  │  pres/rocio)│             │               │              └─────────────────┘
  └─────────────┘             │  Firmware     │
                              │  Alpaca v2.0  │     WiFi     ┌─────────────────┐
  ┌─────────────┐     I2C     │               │<────────────>│  Navegador Web  │
  │  MLX90614   │────────────>│               │              │  (configuracion)│
  │ (temp cielo/│             │               │              └─────────────────┘
  │  nubes)     │             │               │
  └─────────────┘             │               │     GPIO     ┌─────────────────┐
                              │               │─────────────>│  Modulo Rele    │
                              │               │              │  (techo)        │
                              └───────────────┘              └─────────────────┘
```

**Flujo de datos interno:**

1. `SensorManager` lee los sensores cada 5 segundos
2. `SafetyEngine` evalua las condiciones y gestiona la maquina de estados
3. `SafetyMonitorDevice` expone el estado por Alpaca y controla el rele
4. `ObservingConditionsDevice` expone los valores meteorologicos por Alpaca
5. Opcionalmente, `MqttClient` publica notificaciones al broker MQTT

### 1.4 Requisitos previos

- **No se necesita programar.** El firmware viene precompilado.
- Conocimientos basicos de electronica: conectar cables con jumpers, identificar pines
- Un **PC con Windows** (7/10/11) para grabar el firmware
- Una **red WiFi de 2.4 GHz** (el ESP32 no soporta 5 GHz)
- Un **navegador web** moderno (Chrome, Firefox, Edge)

---

## 2. Lista de Materiales

### 2.1 Componentes obligatorios

| Componente               | Descripcion                                   | Cantidad | Notas                                                                                                     |
| ------------------------ | --------------------------------------------- | -------- | --------------------------------------------------------------------------------------------------------- |
| ESP32 DOIT DevKit V1     | Placa de desarrollo con WiFi y Bluetooth      | 1        | Tambien compatible con ESP32-WROOM-32 generico. Debe tener chip USB-serial (CP210x o CH340)               |
| Modulo BME280            | Sensor I2C de temperatura, humedad y presion  | 1        | **Importante**: debe ser BME280 (con H de humidity), NO el BMP280 que no mide humedad. Breakout de 3.3V   |
| Modulo MLX90614 (GY-906) | Sensor infrarrojo de temperatura sin contacto | 1        | Para medir la temperatura del cielo. Breakout I2C, 3.3V                                                   |
| Modulo rele 5V           | Rele con optoacoplador                        | 1        | Tipo SRD-05VDC o similar. Preferible con aislamiento optico                                               |
| Cables jumper            | Macho-hembra y macho-macho                    | 10+      | Para conexiones sin soldadura                                                                             |
| Cable micro-USB          | Cable USB con capacidad de datos              | 1        | **ATENCION**: algunos cables solo cargan y no transmiten datos. Verificar que funciona para transferencia |
| Fuente 5V                | Alimentacion USB o transformador              | 1        | Minimo 1A recomendado                                                                                     |

### 2.2 Componentes opcionales

| Componente          | Descripcion                         | Notas                                     |
| ------------------- | ----------------------------------- | ----------------------------------------- |
| LED 3mm o 5mm       | Indicador de estado WiFi            | Cualquier color. Se conecta a GPIO 23     |
| Resistencia 220 ohm | Limitadora de corriente para el LED | 1/4W es suficiente                        |
| Caja estanca IP65   | Proteccion para montaje exterior    | Recomendada si el sensor va en intemperie |
| Prensaestopas       | Pasamuros para cables               | Para sellar la caja estanca               |

### 2.3 Herramientas necesarias

| Herramienta            | Obligatoria | Notas                                              |
| ---------------------- | ----------- | -------------------------------------------------- |
| PC con Windows         | Si          | Para grabar el firmware y acceder a la web         |
| Soldador de estano     | Opcional    | Solo si las placas no traen pines soldados         |
| Multimetro             | Opcional    | Util para verificar conexiones y voltajes          |
| Destornillador pequeno | Opcional    | Si la caja o el rele tienen terminales de tornillo |

### 2.4 Donde comprar (orientativo)

Estos componentes son muy comunes y estan disponibles en:

- **AliExpress / Banggood**: precio mas bajo, envio lento (2-4 semanas)
- **Amazon**: precio medio, envio rapido
- **Tiendas de electronica locales**: disponibilidad inmediata

Buscar por los nombres exactos: "ESP32 DevKit V1", "BME280 I2C module", "GY-906 MLX90614", "5V relay module optocoupler".

---

## 3. Conexiones de Hardware

### 3.1 Pinout del ESP32 DOIT DevKit V1

A continuacion se muestra el pinout simplificado del ESP32 DOIT DevKit V1, resaltando solo los pines utilizados en este proyecto:

```
                    ┌───────────────────┐
                    │    ESP32 DOIT     │
                    │    DevKit V1      │
                    │                   │
                    │  ┌─────────────┐  │
              3V3 ──┤  │             │  ├── VIN (5V)
              GND ──┤  │             │  ├── GND
             D15  ──┤  │             │  ├── D13
              D2  ──┤  │             │  ├── D12
              D4* ──┤  │   ESP32     │  ├── D14
             D16  ──┤  │   WROOM     │  ├── D27
             D17  ──┤  │             │  ├── D26
              D5  ──┤  │             │  ├── D25
             D18  ──┤  │             │  ├── D33
             D19  ──┤  │             │  ├── D32
        SDA D21* ──┤  │             │  ├── D35
              RX  ──┤  │             │  ├── D34
              TX  ──┤  │             │  ├── VN
        SCL D22* ──┤  │             │  ├── VP
        LED D23* ──┤  │             │  ├── EN
                    │  └─────────────┘  │
                    │   [BOOT*]  [EN]   │
                    │      GPIO0        │
                    │                   │
                    │    [  USB  ]      │
                    └───────────────────┘

  * Pines utilizados por este proyecto:
    D4  = GPIO4  -> Rele de techo (open_pin)
    D21 = GPIO21 -> I2C SCL (reloj)
    D22 = GPIO22 -> I2C SDA (datos)
    D23 = GPIO23 -> LED WiFi (opcional)
    BOOT = GPIO0 -> Boton para forzar modo AP (3 segundos)
```

> **Nota para ESP32-WROOM-32 generico**: los pines GPIO son los mismos en todas las variantes ESP32-WROOM-32. La diferencia puede estar en la disposicion fisica de los pines en la placa. Verificar la serigrafía de su placa.

### 3.2 Diagrama de conexion del bus I2C (BME280 + MLX90614)

Ambos sensores comparten el mismo bus I2C ya que tienen direcciones diferentes (0x76 y 0x5A). Las resistencias pull-up normalmente ya estan incluidas en los modulos breakout.

```
    ESP32                     BME280                   MLX90614 (GY-906)
    ─────                     ──────                   ─────────────────

    3V3  ───────────┬──────── VCC (3.3V)         ┌─── VCC (3.3V)
                    └────────────────────────────┘

    GND  ───────────┬──────── GND                ┌─── GND
                    └────────────────────────────┘

    GPIO22 (SDA) ───┬──────── SDA                ┌─── SDA
                    └────────────────────────────┘

    GPIO21 (SCL) ───┬──────── SCL                ┌─── SCL
                    └────────────────────────────┘

                              SDO ──── GND
                              (para direccion 0x76)
```

> **IMPORTANTE**: El pin SDO del BME280 determina su direccion I2C:
> 
> - SDO conectado a **GND** = direccion **0x76** (por defecto en este firmware)
> - SDO conectado a **VCC** = direccion **0x77**
> 
> Si su modulo BME280 usa la direccion 0x77, puede cambiarla en la pestana Hardware de la interfaz web.

### 3.3 Tabla resumen de pines I2C

| Sensor   | Pin del sensor | Pin del ESP32 | GPIO    | Notas                              |
| -------- | -------------- | ------------- | ------- | ---------------------------------- |
| BME280   | VCC            | 3V3           | -       | Solo 3.3V, **nunca conectar a 5V** |
| BME280   | GND            | GND           | -       | Masa comun                         |
| BME280   | SDA            | D22           | GPIO 22 | Bus I2C compartido                 |
| BME280   | SCL            | D21           | GPIO 21 | Bus I2C compartido                 |
| BME280   | SDO            | GND           | -       | Fija direccion a 0x76              |
| MLX90614 | VCC            | 3V3           | -       | Solo 3.3V, **nunca conectar a 5V** |
| MLX90614 | GND            | GND           | -       | Masa comun                         |
| MLX90614 | SDA            | D22           | GPIO 22 | Bus I2C compartido                 |
| MLX90614 | SCL            | D21           | GPIO 21 | Bus I2C compartido                 |

### 3.4 Diagrama de conexion del modulo rele

El modulo rele se alimenta con 5V (desde el pin VIN del ESP32) pero acepta senales de control de 3.3V gracias al optoacoplador integrado.

```
    ESP32                   Modulo Rele 5V
    ─────                   ──────────────

    GPIO4  ─────────────── IN  (entrada de control)

    VIN (5V) ──────────────  VCC (alimentacion del rele)

    GND  ──────────────────  GND


    Contactos del rele (lado de alta potencia):

                   ┌─── COM ─────── Cable al motor/controlador del techo
                   │
    Modulo Rele ───┤─── NO  ─────── Normalmente Abierto
                   │                (se cierra cuando el rele se activa)
                   │
                   └─── NC  ─────── Normalmente Cerrado
                                    (se abre cuando el rele se activa)
```

**Funcionamiento con la configuracion por defecto (Active HIGH):**

- GPIO4 en **HIGH** (3.3V) = rele **activado** = techo se abre
- GPIO4 en **LOW** (0V) = rele **desactivado** = techo se cierra

**Conexion recomendada:**

- Conectar los cables del motor/controlador del techo a los terminales **COM** y **NO** (Normalmente Abierto)
- Asi, cuando el rele se active (condiciones seguras), el circuito se cerrara y el techo se abrira

> **ADVERTENCIA DE SEGURIDAD**: El rele controla un circuito de potencia que puede accionar motores. Asegurese de que el circuito del motor del techo tiene sus propios fusibles y protecciones. Este rele es una senal de mando, no debe manejar corrientes elevadas directamente.

### 3.5 Tabla resumen de pines del rele

| Pin del rele | Pin del ESP32 | GPIO   | Notas                                        |
| ------------ | ------------- | ------ | -------------------------------------------- |
| IN           | D4            | GPIO 4 | Senal de control (3.3V logica)               |
| VCC          | VIN           | -      | Alimentacion 5V para la bobina del rele      |
| GND          | GND           | -      | Masa comun                                   |
| COM          | -             | -      | Terminal comun del rele (al motor)           |
| NO           | -             | -      | Normalmente Abierto (contacto cuando activo) |
| NC           | -             | -      | Normalmente Cerrado (contacto en reposo)     |

### 3.6 Diagrama del LED WiFi (opcional)

Un LED indicador muestra el estado de la conexion WiFi:

```
    ESP32                         LED
    ─────                         ───

    GPIO23 ──── Resistencia 220Ω ──── Anodo (+) LED ──── Catodo (-) ──── GND
                                      (pata larga)       (pata corta)
```

**Patrones del LED:**

| Estado WiFi              | Comportamiento del LED   |
| ------------------------ | ------------------------ |
| Conectado a WiFi         | Encendido fijo           |
| Modo AP (portal cautivo) | Parpadeo rapido (200 ms) |
| Conectando               | Parpadeo lento (500 ms)  |
| Desconectado             | Apagado                  |

### 3.7 Tabla resumen completa de pines GPIO

| GPIO | Funcion             | Direccion     | Nivel activo  | Configurable          | Notas                         |
| ---- | ------------------- | ------------- | ------------- | --------------------- | ----------------------------- |
| 22   | I2C SDA (datos)     | Bidireccional | -             | Si (pestana Hardware) | Bus compartido entre sensores |
| 21   | I2C SCL (reloj)     | Bidireccional | -             | Si (pestana Hardware) | Bus compartido entre sensores |
| 4    | Rele apertura techo | Salida        | HIGH          | Si (pestana Roof)     | Pin por defecto               |
| -1   | Rele cierre techo   | Salida        | HIGH          | Si (pestana Roof)     | -1 = modo toggle unico        |
| 23   | LED indicador WiFi  | Salida        | HIGH          | Si (pestana Hardware) | Opcional                      |
| 0    | Boton BOOT          | Entrada       | LOW (pulsado) | No (fijo)             | 3 seg = forzar modo AP        |

### 3.8 Notas importantes sobre alimentacion y voltajes

1. **Sensores a 3.3V**: Los sensores BME280 y MLX90614 deben alimentarse con 3.3V (pin 3V3 del ESP32). **Nunca conectar a 5V** ya que se pueden danar.

2. **Rele a 5V**: El modulo rele necesita 5V para su bobina, que se obtiene del pin VIN del ESP32 (conectado directamente al USB 5V). El pin de control (IN) acepta 3.3V gracias al optoacoplador.

3. **Resistencias pull-up I2C**: Normalmente los modulos breakout del BME280 y MLX90614 ya incluyen resistencias pull-up de 4.7K o 10K en las lineas SDA y SCL. **No es necesario agregar resistencias adicionales** salvo que use sensores sin placa breakout.

4. **Alimentacion**: Para funcionamiento estable, utilice una fuente USB de al menos 1A. Si el rele consume mucha corriente, considere alimentar el rele con una fuente independiente (compartiendo solo GND con el ESP32).

5. **Longitud de cables I2C**: Si necesita cables largos (mas de 50 cm) hasta los sensores, considere usar cable apantallado y reducir la velocidad del bus I2C. Cables de mas de 1 metro pueden causar errores de comunicacion.

### 3.9 Modo dual-pin para techo

Por defecto, el sistema usa un solo pin GPIO (modo single-toggle). Si su controlador de techo necesita dos senales separadas (una para abrir y otra para cerrar), configure:

- **Open Pin**: GPIO para la senal de apertura (ej: 4)
- **Close Pin**: GPIO diferente para la senal de cierre (ej: 16)

Cuando ambos pines son diferentes y el close_pin es distinto de -1:

- Para **abrir**: se desactiva close_pin, luego se activa open_pin
- Para **cerrar**: se desactiva open_pin, luego se activa close_pin

Esta configuracion se realiza en la pestana **Roof** de la interfaz web (ver seccion 6.7).

---

## 4. Flasheo del Firmware

### 4.1 Archivos necesarios

Para grabar el firmware necesita estos 5 archivos binarios (incluidos en la distribucion):

| Archivo          | Descripcion                                 | Tamano aprox. |
| ---------------- | ------------------------------------------- | ------------- |
| `bootloader.bin` | Cargador de arranque del ESP32              | ~17 KB        |
| `partitions.bin` | Tabla de particiones de la flash            | ~3 KB         |
| `boot_app0.bin`  | Selector de particion OTA                   | ~8 KB         |
| `firmware.bin`   | Firmware principal del monitor de seguridad | ~1 MB         |
| `spiffs.bin`     | Interfaz web (HTML, JavaScript, CSS)        | variable      |

### 4.2 Instalacion del driver USB

El ESP32 DevKit usa un chip conversor USB-serial. Dependiendo de su placa, sera uno de estos dos:

**Chip CP210x (Silicon Labs)** -- el mas comun en DOIT DevKit V1:

1. Descargar el driver de: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
2. Ejecutar el instalador y reiniciar si es necesario

**Chip CH340 (WCH)** -- comun en placas genericas:

1. Descargar el driver de: https://www.wch-ic.com/downloads/CH341SER_EXE.html
2. Ejecutar el instalador

**Verificar la instalacion:**

1. Conectar el ESP32 al PC con el cable USB
2. Abrir el **Administrador de dispositivos** de Windows (clic derecho en Inicio > Administrador de dispositivos)
3. Expandir **Puertos (COM y LPT)**
4. Debe aparecer un puerto como "Silicon Labs CP210x USB to UART Bridge (COM3)" o "USB-SERIAL CH340 (COM6)"
5. **Anotar el numero de puerto COM** (lo necesitara para el flasheo)

> Si no aparece ningun puerto, pruebe con otro cable USB (asegurese de que sea un cable de datos, no solo de carga).

### 4.3 Descarga e instalacion del Flash Download Tool

1. Ir a la pagina oficial de Espressif: https://www.espressif.com/en/support/download/other-tools
2. Descargar **Flash Download Tools (Windows)**
3. Descomprimir el archivo ZIP en cualquier carpeta
4. Ejecutar `flash_download_tool_X.X.X.exe` (no requiere instalacion)

### 4.4 Configuracion del Flash Download Tool

Al abrir la herramienta:

1. **ChipType**: seleccionar **ESP32**
2. **WorkMode**: seleccionar **Develop**
3. Click en **OK**

En la ventana principal, configurar:

| Parametro  | Valor                                         |
| ---------- | --------------------------------------------- |
| SPI SPEED  | 40 MHz                                        |
| SPI MODE   | DIO                                           |
| FLASH SIZE | 32Mbit (4 MB)                                 |
| COM        | El puerto que anoto en el paso 4.2 (ej: COM6) |
| BAUD       | 921600                                        |

### 4.5 Tabla de archivos y direcciones de flasheo

Esta tabla es **critica**. Cada archivo debe grabarse en la direccion exacta indicada. Un error en las direcciones dejara el ESP32 inoperativo.

|     | Archivo          | Direccion (hex) | Notas                 |
| --- | ---------------- | --------------- | --------------------- |
| 1   | `bootloader.bin` | `0x1000`        | Cargador de arranque  |
| 2   | `partitions.bin` | `0x8000`        | Tabla de particiones  |
| 3   | `boot_app0.bin`  | `0xE000`        | Selector OTA          |
| 4   | `firmware.bin`   | `0x10000`       | Firmware principal    |
| 5   | `spiffs.bin`     | `0x310000`      | Interfaz web (SPIFFS) |

### 4.6 Proceso de flasheo paso a paso

1. En la ventana del Flash Download Tool, hay varias filas con casillas de verificacion. Para cada archivo:
   
   - Marcar la **casilla** de la fila
   - Hacer clic en **"..."** para seleccionar el archivo
   - Escribir la **direccion** en el campo de la derecha

2. Configurar las 5 filas segun la tabla de la seccion 4.5

3. Verificar que:
   
   - El **puerto COM** es correcto
   - La **velocidad** es 921600
   - **SPI MODE** es DIO
   - **FLASH SIZE** es 32Mbit

4. Conectar el ESP32 al PC con el cable USB

5. Pulsar el boton **START**

6. **Si el programa no detecta el ESP32**: mantener pulsado el boton **BOOT** del ESP32 mientras pulsa START, y soltar BOOT cuando aparezca "Connecting..." en la consola

7. Esperar a que el proceso termine. Aparecera el mensaje **"FINISH"** con fondo verde

8. El ESP32 se reiniciara automaticamente

> **Tiempo estimado**: el flasheo completo tarda entre 30 y 60 segundos a 921600 baudios.

### 4.7 Verificacion del flasheo exitoso

Para verificar que el firmware se grabo correctamente:

1. Abrir un programa de terminal serial:
   
   - **PuTTY** (gratuito): https://www.putty.org/
   - **Tera Term** (gratuito): https://teratermproject.github.io/
   - O cualquier otro terminal serial

2. Configurar:
   
   - Puerto: el COM del ESP32
   - Velocidad: **115200** baudios
   - Datos: 8 bits, sin paridad, 1 bit de parada (8-N-1)

3. Pulsar el boton **EN** (reset) del ESP32

4. Debe aparecer en la consola:

```
# AlpacaESP32 Safety Monitor v2.0.0
# SettingsManager: No config file, using defaults
# WiFiManager: No stored credentials, starting AP mode
# WiFiManager: AP started, SSID: 'alpaca_esp32', IP: 192.168.4.1
```

Si ve este mensaje, el firmware esta correctamente instalado. Proceda a la seccion 5 para la configuracion inicial.

### 4.8 Solucion de problemas de flasheo

| Problema                         | Causa probable                         | Solucion                                                                         |
| -------------------------------- | -------------------------------------- | -------------------------------------------------------------------------------- |
| "Failed to connect to ESP32"     | El ESP32 no entra en modo bootloader   | Mantener pulsado BOOT al iniciar el flasheo                                      |
| No aparece puerto COM            | Driver no instalado o cable incorrecto | Instalar driver (seccion 4.2), probar otro cable USB                             |
| Flasheo muy lento                | Velocidad baja configurada             | Subir a 921600 baudios                                                           |
| "Invalid head of packet"         | Cable USB defectuoso                   | Probar con otro cable                                                            |
| El ESP32 no arranca tras flasheo | Direcciones incorrectas                | Verificar las 5 direcciones de la tabla 4.5                                      |
| "A fatal error occurred"         | Flash protegida o danada               | Probar: `Erase` primero (boton ERASE en la herramienta), luego flashear de nuevo |

---

## 5. Primer Arranque y Portal Cautivo

### 5.1 Que sucede en el primer arranque

Cuando el ESP32 arranca por primera vez con el firmware nuevo:

1. El sistema detecta que **no hay credenciales WiFi almacenadas**
2. Automaticamente inicia en **modo AP (Access Point)**, creando una red WiFi propia
3. La red WiFi se llama **`alpaca_esp32`** (nombre configurable despues)
4. No tiene contrasena (red abierta)
5. La IP del ESP32 en esta red es **192.168.4.1**
6. Se activa un **portal cautivo** con servidor DNS que redirige todas las peticiones a la pagina de configuracion

### 5.2 Conexion al punto de acceso (AP)

1. En su telefono movil, tablet o portatil, abra la **lista de redes WiFi disponibles**
2. Busque la red **`alpaca_esp32`**
3. Conectese a ella (no se necesita contrasena)
4. Es posible que su dispositivo muestre un aviso de "Red sin Internet" -- esto es normal
5. En muchos dispositivos, el portal cautivo se abrira automaticamente en el navegador
6. Si no se abre automaticamente, abra manualmente el navegador y vaya a: **http://192.168.4.1/setup**

### 5.3 Configuracion WiFi inicial

Una vez en la interfaz web:

1. Haga clic en la pestana **WiFi**
2. Pulse el boton **Scan** para buscar redes WiFi cercanas
3. Espere unos segundos mientras el ESP32 escanea (la tabla se llenara con las redes encontradas)
4. Haga clic en **Select** junto a su red WiFi domestica
5. El nombre de la red (SSID) se rellenara automaticamente en el campo
6. Escriba la **contrasena** de su red WiFi
7. Pulse **Connect & Save**
8. El sistema guardara las credenciales y se conectara a su red (esto tarda unos segundos)

> **Nota**: Las credenciales WiFi se guardan en memoria no volatil (NVS) del ESP32. Sobreviven a reinicios y actualizaciones de firmware.

### 5.4 Verificacion de la conexion

Una vez conectado a su red WiFi domestica, el ESP32 obtendra una IP de su router. Para conocer la IP:

**Opcion A - Monitor serial:**
Si tiene un terminal serial conectado (seccion 4.7), vera:

```
# WiFiManager: Connected to 'SuRedWiFi', IP: 192.168.1.xxx
# WiFiManager: mDNS started as 'alpaca_esp32.local'
```

**Opcion B - Tabla DHCP del router:**
Acceda a la configuracion de su router y busque el dispositivo "alpaca_esp32" en la lista de clientes DHCP.

**Opcion C - mDNS:**
Si su PC soporta mDNS (Windows 10/11 lo soportan con Bonjour o el servicio nativo), puede acceder directamente a:

```
http://alpaca_esp32.local/setup
```

### 5.5 Acceso a la interfaz web

Abra su navegador y vaya a:

```
http://<IP_del_ESP32>/setup
```

Por ejemplo: `http://192.168.1.167/setup`

O via mDNS: `http://alpaca_esp32.local/setup`

Vera la interfaz de configuracion con 9 pestanas. La seccion 6 describe cada pestana en detalle.

### 5.6 Uso del boton BOOT para forzar modo AP

Si necesita reconfigurar el WiFi en el futuro (por ejemplo, si cambio la contrasena de su router o mudo el equipo a otra red):

1. **Mantenga pulsado el boton BOOT** del ESP32 (marcado como "BOOT" en la placa, es el boton GPIO 0)
2. Mantengalo pulsado durante **3 segundos**
3. El ESP32 entrara en modo AP independientemente de su estado actual
4. En el monitor serial vera: `# WiFiManager: Boot button long press - starting AP mode`
5. Conectese a la red `alpaca_esp32` y reconfigure el WiFi

> **Nota**: Esta accion no borra las credenciales almacenadas. Simplemente fuerza el modo AP para poder acceder a la configuracion.

---

## 6. Interfaz Web de Configuracion

### 6.1 Vista general de la interfaz

La interfaz web tiene un diseno moderno con fondo azul-gris oscuro y un contenedor central blanco. Utiliza Bootstrap para la maquetacion y es accesible desde cualquier navegador moderno, incluyendo dispositivos moviles.

La interfaz consta de **9 pestanas** en la parte superior:

```
[ Status | WiFi | Devices | Hardware | Safety | Roof | MQTT | OTA | Backup ]
```

**Caracteristicas:**

- Los datos se cargan de forma **diferida** al hacer clic en cada pestana (no todo a la vez)
- Cada pestana tiene un boton **Refresh** para recargar los datos y un boton **Save** para guardar cambios
- Los mensajes de exito o error aparecen en alertas coloreadas debajo de los formularios
- Los cambios guardados se aplican inmediatamente (excepto cambios de pines I2C que requieren reinicio)

### 6.2 Pestana Status (Estado)

La pestana Status muestra el estado actual del sistema en tiempo real.

#### Tabla de Estado de Sensores

Muestra cada sensor con su estado de salud y valor actual:

| Columna | Descripcion                            |
| ------- | -------------------------------------- |
| Sensor  | Nombre del sensor (BME280, MLX90614)   |
| Estado  | Indicador de salud con codigo de color |
| Valor   | Ultimo valor leido                     |

**Codigos de color del estado:**

| Badge               | Significado | Descripcion                        |
| ------------------- | ----------- | ---------------------------------- |
| OK (verde)          | Funcionando | El sensor responde correctamente   |
| DEGRADED (amarillo) | Degradado   | 1-4 fallos consecutivos de lectura |
| FAILED (rojo)       | Fallido     | 5 o mas fallos consecutivos        |

Cuando un sensor esta en estado FAILED, el sistema intenta reconectarlo automaticamente cada 10 segundos.

#### Tabla de Estado de Seguridad

Muestra el resultado de la evaluacion de seguridad:

| Campo                 | Descripcion                                                                 |
| --------------------- | --------------------------------------------------------------------------- |
| Estado global         | Badge SAFE (verde) o UNSAFE (rojo)                                          |
| Maquina de estados    | Estado actual: SAFE, UNSAFE, TRANSITIONING_TO_SAFE, TRANSITIONING_TO_UNSAFE |
| Metricas individuales | Estado seguro/inseguro de cada metrica (temperatura, cielo, humedad, rocio) |

**Boton Refresh**: recarga todos los valores del servidor.

### 6.3 Pestana WiFi

Gestiona la conexion WiFi del dispositivo.

#### Seccion Estado WiFi

| Campo    | Descripcion                                              |
| -------- | -------------------------------------------------------- |
| Estado   | connected, ap_mode, disconnected, connecting             |
| SSID     | Nombre de la red conectada actualmente                   |
| IP       | Direccion IP actual (de la red o 192.168.4.1 en modo AP) |
| RSSI     | Intensidad de senal en dBm (mas cercano a 0 = mejor)     |
| Hostname | Nombre mDNS del dispositivo                              |

#### Seccion Redes Almacenadas

Muestra hasta **3 redes WiFi** guardadas en memoria. Para cada red:

| Campo     | Descripcion                              |
| --------- | ---------------------------------------- |
| SSID      | Nombre de la red                         |
| Prioridad | Orden de conexion (0 = maxima prioridad) |
| Accion    | Boton para eliminar la red almacenada    |

En cada arranque, el ESP32 intenta conectarse a las redes almacenadas en orden de prioridad. Si la primera falla, prueba la siguiente, y asi sucesivamente.

#### Seccion Redes Disponibles

- Pulse **Scan** para buscar redes WiFi cercanas
- El escaneo es asincrono y tarda unos 3-5 segundos
- Para cada red encontrada se muestra:

| Campo           | Descripcion                       |
| --------------- | --------------------------------- |
| SSID            | Nombre de la red                  |
| RSSI            | Intensidad de senal               |
| Abierta/Cifrada | Si requiere contrasena            |
| Select          | Boton para rellenar el campo SSID |

#### Formulario de Conexion

| Campo    | Tipo       | Descripcion                                    |
| -------- | ---------- | ---------------------------------------------- |
| SSID     | Texto      | Nombre de la red (se rellena al pulsar Select) |
| Password | Contrasena | Contrasena de la red WiFi                      |

**Botones:**

- **Connect & Save**: guarda la red y se conecta (la conexion tarda ~2 segundos)
- **Start AP Mode**: fuerza el modo portal cautivo

> **Nota**: La conexion es diferida: el boton responde inmediatamente y la conexion real se ejecuta 1.5 segundos despues. Esto permite que la respuesta HTTP llegue al navegador antes de que cambie la red.

### 6.4 Pestana Devices (Dispositivos ASCOM)

Esta pestana muestra la configuracion de los dispositivos ASCOM Alpaca. Aparecen sub-pestanas para cada dispositivo registrado:

- **SafetyMonitor** (dispositivo 0)
- **ObservingConditions** (dispositivo 0)

Cada dispositivo muestra un formulario con sus parametros de configuracion ASCOM, generado dinamicamente por la libreria jsonFormer.

**Botones:**

- **Refresh**: recarga la configuracion desde el ESP32
- **Update**: aplica los cambios **solo en RAM** (se pierden al reiniciar)
- **Save**: guarda los cambios en la **memoria flash** (persisten tras reinicio)

> **Nota**: Normalmente no necesita cambiar nada en esta pestana salvo que sea un usuario avanzado que quiera ajustar los nombres o la descripcion de los dispositivos ASCOM.

### 6.5 Pestana Hardware

Configura los pines de hardware y parametros del bus I2C.

| Campo                 | ID          | Tipo        | Valor por defecto | Descripcion                                                      |
| --------------------- | ----------- | ----------- | ----------------- | ---------------------------------------------------------------- |
| Hostname              | hw-hostname | Texto       | `alpaca_esp32`    | Nombre del dispositivo para mDNS y para el SSID del modo AP      |
| I2C SDA Pin           | hw-sda      | Numero      | 22                | Pin GPIO para la linea de datos I2C                              |
| I2C SCL Pin           | hw-scl      | Numero      | 21                | Pin GPIO para la linea de reloj I2C                              |
| BME280 Address        | hw-bme-addr | Hexadecimal | 0x76              | Direccion I2C del sensor BME280                                  |
| MLX90614 Address      | hw-mlx-addr | Hexadecimal | 0x5A              | Direccion I2C del sensor MLX90614                                |
| WiFi LED Pin          | hw-led      | Numero      | 23                | Pin GPIO del LED indicador WiFi. Use -1 para desactivar          |
| Measure Interval (ms) | hw-interval | Numero      | 5000              | Intervalo de lectura de sensores en milisegundos. Minimo 3000 ms |

> **ATENCION**: Los cambios en los pines I2C (SDA, SCL) requieren **reiniciar** el ESP32 para tomar efecto. El resto de cambios se aplican inmediatamente.

**Botones:**

- **Refresh**: recarga la configuracion actual
- **Save**: guarda los cambios en flash y los aplica

### 6.6 Pestana Safety (Seguridad)

Esta es la pestana mas importante. Configura los umbrales que determinan si las condiciones son seguras para observar.

#### Metricas de seguridad

Cada metrica tiene 4 parametros configurables:

| Parametro         | Descripcion                                          |
| ----------------- | ---------------------------------------------------- |
| Enabled           | Casilla para activar/desactivar esta metrica         |
| Arm (seguro)      | Umbral para pasar a estado seguro                    |
| Disarm (inseguro) | Umbral para pasar a estado inseguro                  |
| Weight (peso)     | Peso relativo en el calculo de seguridad (0.0 a 1.0) |

La diferencia entre Arm y Disarm es la **histeresis**: evita que el sistema oscile rapidamente entre seguro e inseguro cuando los valores estan cerca del limite. Ver seccion 7 para una explicacion detallada.

#### Metrica: Temperatura ambiente

| Parametro | Valor por defecto | Logica                                                              |
| --------- | ----------------- | ------------------------------------------------------------------- |
| Arm       | 1.0 C             | Seguro cuando la temperatura ambiente es **mayor** que este valor   |
| Disarm    | 0.0 C             | Inseguro cuando la temperatura ambiente es **menor** que este valor |
| Weight    | 1.0               | Peso completo en el calculo                                         |

*Ejemplo: Con los valores por defecto, si la temperatura baja de 0 C se activa la alarma. No se desactiva hasta que supere 1 C.*

#### Metrica: Temperatura del cielo

| Parametro | Valor por defecto | Logica                                                                               |
| --------- | ----------------- | ------------------------------------------------------------------------------------ |
| Arm       | -16.0 C           | Seguro cuando la temperatura del cielo es **menor** que este valor (cielo despejado) |
| Disarm    | -15.0 C           | Inseguro cuando la temperatura del cielo es **mayor** que este valor (nubes)         |
| Weight    | 1.0               | Peso completo en el calculo                                                          |

*Un cielo despejado tiene una temperatura infrarroja muy baja (< -20 C). Las nubes irradian calor y la temperatura sube.*

#### Metrica: Humedad relativa

| Parametro | Valor por defecto | Logica                                                 |
| --------- | ----------------- | ------------------------------------------------------ |
| Arm       | 85.0 %            | Seguro cuando la humedad es **menor** que este valor   |
| Disarm    | 90.0 %            | Inseguro cuando la humedad es **mayor** que este valor |
| Weight    | 1.0               | Peso completo en el calculo                            |

*Humedad alta puede causar condensacion en la optica.*

#### Metrica: Margen de punto de rocio

| Parametro | Valor por defecto | Logica                                                                     |
| --------- | ----------------- | -------------------------------------------------------------------------- |
| Arm       | 6.0 C             | Seguro cuando (temperatura - punto de rocio) es **mayor** que este valor   |
| Disarm    | 5.0 C             | Inseguro cuando (temperatura - punto de rocio) es **menor** que este valor |
| Weight    | 1.0               | Peso completo en el calculo                                                |

*El punto de rocio es la temperatura a la que el aire se satura de humedad. Si la temperatura ambiente se acerca al punto de rocio (margen pequeno), hay riesgo de condensacion.*

#### Parametros globales de seguridad

| Campo              | ID              | Valor por defecto | Descripcion                                                                 |
| ------------------ | --------------- | ----------------- | --------------------------------------------------------------------------- |
| Delay to Open (s)  | sf-delay-open   | 1200 (20 min)     | Tiempo que las condiciones deben permanecer seguras antes de abrir el techo |
| Delay to Close (s) | sf-delay-close  | 120 (2 min)       | Tiempo antes de cerrar el techo tras detectar condiciones inseguras         |
| Trend Window (s)   | sf-trend-window | 300 (5 min)       | Ventana de tiempo para el analisis de tendencias                            |
| Trend Threshold    | sf-trend-thresh | 5.0               | Tasa de cambio (unidades/min) que fuerza estado inseguro                    |

> **Consejo**: El delay_to_open largo (20 minutos) es conservador a proposito. Garantiza que las condiciones son realmente estables antes de abrir. Puede reducirlo si su zona tiene clima estable, pero no se recomienda bajarlo de 300 segundos (5 minutos).

**Botones:**

- **Refresh**: recarga los umbrales actuales
- **Save**: guarda los cambios en flash y los aplica inmediatamente

### 6.7 Pestana Roof (Techo)

Configura el control del rele para la apertura/cierre del techo.

| Campo       | ID               | Tipo    | Valor por defecto | Descripcion                                                                                      |
| ----------- | ---------------- | ------- | ----------------- | ------------------------------------------------------------------------------------------------ |
| Open Pin    | roof-open        | Numero  | 4                 | Pin GPIO para el rele de apertura                                                                |
| Close Pin   | roof-close       | Numero  | -1                | Pin GPIO para el rele de cierre. **-1 = modo pin unico**                                         |
| Active High | roof-active-high | Casilla | Marcada (true)    | Si esta marcado, el rele se activa con nivel HIGH (3.3V). Si se desmarca, se activa con LOW (0V) |

**Modos de funcionamiento:**

| Modo               | Configuracion                          | Descripcion                                                                                             |
| ------------------ | -------------------------------------- | ------------------------------------------------------------------------------------------------------- |
| Pin unico (toggle) | close_pin = -1                         | Un solo GPIO controla apertura y cierre. HIGH = abrir, LOW = cerrar (o al reves si active_high = false) |
| Doble pin          | close_pin >= 0 y diferente de open_pin | Dos GPIOs separados. Uno activa apertura, otro activa cierre                                            |

> **Nota**: Al arrancar, el sistema siempre inicia con el techo en estado **cerrado** (rele desactivado). El techo solo se abrira cuando la maquina de seguridad alcance el estado SAFE.

**Botones:**

- **Refresh**: recarga la configuracion actual
- **Save**: guarda los cambios en flash

### 6.8 Pestana MQTT

Configura las notificaciones remotas via protocolo MQTT. Esta funcionalidad es **completamente opcional**.

| Campo                 | ID            | Tipo       | Valor por defecto    | Descripcion                                   |
| --------------------- | ------------- | ---------- | -------------------- | --------------------------------------------- |
| Enable MQTT           | mqtt-enabled  | Casilla    | Desmarcada (off)     | Activa/desactiva el cliente MQTT              |
| Broker Host           | mqtt-broker   | Texto      | (vacio)              | Direccion IP o nombre del broker MQTT         |
| Port                  | mqtt-port     | Numero     | 1883                 | Puerto del broker MQTT                        |
| Username              | mqtt-user     | Texto      | (vacio)              | Usuario para autenticacion (opcional)         |
| Password              | mqtt-pass     | Contrasena | (vacio)              | Contrasena (opcional)                         |
| Topic Prefix          | mqtt-topic    | Texto      | `observatory/safety` | Prefijo para los topics MQTT                  |
| Publish Interval (ms) | mqtt-interval | Numero     | 30000                | Intervalo de publicacion de datos de sensores |

**Topics publicados:**

| Topic                              | Cuando se publica                  | Contenido                                      |
| ---------------------------------- | ---------------------------------- | ---------------------------------------------- |
| `<prefix>/state`                   | Al cambiar el estado de seguridad  | `{"isSafe":true,"reason":"..."}`               |
| `<prefix>/sensors`                 | Cada publish_interval milisegundos | JSON con valores y salud de todos los sensores |
| `<prefix>/sensors/<nombre>/health` | Al cambiar la salud de un sensor   | `{"failed":true}` o `{"failed":false}`         |

**Botones:**

- **Refresh**: recarga la configuracion actual
- **Save**: guarda los cambios. Si se activa MQTT, la conexion se intentara inmediatamente

### 6.9 Pestana OTA (Actualizacion de Firmware)

Permite actualizar el firmware por WiFi sin necesidad de conectar el cable USB.

**Elementos de la pestana:**

| Elemento             | Descripcion                                              |
| -------------------- | -------------------------------------------------------- |
| Version actual       | Muestra la version del firmware instalado (ej: "v2.0.0") |
| Selector de archivo  | Para elegir el archivo .bin del nuevo firmware           |
| Barra de progreso    | Muestra el porcentaje de subida                          |
| Boton Upload & Flash | Inicia la actualizacion (boton rojo de advertencia)      |

**Procedimiento:**

1. Verificar la version actual
2. Seleccionar el archivo `firmware.bin` (solo firmware, NO el spiffs.bin)
3. Pulsar **Upload & Flash**
4. Esperar a que la barra de progreso llegue al 100%
5. El ESP32 se reiniciara automaticamente
6. La pagina se recargara tras unos 10 segundos

> **ADVERTENCIA**:
> 
> - Solo suba archivos `firmware.bin`. **No suba `spiffs.bin` por OTA** (la interfaz web debe actualizarse por cable serial).
> - No desconecte la alimentacion durante la actualizacion.
> - El sistema tiene doble particion OTA: si la nueva version falla, el firmware anterior permanece disponible como respaldo.

### 6.10 Pestana Backup (Respaldo)

Permite exportar e importar toda la configuracion del dispositivo.

#### Exportar configuracion

- Pulse **Export Settings**
- Se descargara un archivo `alpaca_settings.json` con toda la configuracion:
  - Configuracion de hardware (pines, direcciones, hostname)
  - Umbrales de seguridad (todas las metricas)
  - Configuracion del techo (pines, modo)
  - Configuracion MQTT (broker, credenciales)
  - Credenciales WiFi almacenadas (incluyendo contrasenas)

> **Recomendacion**: Exporte un respaldo **antes de cada actualizacion OTA** y cada vez que cambie configuraciones importantes.

#### Importar configuracion

- Seleccione un archivo `.json` previamente exportado
- Pulse **Import Settings**
- El sistema validara el archivo y aplicara la configuracion
- Las credenciales WiFi se restauraran automaticamente

> **Nota**: El archivo JSON puede editarse manualmente con un editor de texto antes de importarlo. Esto es util para preparar configuraciones para multiples dispositivos.

---

## 7. Logica de Seguridad

### 7.1 Maquina de estados (4 estados)

El motor de seguridad utiliza una maquina de 4 estados para tomar decisiones sobre la apertura y cierre del techo:

| Estado                      | Nombre completo           | Significado                                                 |
| --------------------------- | ------------------------- | ----------------------------------------------------------- |
| **SAFE**                    | Seguro                    | Las condiciones son buenas. El techo esta abierto           |
| **TRANSITIONING_TO_UNSAFE** | Transicionando a inseguro | Las condiciones empeoraron. Contando tiempo antes de cerrar |
| **UNSAFE**                  | Inseguro                  | Las condiciones son malas. El techo esta cerrado            |
| **TRANSITIONING_TO_SAFE**   | Transicionando a seguro   | Las condiciones mejoraron. Contando tiempo antes de abrir   |

### 7.2 Histeresis: umbrales arm y disarm

La histeresis evita que el sistema oscile rapidamente entre SAFE y UNSAFE cuando un valor meteorologico esta cerca del limite.

**Ejemplo con humedad (arm=85%, disarm=90%):**

```
                                        disarm=90%
                                            |
    100% ─────────────────────────────────── ┤
                                            |
     90% ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤ ← Se activa alarma (INSEGURO)
                                            |
                    ZONA DE                 |
                    HISTERESIS              |
                    (no cambia estado)      |
                                            |
     85% ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤ ← Se desactiva alarma (SEGURO)
                                            |         arm=85%
     80% ─────────────────────────────────── ┤
```

- Si la humedad **sube por encima de 90%**: la metrica se marca como **insegura**
- Si la humedad **baja por debajo de 85%**: la metrica se marca como **segura**
- Si la humedad esta **entre 85% y 90%**: **no cambia** el estado actual

Esto evita que el sistema abra y cierre el techo repetidamente cuando la humedad oscila alrededor de un valor limite.

### 7.3 Puntuacion ponderada (weighted score)

Cada metrica contribuye proporcionalmente al calculo de seguridad global:

```
puntuacion = suma(peso_i * estado_seguro_i) / suma(peso_i)
```

Donde:

- `peso_i` es el peso configurado de cada metrica (0.0 a 1.0)
- `estado_seguro_i` es 1.0 si la metrica esta segura, 0.0 si no

**Ejemplo con 4 metricas (todos los pesos = 1.0):**

| Metrica     | Estado   | Peso    | Contribucion |
| ----------- | -------- | ------- | ------------ |
| Temperatura | Segura   | 1.0     | 1.0          |
| Cielo       | Insegura | 1.0     | 0.0          |
| Humedad     | Segura   | 1.0     | 1.0          |
| Rocio       | Segura   | 1.0     | 1.0          |
| **Total**   |          | **4.0** | **3.0**      |

Puntuacion = 3.0 / 4.0 = **0.75** → Mayor que 0.5 → **Seguro**

Si la puntuacion es mayor que 0.5 Y no hay tendencias peligrosas (seccion 7.4), el veredicto instantaneo es "seguro".

### 7.4 Analisis de tendencias (trend analysis)

Ademas de los umbrales estaticos, el sistema analiza la **velocidad de cambio** de los valores. Esto permite detectar situaciones como nubes que se acercan rapidamente.

- Se almacenan 12 muestras en un buffer circular durante una ventana de 300 segundos (5 minutos)
- Se calcula la tasa de cambio en unidades por minuto
- Si la **humedad sube** o la **temperatura del cielo sube** a mas de 5 unidades/minuto, se **fuerza el estado INSEGURO** independientemente de los umbrales individuales

**Ejemplo:** Si la humedad pasa de 60% a 85% en 3 minutos, la tasa es 8.3%/min, que supera el umbral de 5.0, forzando el cierre inmediato.

### 7.5 Degradacion elegante (graceful degradation)

Si un sensor falla:

- Su metrica se **excluye** del calculo ponderado
- Los sensores restantes continuan operando normalmente
- El peso total se recalcula solo con los sensores funcionales

Ejemplo: Si el MLX90614 falla, las metricas de temperatura ambiente, humedad y punto de rocio siguen funcionando con sus 3 pesos.

### 7.6 Modo a prueba de fallos (fail-safe)

Si **todos** los sensores fallan o **todas** las metricas estan deshabilitadas:

- La puntuacion agregada es **0.0**
- El estado es **UNSAFE** (inseguro)
- El techo se cierra

**Principio fundamental: ante la duda, cerrar el techo.** El sistema siempre asume el peor caso si no tiene datos.

### 7.7 Diagrama de la maquina de estados

```
                     condiciones inseguras
                 ┌──────────────────────────────┐
                 │                              │
                 v                              │
    ┌────────────────────┐          ┌───────────┴──────────┐
    │                    │          │                      │
    │       SAFE         │────────>│  TRANSITIONING       │
    │  (techo abierto)   │          │  TO_UNSAFE           │
    │                    │<────────│  (contando 120s)     │
    └────────────────────┘          └──────────────────────┘
              ^          condiciones                │
              │          vuelven a                  │ temporizador
              │          ser seguras               │ completado
              │          (se aborta)               │
              │                                    v
    ┌─────────┴──────────┐          ┌──────────────────────┐
    │                    │          │                      │
    │  TRANSITIONING     │<────────│      UNSAFE          │
    │  TO_SAFE           │          │  (techo cerrado)     │
    │  (contando 1200s)  │────────>│                      │
    │                    │          └──────────────────────┘
    └────────────────────┘            condiciones
              │          condiciones    inseguras
              │          vuelven a      (se aborta)
              │          ser inseguras
              v
       temporizador
       completado
           │
           └──> vuelve a SAFE
```

**Tiempos por defecto:**

- SAFE → UNSAFE: 120 segundos (2 minutos) de espera antes de cerrar
- UNSAFE → SAFE: 1200 segundos (20 minutos) de espera antes de abrir

Las transiciones se pueden **abortar**: si durante el conteo las condiciones cambian de vuelta, el temporizador se reinicia y el estado vuelve al anterior.

### 7.8 Ejemplo practico completo

**Escenario: Noche de observacion con frente nuboso**

1. **22:00** - Condiciones claras: temp 12C, humedad 65%, cielo -22C, rocio 5C
   
   - Todas las metricas: SEGURAS
   - Puntuacion: 1.0 → Estado: **SAFE** → Techo abierto

2. **01:30** - Llegan nubes: cielo sube a -14C (supera disarm -15C)
   
   - Metrica cielo: INSEGURA
   - Puntuacion: 3/4 = 0.75 → Todavia seguro (> 0.5)
   - Estado: SAFE (el techo sigue abierto)

3. **01:35** - La humedad sube rapidamente de 70% a 82% en 2 minutos
   
   - Tasa: 6%/min → Supera trend_threshold 5.0
   - **Tendencia fuerza INSEGURO** independientemente de la puntuacion
   - Estado: **TRANSITIONING_TO_UNSAFE** → Comienza cuenta atras de 120s

4. **01:37** - Pasan los 120 segundos y las condiciones no mejoran
   
   - Estado: **UNSAFE** → El rele se desactiva → **Techo se cierra**
   - Se publica por MQTT: `{"isSafe":false,"reason":"trend"}`

5. **02:15** - Las nubes pasan, condiciones mejoran: cielo -20C, humedad 75%
   
   - Todas las metricas: SEGURAS
   - Estado: **TRANSITIONING_TO_SAFE** → Comienza cuenta atras de 1200s (20 min)

6. **02:35** - A los 20 minutos las condiciones siguen buenas
   
   - Estado: **SAFE** → El rele se activa → **Techo se abre**

---

## 8. Integracion ASCOM Alpaca

### 8.1 Que es ASCOM Alpaca

**ASCOM** (Astronomy Common Object Model) es el estandar de comunicacion para equipos astronomicos. Permite que software como N.I.N.A., SGPro o ACP controle telescopios, camaras, monitores meteorologicos y otros dispositivos.

**Alpaca** es la version moderna y multiplataforma de ASCOM que funciona a traves de HTTP/REST por red. No requiere instalar drivers COM de Windows. Cualquier dispositivo en la red local puede ser descubierto y utilizado.

### 8.2 Descubrimiento UDP (puerto 32227)

El ESP32 escucha en el puerto UDP **32227** para peticiones de descubrimiento ASCOM. Cuando un programa compatible envia un broadcast de descubrimiento, el ESP32 responde con su IP y puerto HTTP.

El servidor HTTP de ASCOM opera en el puerto **80** (estandar HTTP).

**Endpoint de management:**

```
GET http://<IP>/management/v1/configureddevices
```

Devuelve los dispositivos disponibles:

- **SafetyMonitor** (dispositivo 0)
- **ObservingConditions** (dispositivo 0)

### 8.3 Dispositivo SafetyMonitor

Tipo ASCOM: `safetymonitor`

**Endpoint principal:**

```
GET http://<IP>/api/v1/safetymonitor/0/issafe
```

Devuelve `true` si las condiciones son seguras (estado SAFE), `false` en cualquier otro estado.

Este es el endpoint que los programas de automatizacion consultan periodicamente para decidir si mantener el techo abierto.

### 8.4 Dispositivo ObservingConditions

Tipo ASCOM: `observingconditions`

**Propiedades disponibles:**

| Propiedad      | Endpoint                                       | Sensor               | Unidad  |
| -------------- | ---------------------------------------------- | -------------------- | ------- |
| Temperature    | `/api/v1/observingconditions/0/temperature`    | BME280               | Celsius |
| Humidity       | `/api/v1/observingconditions/0/humidity`       | BME280               | %       |
| Pressure       | `/api/v1/observingconditions/0/pressure`       | BME280               | hPa     |
| DewPoint       | `/api/v1/observingconditions/0/dewpoint`       | BME280 (calculado)   | Celsius |
| SkyTemperature | `/api/v1/observingconditions/0/skytemperature` | MLX90614 (corregido) | Celsius |
| CloudCover     | `/api/v1/observingconditions/0/cloudcover`     | MLX90614 (estimado)  | %       |

**Propiedades no implementadas** (devuelven 0):

- WindSpeed, WindGust, WindDirection (no hay anemometro)
- SkyBrightness, SkyQuality (no hay sensor SQM)
- RainRate (no hay sensor de lluvia)

### 8.5 Configuracion en N.I.N.A.

1. Abrir N.I.N.A.
2. Ir a **Equipment** (Equipamiento)
3. Para **Safety Monitor**:
   - Desplegar la lista de drivers
   - Seleccionar **ASCOM Alpaca SafetyMonitor**
   - Si no aparece, pulsar **Discover** o "Choose Alpaca Device"
   - El ESP32 deberia aparecer en la lista de dispositivos descubiertos
   - Seleccionar y conectar
4. Para **Weather** (ObservingConditions):
   - Mismo proceso, seleccionar **ASCOM Alpaca ObservingConditions**
   - Conectar
5. En la configuracion de la secuencia, habilitar la comprobacion de seguridad

### 8.6 Configuracion en SGPro (Sequence Generator Pro)

1. Abrir SGPro
2. Ir a **Equipment Profile**
3. En la seccion **Safety Monitor**:
   - Seleccionar **ASCOM Alpaca** como tipo de conexion
   - El dispositivo deberia descubrirse automaticamente
   - Conectar
4. En **Observing Conditions**:
   - Mismo proceso
5. Configurar las acciones de seguridad en la secuencia (pausa, park, etc.)

### 8.7 Otros programas (ACP, etc.)

Para cualquier programa compatible con ASCOM:

1. Si soporta Alpaca nativo, buscar la opcion de descubrimiento Alpaca
2. Si solo soporta ASCOM COM clasico, necesitara instalar el **ASCOM Platform** (https://ascom-standards.org) y crear un perfil Alpaca
3. En ASCOM Diagnostics (incluido en el Platform), puede verificar la conexion:
   - Choose → seleccionar tipo de dispositivo
   - Si el dispositivo no aparece, pulsar "NEW ALPACA DEVICE"
   - Introducir la IP y puerto del ESP32

### 8.8 Solucion de problemas de descubrimiento ASCOM

| Problema                                       | Solucion                                                            |
| ---------------------------------------------- | ------------------------------------------------------------------- |
| El dispositivo no aparece en el descubrimiento | Verificar que el PC y el ESP32 estan en la **misma subred** WiFi    |
| Firewall bloquea el descubrimiento             | Abrir el puerto **UDP 32227** en el firewall de Windows             |
| Timeout de conexion                            | Verificar que el ESP32 esta encendido y conectado a WiFi (LED fijo) |
| "Device not connected"                         | Primero hay que pulsar "Connect" en el software ASCOM               |
| Multiples dispositivos con la misma IP         | Asignar una IP fija al ESP32 en su router (reserva DHCP)            |

---

## 9. MQTT - Notificaciones Remotas

### 9.1 Que es MQTT y para que sirve aqui

**MQTT** (Message Queuing Telemetry Transport) es un protocolo ligero de mensajeria tipo publicador/suscriptor, muy usado en IoT y domotica.

En este proyecto, MQTT es **completamente opcional** y sirve para:

- Recibir notificaciones cuando cambia el estado de seguridad
- Monitorizar valores de sensores desde otros sistemas (Home Assistant, Node-RED, etc.)
- Registrar historicos de datos meteorologicos

### 9.2 Configuracion del broker

Necesita un broker MQTT en su red. Opciones comunes:

- **Mosquitto** (gratuito, se instala en cualquier PC o Raspberry Pi)
- **Broker integrado en Home Assistant**
- **Servicios cloud**: HiveMQ, CloudMQTT (con limitaciones gratuitas)

Configurar en la pestana MQTT (seccion 6.8) la IP del broker, puerto, y credenciales si las hay.

### 9.3 Topics publicados

Con el topic_prefix por defecto `observatory/safety`:

| Topic completo                               | Descripcion                      |
| -------------------------------------------- | -------------------------------- |
| `observatory/safety/state`                   | Estado de seguridad (al cambiar) |
| `observatory/safety/sensors`                 | Valores de sensores (periodico)  |
| `observatory/safety/sensors/BME280/health`   | Cambio de salud del BME280       |
| `observatory/safety/sensors/MLX90614/health` | Cambio de salud del MLX90614     |

### 9.4 Formato de los mensajes JSON

**Mensaje de estado (`/state`):**

```json
{
    "isSafe": true,
    "reason": "all_metrics_safe"
}
```

**Mensaje de sensores (`/sensors`):**

```json
{
    "sensors": [
        {
            "name": "BME280",
            "health": "OK",
            "temperature": 15.3,
            "humidity": 62.5,
            "pressure": 1013.2,
            "dewpoint": 7.8
        },
        {
            "name": "MLX90614",
            "health": "OK",
            "sky_temperature": -21.4,
            "cloud_cover": 0.0,
            "turbulence_db": 0.3
        }
    ]
}
```

**Mensaje de salud (`/sensors/<nombre>/health`):**

```json
{
    "failed": true
}
```

### 9.5 Ejemplo con Mosquitto

Instalar Mosquitto y suscribirse a todos los topics del observatorio:

```bash
# Instalar Mosquitto (ejemplo en Ubuntu/Raspberry Pi)
sudo apt install mosquitto mosquitto-clients

# Suscribirse a todos los topics
mosquitto_sub -h localhost -t "observatory/safety/#" -v
```

Vera mensajes como:

```
observatory/safety/state {"isSafe":true,"reason":"all_metrics_safe"}
observatory/safety/sensors {"sensors":[...]}
```

### 9.6 Integracion con Home Assistant (ejemplo)

Agregar al archivo `configuration.yaml`:

```yaml
mqtt:
  sensor:
    - name: "Observatorio Seguro"
      state_topic: "observatory/safety/state"
      value_template: "{{ 'Seguro' if value_json.isSafe else 'Inseguro' }}"

    - name: "Temperatura Observatorio"
      state_topic: "observatory/safety/sensors"
      value_template: "{{ value_json.sensors[0].temperature }}"
      unit_of_measurement: "C"

    - name: "Humedad Observatorio"
      state_topic: "observatory/safety/sensors"
      value_template: "{{ value_json.sensors[0].humidity }}"
      unit_of_measurement: "%"

    - name: "Temperatura Cielo"
      state_topic: "observatory/safety/sensors"
      value_template: "{{ value_json.sensors[1].sky_temperature }}"
      unit_of_measurement: "C"
```

---

## 10. Actualizacion de Firmware por OTA

### 10.1 Cuando actualizar

Actualice el firmware cuando:

- Se publique una nueva version con mejoras o correcciones
- Se le indique que hay un parche de seguridad

Puede comprobar la version actual en la pestana **OTA** de la interfaz web.

### 10.2 Procedimiento paso a paso

1. **Hacer un respaldo**: Vaya a la pestana **Backup** y pulse **Export Settings** (ver seccion 6.10)
2. Vaya a la pestana **OTA**
3. Verifique la **version actual** que aparece en pantalla
4. Pulse **Seleccionar archivo** y elija el nuevo `firmware.bin`
5. Pulse **Upload & Flash** (boton rojo)
6. Espere a que la barra de progreso llegue al 100%
7. El ESP32 se reiniciara automaticamente
8. La pagina web se recargara tras unos 10 segundos
9. Verifique la nueva version en la pestana OTA

### 10.3 Precauciones importantes

- **Solo suba `firmware.bin`**. El archivo `spiffs.bin` (interfaz web) solo puede actualizarse por cable serial con la herramienta de flasheo (seccion 4)
- **No desconecte la alimentacion** durante la actualizacion
- El ESP32 tiene **doble particion OTA** (app0 + app1): si la nueva version falla al arrancar, el sistema puede volver a la version anterior

### 10.4 Que hacer si la actualizacion falla

| Situacion                    | Solucion                                                               |
| ---------------------------- | ---------------------------------------------------------------------- |
| Progreso se detiene al 0%    | Verificar que el archivo es un .bin valido de ESP32                    |
| Error "Magic byte mismatch"  | El archivo no es un firmware ESP32 valido (debe empezar con byte 0xE9) |
| Error de tamaño              | El firmware es demasiado grande para la particion (max 1.5 MB)         |
| El ESP32 no arranca tras OTA | Reconectar por cable serial y re-flashear (seccion 4)                  |
| Se perdio la configuracion   | Restaurar desde el respaldo JSON (seccion 6.10)                        |

---

## 11. Respaldo y Restauracion

### 11.1 Exportar configuracion

1. Ir a la pestana **Backup**
2. Pulsar **Export Settings**
3. Se descargara automaticamente un archivo `alpaca_settings.json`
4. Guardar este archivo en un lugar seguro

El archivo contiene **toda la configuracion**:

- Hardware (pines, direcciones, hostname)
- Seguridad (umbrales, pesos, tiempos)
- Techo (pines, modo)
- MQTT (broker, credenciales)
- Redes WiFi (incluyendo contrasenas)

### 11.2 Importar configuracion

1. Ir a la pestana **Backup**
2. Seleccionar un archivo `.json` previamente exportado
3. Pulsar **Import Settings**
4. El sistema validara el archivo (debe contener al menos las secciones "hardware" o "safety")
5. La configuracion se aplicara y guardara inmediatamente

### 11.3 Estructura del archivo JSON

```json
{
    "hardware": {
        "i2c_sda": 22,
        "i2c_scl": 21,
        "bme280_addr": 118,
        "mlx90614_addr": 90,
        "wifi_led_pin": 23,
        "measure_interval_ms": 5000,
        "hostname": "alpaca_esp32"
    },
    "roof": {
        "open_pin": 4,
        "close_pin": -1,
        "active_high": true
    },
    "safety": {
        "temperature": { "arm": 1.0, "disarm": 0.0, "weight": 1.0, "enabled": true },
        "sky_temp":    { "arm": -16.0, "disarm": -15.0, "weight": 1.0, "enabled": true },
        "humidity":    { "arm": 85.0, "disarm": 90.0, "weight": 1.0, "enabled": true },
        "dew_margin":  { "arm": 6.0, "disarm": 5.0, "weight": 1.0, "enabled": true },
        "delay_to_open_s": 1200.0,
        "delay_to_close_s": 120.0,
        "trend_window_s": 300.0,
        "trend_threshold": 5.0
    },
    "mqtt": {
        "enabled": false,
        "broker": "",
        "port": 1883,
        "username": "",
        "password": "",
        "topic_prefix": "observatory/safety",
        "publish_interval_ms": 30000
    },
    "wifi_networks": [
        { "ssid": "MiRedWiFi", "password": "mi_contrasena", "priority": 0 }
    ]
}
```

> **Nota sobre las direcciones I2C**: En el JSON se almacenan como numeros decimales (118 = 0x76, 90 = 0x5A).

### 11.4 Edicion manual del archivo de configuracion

Puede editar el archivo JSON con cualquier editor de texto (Notepad++, Visual Studio Code, etc.) antes de importarlo. Esto es util para:

- Preparar configuraciones para multiples dispositivos
- Cambiar valores sin acceder a la interfaz web
- Hacer modificaciones masivas

Asegurese de mantener la estructura JSON valida (verificar con un validador online si es necesario).

---

## 12. Solucion de Problemas

### 12.1 El ESP32 no arranca

**Sintomas**: No hay salida en el monitor serial, LED de la placa no parpadea.

| Causa probable                      | Solucion                                          |
| ----------------------------------- | ------------------------------------------------- |
| Cable USB solo de carga             | Probar con un cable diferente que transmita datos |
| Fuente de alimentacion insuficiente | Usar fuente USB de al menos 1A                    |
| Firmware no grabado correctamente   | Re-flashear siguiendo la seccion 4                |
| Placa defectuosa                    | Verificar con otro ESP32 si esta disponible       |

### 12.2 No aparece la red WiFi del AP

**Sintomas**: El ESP32 arranca pero no aparece la red "alpaca_esp32" en la lista WiFi.

| Causa probable                         | Solucion                                                                                             |
| -------------------------------------- | ---------------------------------------------------------------------------------------------------- |
| Ya tiene credenciales WiFi almacenadas | El ESP32 intenta conectarse primero a las redes almacenadas. Mantener BOOT 3 segundos para forzar AP |
| La red se creo con otro hostname       | Verificar en el monitor serial cual es el SSID del AP                                                |
| Interferencia WiFi                     | Acercarse al ESP32 con el dispositivo movil                                                          |

### 12.3 No se conecta a mi red WiFi

**Sintomas**: Al intentar conectar desde la pestana WiFi, falla la conexion.

| Causa probable                    | Solucion                                                                             |
| --------------------------------- | ------------------------------------------------------------------------------------ |
| Red de 5 GHz                      | El ESP32 **solo soporta WiFi de 2.4 GHz**. Verificar que su red transmite en 2.4 GHz |
| Contrasena incorrecta             | Verificar mayusculas/minusculas y caracteres especiales                              |
| Red oculta                        | Las redes ocultas no aparecen en el escaneo. Escribir el SSID manualmente            |
| Canal WiFi no soportado           | El ESP32 soporta canales 1-13. Canal 14 no esta soportado                            |
| Demasiados dispositivos en la red | Algunos routers limitan el numero de clientes. Verificar la configuracion del router |

### 12.4 Los sensores aparecen como FAILED

**Sintomas**: En la pestana Status, uno o ambos sensores muestran "FAILED" (rojo).

| Causa probable           | Solucion                                                                                                                       |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------ |
| Cableado incorrecto      | Verificar las conexiones SDA, SCL, VCC, GND segun seccion 3                                                                    |
| Voltaje incorrecto       | Los sensores deben estar a **3.3V**, no a 5V                                                                                   |
| Direccion I2C incorrecta | BME280: verificar que SDO esta a GND (direccion 0x76). Ajustar en pestana Hardware si es 0x77                                  |
| Sensor defectuoso        | Probar con otro modulo sensor                                                                                                  |
| Cables demasiado largos  | Cables I2C mayores de 50 cm pueden causar errores. Acortar o usar cable apantallado                                            |
| Pull-ups inadecuados     | Verificar que los modulos breakout tienen resistencias pull-up. Si usa sensores sin placa, agregar 4.7K a SDA y SCL hacia 3.3V |

> **Nota**: El sistema intenta reconectar los sensores fallidos automaticamente cada 10 segundos. Verifique el monitor serial para ver los mensajes de reconexion.

### 12.5 La interfaz web no carga

**Sintomas**: Al acceder a `http://<IP>/setup` aparece error 404 o pagina en blanco.

| Causa probable        | Solucion                                                                            |
| --------------------- | ----------------------------------------------------------------------------------- |
| SPIFFS no fue grabado | Re-flashear `spiffs.bin` en la direccion 0x310000 (seccion 4)                       |
| SPIFFS corrupto       | Re-flashear `spiffs.bin`                                                            |
| IP incorrecta         | Verificar la IP en el monitor serial o usar mDNS: `http://alpaca_esp32.local/setup` |
| Cache del navegador   | Forzar recarga con Ctrl+Shift+R                                                     |

### 12.6 El rele no actua

**Sintomas**: El techo no se abre/cierra aunque el estado de seguridad cambia.

| Causa probable                          | Solucion                                                                                      |
| --------------------------------------- | --------------------------------------------------------------------------------------------- |
| Pin GPIO incorrecto                     | Verificar que el pin configurado en la pestana Roof coincide con la conexion fisica           |
| Active High/Low mal configurado         | Si su rele se activa con LOW, desmarcar "Active High" en la pestana Roof                      |
| Alimentacion del rele                   | Verificar que VCC del rele esta conectado a 5V (VIN), no a 3.3V                               |
| Rele necesita mas corriente             | Algunos reles necesitan mas de lo que el GPIO puede dar. Usar un driver (transistor o MOSFET) |
| El sistema esta en estado TRANSITIONING | El rele solo cambia al llegar al estado final SAFE o UNSAFE, no durante las transiciones      |

### 12.7 ASCOM no descubre el dispositivo

| Causa probable                    | Solucion                                                 |
| --------------------------------- | -------------------------------------------------------- |
| Diferente subred                  | PC y ESP32 deben estar en la **misma red WiFi y subred** |
| Firewall de Windows               | Permitir el puerto **UDP 32227** entrante y saliente     |
| ASCOM Platform no instalado       | Instalar desde https://ascom-standards.org               |
| El ESP32 no esta conectado a WiFi | Verificar LED (fijo = conectado) o monitor serial        |
| VPN activa                        | Desactivar VPN que podria bloquear trafico local         |

### 12.8 MQTT no conecta

| Causa probable               | Solucion                                                            |
| ---------------------------- | ------------------------------------------------------------------- |
| Broker apagado o inaccesible | Verificar que el broker esta corriendo y es alcanzable desde la red |
| IP/puerto incorrecto         | Verificar configuracion en pestana MQTT                             |
| Credenciales incorrectas     | Verificar usuario y contrasena                                      |
| Firewall bloquea puerto 1883 | Abrir el puerto en el firewall del PC/servidor del broker           |

El cliente MQTT intenta reconectar automaticamente cada 30 segundos.

### 12.9 La actualizacion OTA falla

| Causa probable           | Solucion                                                                                                                             |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| Archivo incorrecto       | Solo subir `firmware.bin`. No subir `spiffs.bin` por OTA                                                                             |
| Archivo no es ESP32      | El firmware debe tener el magic byte 0xE9. Si se genera con PlatformIO, el archivo `.pio/build/esp32dev/firmware.bin` es el correcto |
| Archivo demasiado grande | El firmware no puede superar 1.5 MB (tamano de la particion app)                                                                     |
| Conexion WiFi inestable  | Acercarse al router o mejorar la senal                                                                                               |

### 12.10 El sistema marca UNSAFE cuando deberia ser SAFE

| Causa probable           | Solucion                                                                                           |
| ------------------------ | -------------------------------------------------------------------------------------------------- |
| Umbrales muy ajustados   | Revisar los valores arm/disarm en la pestana Safety. El arm debe ser mas conservador que el disarm |
| Tendencias activas       | Verificar que la velocidad de cambio no supera el trend_threshold (5.0 por defecto)                |
| Un sensor esta FAILED    | Los sensores fallidos se excluyen, pero si todos fallan el resultado es UNSAFE                     |
| Peso de una metrica es 0 | Si una metrica insegura tiene peso alto y el resto bajo, puede dominar el calculo                  |

### 12.11 Lectura de temperatura del cielo incorrecta

La temperatura del cielo medida por el MLX90614 pasa por una **correccion algoritmica** que tiene en cuenta la temperatura ambiente. Factores que afectan la lectura:

- El sensor debe tener **linea directa de vision al cielo** sin obstaculos
- Un **tubo o parasol** alrededor del sensor mejora las lecturas evitando radiacion lateral
- La **humedad alta** puede hacer que las lecturas del cielo sean mas calidas de lo esperado
- Verificar que el sensor no tiene **condensacion** en la lente

### 12.12 Monitor serial para diagnostico

El monitor serial es la herramienta mas valiosa para diagnosticar problemas. Todos los mensajes de diagnostico comienzan con el caracter `#`.

**Configuracion:**

- Puerto: COM del ESP32
- Velocidad: **115200** baudios
- 8 bits, sin paridad, 1 bit de parada

**Mensajes importantes a buscar:**

| Mensaje                                 | Significado                       |
| --------------------------------------- | --------------------------------- |
| `# AlpacaESP32 Safety Monitor v2.0.0`   | Firmware arrancado correctamente  |
| `# SettingsManager: Config loaded`      | Configuracion cargada desde flash |
| `# WiFiManager: Connected to 'xxx'`     | Conexion WiFi exitosa             |
| `# WiFiManager: AP started`             | Modo AP activo                    |
| `# BME280: init FAILED`                 | Sensor BME280 no responde         |
| `# MLX90614: init FAILED`               | Sensor MLX90614 no responde       |
| `# BME280: attempting reconnect...`     | Intentando reconectar sensor      |
| `# WiFiManager: Boot button long press` | Boton BOOT detectado              |

---

## 13. Referencia de la API REST

### 13.1 Endpoints de la interfaz web

Todos los endpoints aceptan y devuelven JSON (Content-Type: application/json).

| Metodo | Ruta                   | Descripcion                                 |
| ------ | ---------------------- | ------------------------------------------- |
| GET    | `/setup`               | Pagina web de configuracion                 |
| GET    | `/api/sensors/health`  | Estado de salud y valores de sensores       |
| GET    | `/api/safety/state`    | Estado actual de la maquina de seguridad    |
| GET    | `/api/hardware`        | Obtener configuracion de hardware           |
| POST   | `/api/hardware`        | Guardar configuracion de hardware           |
| GET    | `/api/safety`          | Obtener umbrales de seguridad               |
| POST   | `/api/safety`          | Guardar umbrales de seguridad               |
| GET    | `/api/roof`            | Obtener configuracion del techo             |
| POST   | `/api/roof`            | Guardar configuracion del techo             |
| GET    | `/api/mqtt`            | Obtener configuracion MQTT                  |
| POST   | `/api/mqtt`            | Guardar configuracion MQTT                  |
| GET    | `/api/settings/export` | Descargar toda la configuracion como JSON   |
| POST   | `/api/settings/import` | Importar configuracion desde JSON           |
| GET    | `/api/ota/version`     | Obtener version actual del firmware         |
| POST   | `/api/ota/upload`      | Subir y flashear nuevo firmware (multipart) |
| GET    | `/api/wifi/scan`       | Escanear redes WiFi disponibles             |
| GET    | `/api/wifi/status`     | Estado de la conexion WiFi                  |
| GET    | `/api/wifi/networks`   | Listar redes WiFi almacenadas               |
| POST   | `/api/wifi/connect`    | Conectar a una red WiFi                     |

### 13.2 Endpoints ASCOM Alpaca

| Protocolo | Puerto | Ruta                                           | Descripcion                        |
| --------- | ------ | ---------------------------------------------- | ---------------------------------- |
| UDP       | 32227  | (broadcast)                                    | Descubrimiento de dispositivos     |
| HTTP      | 80     | `/management/v1/configureddevices`             | Lista de dispositivos configurados |
| HTTP      | 80     | `/api/v1/safetymonitor/0/issafe`               | Estado de seguridad (GET)          |
| HTTP      | 80     | `/api/v1/safetymonitor/0/connected`            | Estado de conexion (GET/PUT)       |
| HTTP      | 80     | `/api/v1/observingconditions/0/temperature`    | Temperatura ambiente               |
| HTTP      | 80     | `/api/v1/observingconditions/0/humidity`       | Humedad relativa                   |
| HTTP      | 80     | `/api/v1/observingconditions/0/pressure`       | Presion atmosferica                |
| HTTP      | 80     | `/api/v1/observingconditions/0/dewpoint`       | Punto de rocio                     |
| HTTP      | 80     | `/api/v1/observingconditions/0/skytemperature` | Temperatura del cielo              |
| HTTP      | 80     | `/api/v1/observingconditions/0/cloudcover`     | Cobertura nubosa                   |

---

## 14. Especificaciones Tecnicas

### 14.1 Tabla de particiones de la flash

| Nombre  | Tipo          | Offset   | Tamano | Uso                               |
| ------- | ------------- | -------- | ------ | --------------------------------- |
| nvs     | data (nvs)    | 0x9000   | 20 KB  | Credenciales WiFi (wear-leveled)  |
| otadata | data (ota)    | 0xE000   | 8 KB   | Selector de particion OTA         |
| app0    | app (ota_0)   | 0x10000  | 1.5 MB | Firmware principal                |
| app1    | app (ota_1)   | 0x190000 | 1.5 MB | Firmware de respaldo OTA          |
| spiffs  | data (spiffs) | 0x310000 | 320 KB | Interfaz web y configuracion JSON |

### 14.2 Almacenamiento de configuracion

| Tipo de dato      | Ubicacion             | Tecnologia                 | Supervive a...                   |
| ----------------- | --------------------- | -------------------------- | -------------------------------- |
| Credenciales WiFi | NVS                   | Preferences (wear-leveled) | Reinicios, OTA, flasheo firmware |
| Config general    | SPIFFS `/config.json` | ArduinoJson                | Reinicios, OTA                   |
| Interfaz web      | SPIFFS `/data/www/`   | HTML/JS/CSS                | Reinicios, OTA                   |

> **Nota**: Las credenciales WiFi en NVS sobreviven incluso al re-flashear el firmware, ya que la particion NVS no se borra. Para borrarlas completamente, use la opcion ERASE del Flash Download Tool.

### 14.3 Sensores soportados

#### BME280

| Parametro   | Rango          | Precision   | Resolucion |
| ----------- | -------------- | ----------- | ---------- |
| Temperatura | -40 a +85 C    | +/- 1.0 C   | 0.01 C     |
| Humedad     | 0 a 100 % RH   | +/- 3 % RH  | 0.008 %    |
| Presion     | 300 a 1100 hPa | +/- 1.0 hPa | 0.18 Pa    |

Fabricante: Bosch Sensortec. Interfaz: I2C. Alimentacion: 1.8V a 3.6V.

#### MLX90614

| Parametro         | Rango        | Precision | Resolucion |
| ----------------- | ------------ | --------- | ---------- |
| Temp. objeto (IR) | -70 a +380 C | +/- 0.5 C | 0.02 C     |
| Temp. ambiente    | -40 a +125 C | +/- 0.5 C | 0.02 C     |

Fabricante: Melexis. Interfaz: I2C (SMBus). Alimentacion: 3.0V a 3.6V. Campo de vision: 90 grados.

**Medidas derivadas:**

- **Punto de rocio**: calculado como `T - (100 - RH) / 5` (aproximacion simplificada)
- **Cobertura nubosa**: estimada como `100 + (skyTemp * 6.0)`, limitada entre 0% y 100%
- **Turbulencia**: calculada como la desviacion RMS de 24 muestras del buffer circular

### 14.4 Limites del sistema

| Parametro                     | Limite                | Notas                                 |
| ----------------------------- | --------------------- | ------------------------------------- |
| Redes WiFi almacenadas        | 3 maximo              | Con prioridad configurable            |
| Intervalo minimo de medicion  | 3000 ms               | Menos puede saturar el bus I2C        |
| Longitud del hostname         | 32 caracteres maximo  | Sin espacios ni caracteres especiales |
| Tamano maximo de firmware OTA | 1.5 MB                | Limitado por la particion app         |
| Sensores I2C                  | 2 (BME280 + MLX90614) | Limitado por el firmware actual       |
| Frecuencia WiFi               | Solo 2.4 GHz          | El ESP32 no soporta 5 GHz             |

---

## 15. Glosario

| Termino            | Definicion                                                                                       |
| ------------------ | ------------------------------------------------------------------------------------------------ |
| **ASCOM**          | Astronomy Common Object Model. Estandar de comunicacion para equipos astronomicos                |
| **Alpaca**         | Version HTTP/REST multiplataforma del protocolo ASCOM                                            |
| **AP**             | Access Point (Punto de Acceso). Modo WiFi donde el ESP32 crea su propia red                      |
| **BME280**         | Sensor digital de temperatura, humedad y presion atmosferica (Bosch)                             |
| **Broker**         | Servidor central de MQTT que recibe y distribuye mensajes                                        |
| **Bus I2C**        | Protocolo de comunicacion serial de 2 hilos (SDA + SCL) para conectar sensores                   |
| **ESP32**          | Microcontrolador de Espressif Systems con WiFi y Bluetooth integrados                            |
| **GPIO**           | General Purpose Input/Output. Pines digitales del microcontrolador                               |
| **GND**            | Ground (masa/tierra). Referencia de voltaje comun                                                |
| **Histeresis**     | Uso de dos umbrales distintos (arm/disarm) para evitar oscilaciones                              |
| **I2C**            | Inter-Integrated Circuit. Protocolo de comunicacion (SDA=datos, SCL=reloj)                       |
| **mDNS**           | Multicast DNS. Permite acceder al dispositivo por nombre (ej: alpaca_esp32.local) en lugar de IP |
| **MLX90614**       | Sensor infrarrojo de temperatura sin contacto (Melexis)                                          |
| **MQTT**           | Message Queuing Telemetry Transport. Protocolo de mensajeria para IoT                            |
| **NVS**            | Non-Volatile Storage. Almacenamiento persistente del ESP32 con nivelacion de desgaste            |
| **OTA**            | Over-The-Air. Actualizacion de firmware por WiFi sin cable USB                                   |
| **Punto de rocio** | Temperatura a la que el aire se satura de humedad y comienza a condensar                         |
| **SDA**            | Serial Data. Linea de datos del bus I2C                                                          |
| **SCL**            | Serial Clock. Linea de reloj del bus I2C                                                         |
| **SPIFFS**         | SPI Flash File System. Sistema de archivos en la memoria flash del ESP32                         |
| **STA**            | Station Mode. Modo WiFi donde el ESP32 se conecta a una red existente                            |
| **Topic**          | Canal de comunicacion en MQTT. Similar a un "tema" o "asunto"                                    |
| **VCC**            | Voltage Common Collector. Pin de alimentacion positiva                                           |
| **VIN**            | Voltage Input. Pin de entrada de 5V en el ESP32 (conectado al USB)                               |

---

## 16. Historial de Versiones

| Version | Fecha | Cambios principales                                                                                                                                                                                                                  |
| ------- | ----- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| v2.0.0  | 2025  | Version inicial documentada. Reescritura completa del firmware con interfaz web de 9 pestanas, logica de seguridad con histeresis y tendencias, soporte MQTT, OTA, backup/restore, portal cautivo WiFi, control de rele configurable |

---

*Este manual corresponde al firmware AlpacaESP32 Safety Monitor v2.0.0*
*Proyecto basado en ASCOM Alpaca para ESP32*
