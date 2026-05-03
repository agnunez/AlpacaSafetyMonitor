#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include "Config.h"

struct WiFiCredential {
    char ssid[33];
    char password[65];
    int8_t priority;  // 0 = highest
};

struct HardwareConfig {
    int8_t  i2c_sda;
    int8_t  i2c_scl;
    uint8_t bme280_addr;
    uint8_t mlx90614_addr;
    int8_t  wifi_led_pin;
    uint32_t measure_interval_ms;
    char    hostname[33];
};

struct DomeConfig {
    // GPIO pins
    int8_t  motor_open_pin;
    int8_t  motor_close_pin;
    int8_t  motor_power_pin;        // -1 to disable
    int8_t  endstop_open_pin;
    int8_t  endstop_close_pin;
    bool    endstop_active_low;     // true = NC switch to GND
    // Timeouts (ms)
    uint32_t move_timeout_ms;
    uint32_t reversal_ms;
    uint32_t debounce_ms;
    uint32_t retry_pause_ms;
    // Persistent state
    bool    safety_override;
    uint8_t last_state;             // ShutterStatus enum value (boot recovery hint)
    char    unique_id[33];          // 32-hex char UUID, generated on first boot
};

struct SafetyThreshold {
    float arm_value;        // transition to safe
    float disarm_value;     // transition to unsafe
    float weight;           // 0.0 to 1.0
    bool  enabled;
};

struct SafetyConfig {
    SafetyThreshold temperature;
    SafetyThreshold sky_temp;
    SafetyThreshold humidity;
    SafetyThreshold dew_margin;
    float delay_to_open_s;
    float delay_to_close_s;
    float trend_window_s;
    float trend_threshold;
};

struct MqttConfig {
    bool     enabled;
    char     broker[65];
    uint16_t port;
    char     username[33];
    char     password[65];
    char     topic_prefix[65];
    uint32_t publish_interval_ms;
};

class SettingsManager {
public:
    bool begin();

    // WiFi credentials (NVS)
    bool loadWiFiCredentials(WiFiCredential* creds, uint8_t& count);
    bool saveWiFiCredentials(const WiFiCredential* creds, uint8_t count);

    // App settings (SPIFFS JSON)
    bool loadSettings();
    bool saveSettings();

    // Dome settings (NVS)
    bool loadDomeFromNVS();
    bool saveDomeToNVS();
    bool saveDomeStateToNVS(uint8_t state);
    bool saveDomeOverrideToNVS(bool override);

    // Backup/restore
    String exportAllAsJson();
    bool importFromJson(const String& json);

    // Accessors
    HardwareConfig& hardware() { return _hw; }
    const HardwareConfig& hardware() const { return _hw; }
    DomeConfig& dome() { return _dome; }
    const DomeConfig& dome() const { return _dome; }
    SafetyConfig& safety() { return _safety; }
    const SafetyConfig& safety() const { return _safety; }
    MqttConfig& mqtt() { return _mqtt; }
    const MqttConfig& mqtt() const { return _mqtt; }

    void applyDefaults();

private:
    Preferences _nvs;
    HardwareConfig _hw;
    DomeConfig _dome;
    SafetyConfig _safety;
    MqttConfig _mqtt;

    static constexpr const char* NVS_NAMESPACE = "wifi";
    static constexpr const char* NVS_NAMESPACE_DOME = "dome";
    static constexpr const char* SETTINGS_FILE = "/config.json";

    void _writeHardwareJson(JsonObject& obj);
    void _readHardwareJson(JsonObject& obj);
    void _writeDomeJson(JsonObject& obj);
    void _readDomeJson(JsonObject& obj);
    void _writeSafetyJson(JsonObject& obj);
    void _readSafetyJson(JsonObject& obj);
    void _writeMqttJson(JsonObject& obj);
    void _readMqttJson(JsonObject& obj);

    void _generateDomeUUID(char* out, size_t out_size);
};
