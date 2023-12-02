**Alpaca Ascom drivers for ESP32**

@agnuca

This are an ObservingConditions and SafetyMonitor alpaca ascom drivers. (work in progress stage)

For Ascom Alpaca compability it uses AlpacaServerESP32 library (https://github.com/agnunez/AlpacaServerESP32.git)

Its is prepared to be compiled and install with PlatformIO for ESP32.

Brief steps to install:

- Prepare Visual Studio Code and add PlatformIO extension for ESP32. Git extension also is recommended
- clone https://github.com/agnunez/AlpacaSafetyMonitor.git 
- Check that platform.ini contains (or install libraries):

``` 
[env:esp32dev]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino
upload_speed = 921600
monitor_speed = 115200
lib_deps = 
	ESP_WiFiManager
	ArduinoJSON
	https://github.com/me-no-dev/ESPAsyncWebServer.git
	https://github.com/agnunez/AlpacaServerESP32.git
	adafruit/Adafruit MLX90614 Library@^2.1.5
	adafruit/Adafruit BME280 Library@^2.2.4
```

Click on Platform icon (left side)
- Build
- Upload 
in Platform submenu
- Build FilesystemImage (within platform icon submenu)
- Upload FilesystemImage

Connect ESP32 to power and look in you mobile/cellphone an SSID [alpaca_esp32]
Connect without internet. Open browser in your phone at http://192.168.4.1
Choose in the menu your local WiFi SSDI, and insert your password
in a VSCode serial monitor, get ready to follow the connection steps
ESP32 Alpaca device will connect to your network and provide its IP (or check in your router its address) 

Once the installation is ready. Connect BME280(port 0x76) and MLX90614 to pins 22 (SDA) and pin 21 (SCL) (pins are defined in pins.h files) (if not connected yet)

In windows / linux / macos:
- Open a browser and navigate to http://ESP32_IP/setup
you will be ready to configure your minimeteo parameters with [UPDATE] and [REFRESH] current values

In your ASCOM compatible Astronomy program (N.I.N.A, TheSkyX, SharpCap, ...) you will be able to "discover" two new devices
- ObservationConditions (meteo info)
- SafetyMonitor (logic to open/close your observatory depending on ObservationConditions and Safety limits configured through this form)

Enjoy!
@agnuca

ASCOM Astronomy Programs Discovering Problems!!!
================================================

There are serveral observatory programs that are not yet able to discover ASCOM-ALPACA drivers. Follow these steps:

- install ASCOM latest version
- install ASCOM Diagnostics program and execute 
- Select in [Choose Devices->Connect to Devices]  
- [Select Device Type] SafetyMonitor (or ObservingConditions in turn) 
- [Choose]
- ASCOM Chooser will appear
- Choose Alpaca top menu
- Enable Discovery
- Discover now
- Deploy Select form and choos "NEW ALPACA DEVICE" and [OK]

Close and use your software again to see if it discovers your Alpaca drivers.

Enjoy!






