#pragma once

// Compile-time defaults -- all overridable at runtime via web UI + SettingsManager

// Device identity
#define DEFAULT_HOSTNAME        "alpaca_esp32"
#define DEFAULT_DEVICE_NAME     "AlpacaESP32driver"

// ASCOM Alpaca ports
#define ALPACA_UDP_PORT         32227
#define ALPACA_TCP_PORT         80

// I2C defaults
#define DEFAULT_I2C_SDA         22
#define DEFAULT_I2C_SCL         21
#define DEFAULT_BME280_ADDR     0x76
#define DEFAULT_MLX90614_ADDR   0x5A

// GPIO defaults
#define DEFAULT_WIFI_LED_PIN    23

// Dome (roll-off roof) GPIO defaults
#define DEFAULT_DOME_MOTOR_OPEN_PIN     25
#define DEFAULT_DOME_MOTOR_CLOSE_PIN    26
#define DEFAULT_DOME_MOTOR_POWER_PIN    27    // -1 to disable
#define DEFAULT_DOME_ENDSTOP_OPEN_PIN   32
#define DEFAULT_DOME_ENDSTOP_CLOSE_PIN  33
#define DEFAULT_DOME_ENDSTOP_ACTIVE_LOW true  // NC switch to GND

// Dome motion timeouts (milliseconds)
#define DEFAULT_DOME_MOVE_TIMEOUT_MS    60000
#define DEFAULT_DOME_REVERSAL_MS        1500
#define DEFAULT_DOME_DEBOUNCE_MS        40
#define DEFAULT_DOME_RETRY_PAUSE_MS     1500
#define DEFAULT_DOME_SAFETY_OVERRIDE    false

// Measurement
#define DEFAULT_MEASURE_INTERVAL_MS 5000  // minimum 3000

// Safety thresholds with hysteresis
#define DEFAULT_TEMP_ARM        1.0f     // safe when ambient > this
#define DEFAULT_TEMP_DISARM     0.0f     // unsafe when ambient < this
#define DEFAULT_TEMP_WEIGHT     1.0f

#define DEFAULT_SKY_ARM         -16.0f   // safe when sky temp < this (clear sky)
#define DEFAULT_SKY_DISARM      -15.0f   // unsafe when sky temp > this (cloudy)
#define DEFAULT_SKY_WEIGHT      1.0f

#define DEFAULT_HUMID_ARM       85.0f    // safe when humidity < this
#define DEFAULT_HUMID_DISARM    90.0f    // unsafe when humidity > this
#define DEFAULT_HUMID_WEIGHT    1.0f

#define DEFAULT_DEW_ARM         6.0f     // safe when (temp - dewpoint) > this
#define DEFAULT_DEW_DISARM      5.0f     // unsafe when (temp - dewpoint) < this
#define DEFAULT_DEW_WEIGHT      1.0f

#define DEFAULT_DELAY_TO_OPEN   1200.0f  // seconds before opening after unsafe
#define DEFAULT_DELAY_TO_CLOSE  120.0f   // seconds before closing after safe->unsafe

// Trend analysis
#define DEFAULT_TREND_WINDOW_S  300.0f   // 5-minute window
#define DEFAULT_TREND_THRESHOLD 5.0f     // units/min rate that triggers unsafe

// MQTT defaults
#define DEFAULT_MQTT_PORT       1883
#define DEFAULT_MQTT_TOPIC      "observatory/safety"
#define DEFAULT_MQTT_INTERVAL   30000    // publish every 30s

// WiFi
#define MAX_WIFI_NETWORKS       3
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_BOOT_BUTTON_PIN    0
#define WIFI_LONG_PRESS_MS      3000
