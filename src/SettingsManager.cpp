#include "SettingsManager.h"

bool SettingsManager::begin()
{
    applyDefaults();

    // Ensure NVS namespace exists (open read-write creates it if missing)
    if (_nvs.begin(NVS_NAMESPACE, false)) {
        _nvs.end();
    } else {
        Serial.println(F("# SettingsManager: WARNING - NVS namespace creation failed"));
    }

    if (!SPIFFS.begin(true)) {
        Serial.println(F("# SettingsManager: SPIFFS mount failed"));
        return false;
    }

    loadSettings();
    return true;
}

void SettingsManager::applyDefaults()
{
    // Hardware
    _hw.i2c_sda = DEFAULT_I2C_SDA;
    _hw.i2c_scl = DEFAULT_I2C_SCL;
    _hw.bme280_addr = DEFAULT_BME280_ADDR;
    _hw.mlx90614_addr = DEFAULT_MLX90614_ADDR;
    _hw.wifi_led_pin = DEFAULT_WIFI_LED_PIN;
    _hw.measure_interval_ms = DEFAULT_MEASURE_INTERVAL_MS;
    strlcpy(_hw.hostname, DEFAULT_HOSTNAME, sizeof(_hw.hostname));

    // Dome
    _dome.motor_open_pin     = DEFAULT_DOME_MOTOR_OPEN_PIN;
    _dome.motor_close_pin    = DEFAULT_DOME_MOTOR_CLOSE_PIN;
    _dome.motor_power_pin    = DEFAULT_DOME_MOTOR_POWER_PIN;
    _dome.endstop_open_pin   = DEFAULT_DOME_ENDSTOP_OPEN_PIN;
    _dome.endstop_close_pin  = DEFAULT_DOME_ENDSTOP_CLOSE_PIN;
    _dome.endstop_active_low = DEFAULT_DOME_ENDSTOP_ACTIVE_LOW;
    _dome.move_timeout_ms    = DEFAULT_DOME_MOVE_TIMEOUT_MS;
    _dome.reversal_ms        = DEFAULT_DOME_REVERSAL_MS;
    _dome.debounce_ms        = DEFAULT_DOME_DEBOUNCE_MS;
    _dome.retry_pause_ms     = DEFAULT_DOME_RETRY_PAUSE_MS;
    _dome.safety_override    = DEFAULT_DOME_SAFETY_OVERRIDE;
    _dome.last_state         = 4;  // Error
    memset(_dome.unique_id, 0, sizeof(_dome.unique_id));

    // Safety thresholds with hysteresis
    _safety.temperature = { DEFAULT_TEMP_ARM, DEFAULT_TEMP_DISARM, DEFAULT_TEMP_WEIGHT, true };
    _safety.sky_temp    = { DEFAULT_SKY_ARM, DEFAULT_SKY_DISARM, DEFAULT_SKY_WEIGHT, true };
    _safety.humidity    = { DEFAULT_HUMID_ARM, DEFAULT_HUMID_DISARM, DEFAULT_HUMID_WEIGHT, true };
    _safety.dew_margin  = { DEFAULT_DEW_ARM, DEFAULT_DEW_DISARM, DEFAULT_DEW_WEIGHT, true };
    _safety.delay_to_open_s = DEFAULT_DELAY_TO_OPEN;
    _safety.delay_to_close_s = DEFAULT_DELAY_TO_CLOSE;
    _safety.trend_window_s = DEFAULT_TREND_WINDOW_S;
    _safety.trend_threshold = DEFAULT_TREND_THRESHOLD;

    // MQTT
    _mqtt.enabled = false;
    memset(_mqtt.broker, 0, sizeof(_mqtt.broker));
    _mqtt.port = DEFAULT_MQTT_PORT;
    memset(_mqtt.username, 0, sizeof(_mqtt.username));
    memset(_mqtt.password, 0, sizeof(_mqtt.password));
    strlcpy(_mqtt.topic_prefix, DEFAULT_MQTT_TOPIC, sizeof(_mqtt.topic_prefix));
    _mqtt.publish_interval_ms = DEFAULT_MQTT_INTERVAL;
}

// --- WiFi credentials in NVS ---

bool SettingsManager::loadWiFiCredentials(WiFiCredential* creds, uint8_t& count)
{
    if (!_nvs.begin(NVS_NAMESPACE, true)) { // read-only; fails if namespace doesn't exist yet
        count = 0;
        return false;
    }
    count = _nvs.getUChar("count", 0);
    if (count > MAX_WIFI_NETWORKS) count = MAX_WIFI_NETWORKS;

    for (uint8_t i = 0; i < count; i++) {
        char key_ssid[12], key_pass[12], key_prio[12];
        snprintf(key_ssid, sizeof(key_ssid), "ssid%d", i);
        snprintf(key_pass, sizeof(key_pass), "pass%d", i);
        snprintf(key_prio, sizeof(key_prio), "prio%d", i);

        String ssid = _nvs.getString(key_ssid, "");
        String pass = _nvs.getString(key_pass, "");
        strlcpy(creds[i].ssid, ssid.c_str(), sizeof(creds[i].ssid));
        strlcpy(creds[i].password, pass.c_str(), sizeof(creds[i].password));
        creds[i].priority = _nvs.getChar(key_prio, i);
    }
    _nvs.end();
    return count > 0;
}

bool SettingsManager::saveWiFiCredentials(const WiFiCredential* creds, uint8_t count)
{
    if (count > MAX_WIFI_NETWORKS) count = MAX_WIFI_NETWORKS;

    _nvs.begin(NVS_NAMESPACE, false); // read-write
    _nvs.putUChar("count", count);

    for (uint8_t i = 0; i < count; i++) {
        char key_ssid[12], key_pass[12], key_prio[12];
        snprintf(key_ssid, sizeof(key_ssid), "ssid%d", i);
        snprintf(key_pass, sizeof(key_pass), "pass%d", i);
        snprintf(key_prio, sizeof(key_prio), "prio%d", i);

        _nvs.putString(key_ssid, creds[i].ssid);
        _nvs.putString(key_pass, creds[i].password);
        _nvs.putChar(key_prio, creds[i].priority);
    }

    // Clear any leftover entries (only if they exist)
    uint8_t storedCount = _nvs.getUChar("count", 0);
    for (uint8_t i = count; i < storedCount && i < MAX_WIFI_NETWORKS; i++) {
        char key_ssid[12], key_pass[12], key_prio[12];
        snprintf(key_ssid, sizeof(key_ssid), "ssid%d", i);
        snprintf(key_pass, sizeof(key_pass), "pass%d", i);
        snprintf(key_prio, sizeof(key_prio), "prio%d", i);
        _nvs.remove(key_ssid);
        _nvs.remove(key_pass);
        _nvs.remove(key_prio);
    }

    _nvs.end();
    return true;
}

// --- App settings in SPIFFS JSON ---

void SettingsManager::_writeHardwareJson(JsonObject& obj)
{
    obj["i2c_sda"] = _hw.i2c_sda;
    obj["i2c_scl"] = _hw.i2c_scl;
    obj["bme280_addr"] = _hw.bme280_addr;
    obj["mlx90614_addr"] = _hw.mlx90614_addr;
    obj["wifi_led_pin"] = _hw.wifi_led_pin;
    obj["measure_interval_ms"] = _hw.measure_interval_ms;
    obj["hostname"] = _hw.hostname;
}

void SettingsManager::_readHardwareJson(JsonObject& obj)
{
    _hw.i2c_sda = obj["i2c_sda"] | _hw.i2c_sda;
    _hw.i2c_scl = obj["i2c_scl"] | _hw.i2c_scl;
    _hw.bme280_addr = obj["bme280_addr"] | _hw.bme280_addr;
    _hw.mlx90614_addr = obj["mlx90614_addr"] | _hw.mlx90614_addr;
    _hw.wifi_led_pin = obj["wifi_led_pin"] | _hw.wifi_led_pin;
    _hw.measure_interval_ms = obj["measure_interval_ms"] | _hw.measure_interval_ms;
    const char* hostname = obj["hostname"];
    if (hostname) strlcpy(_hw.hostname, hostname, sizeof(_hw.hostname));
}

void SettingsManager::_writeDomeJson(JsonObject& obj)
{
    obj["motor_open_pin"]     = _dome.motor_open_pin;
    obj["motor_close_pin"]    = _dome.motor_close_pin;
    obj["motor_power_pin"]    = _dome.motor_power_pin;
    obj["endstop_open_pin"]   = _dome.endstop_open_pin;
    obj["endstop_close_pin"]  = _dome.endstop_close_pin;
    obj["endstop_active_low"] = _dome.endstop_active_low;
    obj["move_timeout_ms"]    = _dome.move_timeout_ms;
    obj["reversal_ms"]        = _dome.reversal_ms;
    obj["debounce_ms"]        = _dome.debounce_ms;
    obj["retry_pause_ms"]     = _dome.retry_pause_ms;
    obj["safety_override"]    = _dome.safety_override;
    obj["unique_id"]          = _dome.unique_id;
}

void SettingsManager::_readDomeJson(JsonObject& obj)
{
    _dome.motor_open_pin     = obj["motor_open_pin"]     | _dome.motor_open_pin;
    _dome.motor_close_pin    = obj["motor_close_pin"]    | _dome.motor_close_pin;
    _dome.motor_power_pin    = obj["motor_power_pin"]    | _dome.motor_power_pin;
    _dome.endstop_open_pin   = obj["endstop_open_pin"]   | _dome.endstop_open_pin;
    _dome.endstop_close_pin  = obj["endstop_close_pin"]  | _dome.endstop_close_pin;
    _dome.endstop_active_low = obj["endstop_active_low"] | _dome.endstop_active_low;
    _dome.move_timeout_ms    = obj["move_timeout_ms"]    | _dome.move_timeout_ms;
    _dome.reversal_ms        = obj["reversal_ms"]        | _dome.reversal_ms;
    _dome.debounce_ms        = obj["debounce_ms"]        | _dome.debounce_ms;
    _dome.retry_pause_ms     = obj["retry_pause_ms"]     | _dome.retry_pause_ms;
    _dome.safety_override    = obj["safety_override"]    | _dome.safety_override;
    const char* uid = obj["unique_id"];
    if (uid) strlcpy(_dome.unique_id, uid, sizeof(_dome.unique_id));
}

void SettingsManager::_generateDomeUUID(char* out, size_t out_size)
{
    // Generate 32-hex char UUID using esp_random()
    if (out_size < 33) return;
    for (int i = 0; i < 4; i++) {
        uint32_t r = esp_random();
        snprintf(out + i * 8, out_size - i * 8, "%08X", r);
    }
    out[32] = '\0';
}

bool SettingsManager::loadDomeFromNVS()
{
    if (!_nvs.begin(NVS_NAMESPACE_DOME, true)) {
        // namespace doesn't exist yet -- create defaults and persist
        _nvs.end();
        if (_nvs.begin(NVS_NAMESPACE_DOME, false)) {
            _generateDomeUUID(_dome.unique_id, sizeof(_dome.unique_id));
            _nvs.end();
            saveDomeToNVS();
            Serial.printf("# SettingsManager: Generated new dome UID = %s\n", _dome.unique_id);
        }
        return false;
    }

    _dome.motor_open_pin     = _nvs.getChar("mo_pin",  _dome.motor_open_pin);
    _dome.motor_close_pin    = _nvs.getChar("mc_pin",  _dome.motor_close_pin);
    _dome.motor_power_pin    = _nvs.getChar("mp_pin",  _dome.motor_power_pin);
    _dome.endstop_open_pin   = _nvs.getChar("eo_pin",  _dome.endstop_open_pin);
    _dome.endstop_close_pin  = _nvs.getChar("ec_pin",  _dome.endstop_close_pin);
    _dome.endstop_active_low = _nvs.getBool("e_actlow", _dome.endstop_active_low);
    _dome.move_timeout_ms    = _nvs.getULong("t_move",  _dome.move_timeout_ms);
    _dome.reversal_ms        = _nvs.getULong("t_rev",   _dome.reversal_ms);
    _dome.debounce_ms        = _nvs.getULong("t_deb",   _dome.debounce_ms);
    _dome.retry_pause_ms     = _nvs.getULong("t_retry", _dome.retry_pause_ms);
    _dome.safety_override    = _nvs.getBool("sf_ovr",   _dome.safety_override);
    _dome.last_state         = _nvs.getUChar("state",   _dome.last_state);

    String uid = _nvs.getString("uid", "");
    if (uid.length() == 32) {
        strlcpy(_dome.unique_id, uid.c_str(), sizeof(_dome.unique_id));
    } else {
        _nvs.end();
        // generate and persist
        _generateDomeUUID(_dome.unique_id, sizeof(_dome.unique_id));
        if (_nvs.begin(NVS_NAMESPACE_DOME, false)) {
            _nvs.putString("uid", _dome.unique_id);
            _nvs.end();
        }
        Serial.printf("# SettingsManager: Generated new dome UID = %s\n", _dome.unique_id);
        return true;
    }

    _nvs.end();
    return true;
}

bool SettingsManager::saveDomeToNVS()
{
    if (!_nvs.begin(NVS_NAMESPACE_DOME, false)) {
        return false;
    }
    _nvs.putChar ("mo_pin",   _dome.motor_open_pin);
    _nvs.putChar ("mc_pin",   _dome.motor_close_pin);
    _nvs.putChar ("mp_pin",   _dome.motor_power_pin);
    _nvs.putChar ("eo_pin",   _dome.endstop_open_pin);
    _nvs.putChar ("ec_pin",   _dome.endstop_close_pin);
    _nvs.putBool ("e_actlow", _dome.endstop_active_low);
    _nvs.putULong("t_move",   _dome.move_timeout_ms);
    _nvs.putULong("t_rev",    _dome.reversal_ms);
    _nvs.putULong("t_deb",    _dome.debounce_ms);
    _nvs.putULong("t_retry",  _dome.retry_pause_ms);
    _nvs.putBool ("sf_ovr",   _dome.safety_override);
    _nvs.putUChar("state",    _dome.last_state);
    if (strlen(_dome.unique_id) == 32) {
        _nvs.putString("uid", _dome.unique_id);
    }
    _nvs.end();
    return true;
}

bool SettingsManager::saveDomeStateToNVS(uint8_t state)
{
    _dome.last_state = state;
    if (!_nvs.begin(NVS_NAMESPACE_DOME, false)) return false;
    _nvs.putUChar("state", state);
    _nvs.end();
    return true;
}

bool SettingsManager::saveDomeOverrideToNVS(bool override)
{
    _dome.safety_override = override;
    if (!_nvs.begin(NVS_NAMESPACE_DOME, false)) return false;
    _nvs.putBool("sf_ovr", override);
    _nvs.end();
    return true;
}

static void writeThresholdJson(JsonObject& obj, const char* name, const SafetyThreshold& t)
{
    JsonObject th = obj.createNestedObject(name);
    th["arm"] = t.arm_value;
    th["disarm"] = t.disarm_value;
    th["weight"] = t.weight;
    th["enabled"] = t.enabled;
}

static void readThresholdJson(JsonObject& obj, const char* name, SafetyThreshold& t)
{
    if (JsonObject th = obj[name]) {
        t.arm_value = th["arm"] | t.arm_value;
        t.disarm_value = th["disarm"] | t.disarm_value;
        t.weight = th["weight"] | t.weight;
        t.enabled = th["enabled"] | t.enabled;
    }
}

void SettingsManager::_writeSafetyJson(JsonObject& obj)
{
    writeThresholdJson(obj, "temperature", _safety.temperature);
    writeThresholdJson(obj, "sky_temp", _safety.sky_temp);
    writeThresholdJson(obj, "humidity", _safety.humidity);
    writeThresholdJson(obj, "dew_margin", _safety.dew_margin);
    obj["delay_to_open_s"] = _safety.delay_to_open_s;
    obj["delay_to_close_s"] = _safety.delay_to_close_s;
    obj["trend_window_s"] = _safety.trend_window_s;
    obj["trend_threshold"] = _safety.trend_threshold;
}

void SettingsManager::_readSafetyJson(JsonObject& obj)
{
    readThresholdJson(obj, "temperature", _safety.temperature);
    readThresholdJson(obj, "sky_temp", _safety.sky_temp);
    readThresholdJson(obj, "humidity", _safety.humidity);
    readThresholdJson(obj, "dew_margin", _safety.dew_margin);
    _safety.delay_to_open_s = obj["delay_to_open_s"] | _safety.delay_to_open_s;
    _safety.delay_to_close_s = obj["delay_to_close_s"] | _safety.delay_to_close_s;
    _safety.trend_window_s = obj["trend_window_s"] | _safety.trend_window_s;
    _safety.trend_threshold = obj["trend_threshold"] | _safety.trend_threshold;
}

void SettingsManager::_writeMqttJson(JsonObject& obj)
{
    obj["enabled"] = _mqtt.enabled;
    obj["broker"] = _mqtt.broker;
    obj["port"] = _mqtt.port;
    obj["username"] = _mqtt.username;
    obj["password"] = _mqtt.password;
    obj["topic_prefix"] = _mqtt.topic_prefix;
    obj["publish_interval_ms"] = _mqtt.publish_interval_ms;
}

void SettingsManager::_readMqttJson(JsonObject& obj)
{
    _mqtt.enabled = obj["enabled"] | _mqtt.enabled;
    const char* broker = obj["broker"];
    if (broker) strlcpy(_mqtt.broker, broker, sizeof(_mqtt.broker));
    _mqtt.port = obj["port"] | _mqtt.port;
    const char* user = obj["username"];
    if (user) strlcpy(_mqtt.username, user, sizeof(_mqtt.username));
    const char* pass = obj["password"];
    if (pass) strlcpy(_mqtt.password, pass, sizeof(_mqtt.password));
    const char* topic = obj["topic_prefix"];
    if (topic) strlcpy(_mqtt.topic_prefix, topic, sizeof(_mqtt.topic_prefix));
    _mqtt.publish_interval_ms = obj["publish_interval_ms"] | _mqtt.publish_interval_ms;
}

bool SettingsManager::loadSettings()
{
    File file = SPIFFS.open(SETTINGS_FILE, FILE_READ);
    if (!file) {
        Serial.println(F("# SettingsManager: No config file, using defaults"));
        return false;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print(F("# SettingsManager: JSON parse error: "));
        Serial.println(error.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();

    if (JsonObject hw = root["hardware"]) _readHardwareJson(hw);
    if (JsonObject sf = root["safety"])   _readSafetyJson(sf);
    if (JsonObject mq = root["mqtt"])     _readMqttJson(mq);

    Serial.println(F("# SettingsManager: Config loaded"));
    return true;
}

bool SettingsManager::saveSettings()
{
    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();

    JsonObject hw = root.createNestedObject("hardware");
    _writeHardwareJson(hw);
    JsonObject sf = root.createNestedObject("safety");
    _writeSafetyJson(sf);
    JsonObject mq = root.createNestedObject("mqtt");
    _writeMqttJson(mq);

    SPIFFS.remove(SETTINGS_FILE);
    File file = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
    if (!file) {
        Serial.println(F("# SettingsManager: Cannot create config file"));
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println(F("# SettingsManager: JSON write failed"));
        file.close();
        return false;
    }

    file.close();
    Serial.println(F("# SettingsManager: Config saved"));
    return true;
}

String SettingsManager::exportAllAsJson()
{
    DynamicJsonDocument doc(6144);
    JsonObject root = doc.to<JsonObject>();

    // App settings
    JsonObject hw = root.createNestedObject("hardware");
    _writeHardwareJson(hw);
    JsonObject dm = root.createNestedObject("dome");
    _writeDomeJson(dm);
    JsonObject sf = root.createNestedObject("safety");
    _writeSafetyJson(sf);
    JsonObject mq = root.createNestedObject("mqtt");
    _writeMqttJson(mq);

    // WiFi credentials
    WiFiCredential creds[MAX_WIFI_NETWORKS];
    uint8_t count = 0;
    loadWiFiCredentials(creds, count);

    JsonArray wifiArr = root.createNestedArray("wifi_networks");
    for (uint8_t i = 0; i < count; i++) {
        JsonObject net = wifiArr.createNestedObject();
        net["ssid"] = creds[i].ssid;
        net["password"] = creds[i].password;
        net["priority"] = creds[i].priority;
    }

    String output;
    serializeJsonPretty(doc, output);
    return output;
}

bool SettingsManager::importFromJson(const String& json)
{
    DynamicJsonDocument doc(6144);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.print(F("# SettingsManager: Import parse error: "));
        Serial.println(error.c_str());
        return false;
    }

    JsonObject root = doc.as<JsonObject>();

    // Validate structure before applying
    if (!root.containsKey("hardware") && !root.containsKey("safety")) {
        Serial.println(F("# SettingsManager: Import missing required sections"));
        return false;
    }

    if (JsonObject hw_obj = root["hardware"]) _readHardwareJson(hw_obj);
    if (JsonObject dm_obj = root["dome"])     { _readDomeJson(dm_obj); saveDomeToNVS(); }
    if (JsonObject sf_obj = root["safety"])   _readSafetyJson(sf_obj);
    if (JsonObject mq_obj = root["mqtt"])     _readMqttJson(mq_obj);

    // Import WiFi credentials if present
    if (JsonArray wifiArr = root["wifi_networks"]) {
        WiFiCredential creds[MAX_WIFI_NETWORKS];
        uint8_t count = 0;
        for (JsonObject net : wifiArr) {
            if (count >= MAX_WIFI_NETWORKS) break;
            strlcpy(creds[count].ssid, net["ssid"] | "", sizeof(creds[count].ssid));
            strlcpy(creds[count].password, net["password"] | "", sizeof(creds[count].password));
            creds[count].priority = net["priority"] | (int8_t)count;
            count++;
        }
        saveWiFiCredentials(creds, count);
    }

    saveSettings();
    Serial.println(F("# SettingsManager: Import complete"));
    return true;
}
