# AlpacaESP32 Safety Monitor v2.0

ASCOM Alpaca-compliant observatory safety monitor and observing conditions device running on ESP32 (DoitWiFi-32 DevKit).

## Build System

PlatformIO with Arduino framework (espressif32 platform).

```
pio run                 # compile
pio run -t upload       # flash firmware via serial
pio run -t uploadfs     # flash SPIFFS web UI
pio device monitor      # serial console (115200 baud)
```

Custom partition table: `partitions.csv` (dual OTA + 320KB SPIFFS + 24KB NVS).

## Project Structure

```
AlpacaSafetyMonitor/
├── platformio.ini
├── partitions.csv
├── include/version.h           # FW_VERSION define
├── lib/AlpacaServerESP32/src/  # Inlined Alpaca protocol library
│   ├── AlpacaServer.h/.cpp     # UDP discovery + HTTP REST server
│   ├── AlpacaDevice.h/.cpp     # Base device class
│   ├── AlpacaSafetyMonitor.h/.cpp
│   ├── AlpacaObservingConditions.h/.cpp
│   └── AlpacaHelpers.h
├── src/
│   ├── main.cpp                # Orchestrator, REST endpoints, main loop
│   ├── Config.h                # Compile-time defaults (all runtime-overridable)
│   ├── SettingsManager.h/.cpp  # NVS (WiFi) + SPIFFS JSON (everything else)
│   ├── WiFiManager.h/.cpp      # Multi-network, captive portal, GPIO 0 reset
│   ├── ISensor.h               # Abstract sensor interface + health enum
│   ├── BME280Sensor.h/.cpp     # Temperature, humidity, pressure, dewpoint
│   ├── MLX90614Sensor.h/.cpp   # IR sky temperature, cloud cover, turbulence
│   ├── SensorManager.h/.cpp    # I2C bus owner, sensor orchestrator
│   ├── CircularBuffer.h        # Templated ring buffer
│   ├── SafetyEngine.h/.cpp     # Weighted thresholds, hysteresis, trend analysis
│   ├── RoofController.h/.cpp   # GPIO relay control (single/dual mode)
│   ├── MqttClient.h/.cpp       # Optional MQTT notifications
│   ├── OTAHandler.h/.cpp       # Web-based firmware upload
│   ├── SafetyMonitorDevice.h/.cpp      # ASCOM SafetyMonitor device
│   └── ObservingConditionsDevice.h/.cpp # ASCOM ObservingConditions device
└── data/www/
    ├── setup.html              # Tabbed config UI (9 tabs)
    ├── js/app.js               # Web UI logic
    ├── js/jsonFormer.jquery.js # Alpaca device config forms
    ├── js/*.gz                 # jQuery, Bootstrap (gzipped)
    └── css/theme.css           # Dark blue/gray theme
```

## Architecture

Dependency injection: all components receive dependencies via constructor references. No global state except top-level instances in main.cpp.

**Data flow**: `SensorManager.update()` -> `SafetyEngine.update()` -> `SafetyMonitorDevice` / `ObservingConditionsDevice` -> Alpaca HTTP API + MQTT

**Storage**: WiFi credentials in NVS (wear-leveled). All other settings in `/config.json` on SPIFFS.

**Safety logic**: 4-state machine (SAFE, UNSAFE, TRANSITIONING_TO_SAFE, TRANSITIONING_TO_UNSAFE). Each metric has hysteresis (separate arm/disarm thresholds), configurable weight, and enable flag. Failed sensors are excluded from weighted calculation; if all sensors fail, state = UNSAFE.

## Key Endpoints

### Web UI
- `GET /setup` - Configuration interface

### REST API (used by web UI)
- `GET/POST /api/hardware` - I2C pins, addresses, hostname, interval
- `GET/POST /api/safety` - Thresholds, weights, delays, trend settings
- `GET/POST /api/roof` - Relay pins, active high/low
- `GET/POST /api/mqtt` - Broker, credentials, topic, interval
- `GET /api/sensors/health` - Sensor readings + health status
- `GET /api/safety/state` - Current safety state + per-metric status
- `GET /api/settings/export` - Download all settings as JSON
- `POST /api/settings/import` - Upload settings JSON
- `GET /api/ota/version` - Current firmware version
- `POST /api/ota/upload` - Flash new firmware
- `GET /api/wifi/scan` - Scan available networks
- `GET /api/wifi/status` - Current WiFi state
- `GET /api/wifi/networks` - Stored network list
- `POST /api/wifi/connect` - Connect to network

### ASCOM Alpaca
- UDP discovery on port 32227
- `GET /management/v1/configureddevices`
- `/api/v1/safetymonitor/0/*` - SafetyMonitor device
- `/api/v1/observingconditions/0/*` - ObservingConditions device

## Hardware

- **MCU**: ESP32 DoitWiFi-32 DevKit
- **Sensors**: BME280 (I2C, temp/humidity/pressure), MLX90614 (I2C, IR sky temp)
- **GPIO**: Roof relay (configurable pins), WiFi status LED, BOOT button (GPIO 0) for AP mode reset
- **Default I2C**: SDA=22, SCL=21, BME280=0x76, MLX90614=0x5A

## Libraries (lib_deps)

- ArduinoJson ^6.21.0
- ESPAsyncWebServer (me-no-dev)
- Adafruit MLX90614 Library ^2.1.5
- Adafruit BME280 Library ^2.2.4
- PubSubClient ^2.8

## Conventions

- All runtime-configurable defaults defined in `Config.h` with `DEFAULT_` prefix
- Sensor classes implement `ISensor` interface (read, health, name, reconnect)
- Settings structs use ArduinoJson `|` operator pattern for safe defaults on missing keys
- REST POST handlers use `AsyncCallbackJsonWebHandler` (heap-allocated, registered via `addHandler`)
- REST GET handlers use `server->on()` with lambda
