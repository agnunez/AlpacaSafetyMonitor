#include <Arduino.h>
#include "Config.h"
#include "version.h"

#include "SettingsManager.h"
#include "WiFiManager.h"
#include "SensorManager.h"
#include "SafetyEngine.h"
#include "DomeController.h"
#include "MqttClient.h"
#include "OTAHandler.h"

#include "AlpacaServer.h"
#include "SafetyMonitorDevice.h"
#include "ObservingConditionsDevice.h"
#include "DomeDevice.h"

// --- Top-level instances (dependency injection via references) ---
SettingsManager settings;
WiFiManagerCustom wifiManager(settings);
SensorManager sensorManager(settings);
SafetyEngine safetyEngine(sensorManager, settings);
DomeController domeController(settings, safetyEngine);
MqttClient mqttClient(settings);
OTAHandler otaHandler;

AlpacaServer alpacaServer("Alpaca_ESP32");

SafetyMonitorDevice safetyMonitor(safetyEngine, domeController);
ObservingConditionsDevice observingConditions(sensorManager);
DomeDevice dome(domeController);

unsigned long lastUpdate = 0;
bool previousSafe = false;

void setup()
{
    Serial.begin(115200, SERIAL_8N1);
    Serial.println();
    Serial.print(F("# AlpacaESP32 Safety Monitor v"));
    Serial.println(FW_VERSION);

    // Load configuration (applies defaults first, then loads saved config)
    settings.begin();

    // Connect to WiFi (tries stored networks, falls back to captive portal)
    wifiManager.begin();

    // Wait for WiFi connection or AP mode (max 30s)
    unsigned long wifiStart = millis();
    while (!wifiManager.isConnected() &&
           wifiManager.getState() != WiFiState::AP_MODE &&
           millis() - wifiStart < 30000) {
        wifiManager.loop();
        delay(100);
    }

    if (wifiManager.isConnected()) {
        Serial.print(F("# Connected - IP: "));
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(F("# Running in AP mode for configuration"));
    }

    // Start ASCOM Alpaca server
    alpacaServer.begin(ALPACA_UDP_PORT, ALPACA_TCP_PORT);

    // Register WiFi and OTA endpoints on the web server
    wifiManager.registerEndpoints(alpacaServer.getServerTCP());
    otaHandler.registerEndpoints(alpacaServer.getServerTCP());

    // --- Register REST API endpoints for web UI ---
    AsyncWebServer* srv = alpacaServer.getServerTCP();

    // Settings export/import
    srv->on("/api/settings/export", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = settings.exportAllAsJson();
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
        response->addHeader("Content-Disposition", "attachment; filename=\"alpaca_settings.json\"");
        request->send(response);
    });

    AsyncCallbackJsonWebHandler* importHandler = new AsyncCallbackJsonWebHandler("/api/settings/import", [](AsyncWebServerRequest *request, JsonVariant& json) {
        String jsonStr;
        serializeJson(json, jsonStr);
        if (settings.importFromJson(jsonStr)) {
            request->send(200, "application/json", F("{\"success\":true}"));
        } else {
            request->send(400, "application/json", F("{\"success\":false,\"error\":\"Invalid config\"}"));
        }
    });
    srv->addHandler(importHandler);

    // Sensor health
    srv->on("/api/sensors/health", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", sensorManager.healthJson());
    });

    // Safety state
    srv->on("/api/safety/state", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();
        safetyEngine.statusJson(root);
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Hardware config GET
    srv->on("/api/hardware", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        JsonObject root = doc.to<JsonObject>();
        const HardwareConfig& hw = settings.hardware();
        root["hostname"] = hw.hostname;
        root["i2c_sda"] = hw.i2c_sda;
        root["i2c_scl"] = hw.i2c_scl;
        root["bme280_addr"] = hw.bme280_addr;
        root["mlx90614_addr"] = hw.mlx90614_addr;
        root["wifi_led_pin"] = hw.wifi_led_pin;
        root["measure_interval_ms"] = hw.measure_interval_ms;
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Hardware config POST
    AsyncCallbackJsonWebHandler* hwHandler = new AsyncCallbackJsonWebHandler("/api/hardware", [](AsyncWebServerRequest *request, JsonVariant& json) {
        HardwareConfig& hw = settings.hardware();
        JsonObject obj = json.as<JsonObject>();
        const char* h = obj["hostname"];
        if (h) strlcpy(hw.hostname, h, sizeof(hw.hostname));
        hw.i2c_sda = obj["i2c_sda"] | hw.i2c_sda;
        hw.i2c_scl = obj["i2c_scl"] | hw.i2c_scl;
        hw.bme280_addr = obj["bme280_addr"] | hw.bme280_addr;
        hw.mlx90614_addr = obj["mlx90614_addr"] | hw.mlx90614_addr;
        hw.wifi_led_pin = obj["wifi_led_pin"] | hw.wifi_led_pin;
        hw.measure_interval_ms = obj["measure_interval_ms"] | hw.measure_interval_ms;
        settings.saveSettings();
        request->send(200, "application/json", F("{\"success\":true}"));
    });
    srv->addHandler(hwHandler);

    // Safety thresholds GET
    srv->on("/api/safety", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(1024);
        JsonObject root = doc.to<JsonObject>();
        const SafetyConfig& sf = settings.safety();

        auto writeThreshold = [](JsonObject& parent, const char* name, const SafetyThreshold& t) {
            JsonObject obj = parent.createNestedObject(name);
            obj["arm"] = t.arm_value;
            obj["disarm"] = t.disarm_value;
            obj["weight"] = t.weight;
            obj["enabled"] = t.enabled;
        };
        writeThreshold(root, "temperature", sf.temperature);
        writeThreshold(root, "sky_temp", sf.sky_temp);
        writeThreshold(root, "humidity", sf.humidity);
        writeThreshold(root, "dew_margin", sf.dew_margin);
        root["delay_to_open_s"] = sf.delay_to_open_s;
        root["delay_to_close_s"] = sf.delay_to_close_s;
        root["trend_window_s"] = sf.trend_window_s;
        root["trend_threshold"] = sf.trend_threshold;

        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Safety thresholds POST
    AsyncCallbackJsonWebHandler* sfHandler = new AsyncCallbackJsonWebHandler("/api/safety", [](AsyncWebServerRequest *request, JsonVariant& json) {
        SafetyConfig& sf = settings.safety();
        JsonObject obj = json.as<JsonObject>();

        auto readThreshold = [](JsonObject& parent, const char* name, SafetyThreshold& t) {
            if (JsonObject th = parent[name]) {
                t.arm_value = th["arm"] | t.arm_value;
                t.disarm_value = th["disarm"] | t.disarm_value;
                t.weight = th["weight"] | t.weight;
                t.enabled = th["enabled"] | t.enabled;
            }
        };
        readThreshold(obj, "temperature", sf.temperature);
        readThreshold(obj, "sky_temp", sf.sky_temp);
        readThreshold(obj, "humidity", sf.humidity);
        readThreshold(obj, "dew_margin", sf.dew_margin);
        sf.delay_to_open_s = obj["delay_to_open_s"] | sf.delay_to_open_s;
        sf.delay_to_close_s = obj["delay_to_close_s"] | sf.delay_to_close_s;
        sf.trend_window_s = obj["trend_window_s"] | sf.trend_window_s;
        sf.trend_threshold = obj["trend_threshold"] | sf.trend_threshold;
        settings.saveSettings();
        request->send(200, "application/json", F("{\"success\":true}"));
    });
    srv->addHandler(sfHandler);

    // Dome config GET (pins, timeouts, current state, endstops, override)
    srv->on("/api/dome", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(768);
        JsonObject root = doc.to<JsonObject>();
        const DomeConfig& dm = settings.dome();
        root["motor_open_pin"]     = dm.motor_open_pin;
        root["motor_close_pin"]    = dm.motor_close_pin;
        root["motor_power_pin"]    = dm.motor_power_pin;
        root["endstop_open_pin"]   = dm.endstop_open_pin;
        root["endstop_close_pin"]  = dm.endstop_close_pin;
        root["endstop_active_low"] = dm.endstop_active_low;
        root["move_timeout_ms"]    = dm.move_timeout_ms;
        root["reversal_ms"]        = dm.reversal_ms;
        root["debounce_ms"]        = dm.debounce_ms;
        root["retry_pause_ms"]     = dm.retry_pause_ms;
        root["safety_override"]    = dm.safety_override;
        root["unique_id"]          = dm.unique_id;
        // Live state
        int s = (int)domeController.shutterStatus();
        const char* names[] = {"Open","Closed","Opening","Closing","Error"};
        root["shutter_status"]  = s;
        root["shutter_name"]    = (s >= 0 && s <= 4) ? names[s] : "Unknown";
        root["endstop_open"]    = domeController.endstopOpen();
        root["endstop_closed"]  = domeController.endstopClosed();

        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // Dome config POST
    AsyncCallbackJsonWebHandler* dmHandler = new AsyncCallbackJsonWebHandler("/api/dome",
        [](AsyncWebServerRequest *request, JsonVariant& json) {
            DomeConfig& dm = settings.dome();
            JsonObject obj = json.as<JsonObject>();
            int8_t old_mo = dm.motor_open_pin;
            int8_t old_mc = dm.motor_close_pin;
            int8_t old_mp = dm.motor_power_pin;
            int8_t old_eo = dm.endstop_open_pin;
            int8_t old_ec = dm.endstop_close_pin;

            dm.motor_open_pin     = obj["motor_open_pin"]     | dm.motor_open_pin;
            dm.motor_close_pin    = obj["motor_close_pin"]    | dm.motor_close_pin;
            dm.motor_power_pin    = obj["motor_power_pin"]    | dm.motor_power_pin;
            dm.endstop_open_pin   = obj["endstop_open_pin"]   | dm.endstop_open_pin;
            dm.endstop_close_pin  = obj["endstop_close_pin"]  | dm.endstop_close_pin;
            dm.endstop_active_low = obj["endstop_active_low"] | dm.endstop_active_low;
            dm.move_timeout_ms    = obj["move_timeout_ms"]    | dm.move_timeout_ms;
            dm.reversal_ms        = obj["reversal_ms"]        | dm.reversal_ms;
            dm.debounce_ms        = obj["debounce_ms"]        | dm.debounce_ms;
            dm.retry_pause_ms     = obj["retry_pause_ms"]     | dm.retry_pause_ms;
            if (obj.containsKey("safety_override")) {
                bool v = obj["safety_override"] | dm.safety_override;
                domeController.setSafetyOverride(v);  // also persists
            }

            settings.saveDomeToNVS();

            bool reboot_required = (old_mo != dm.motor_open_pin) ||
                                   (old_mc != dm.motor_close_pin) ||
                                   (old_mp != dm.motor_power_pin) ||
                                   (old_eo != dm.endstop_open_pin) ||
                                   (old_ec != dm.endstop_close_pin);

            // Hot-reload timeouts (safe at runtime)
            domeController.reloadTimeoutsFromSettings();

            char resp[64];
            snprintf(resp, sizeof(resp), "{\"success\":true,\"reboot_required\":%s}",
                     reboot_required ? "true" : "false");
            request->send(200, "application/json", resp);
        });
    srv->addHandler(dmHandler);

    // MQTT config GET
    srv->on("/api/mqtt", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(512);
        JsonObject root = doc.to<JsonObject>();
        const MqttConfig& mq = settings.mqtt();
        root["enabled"] = mq.enabled;
        root["broker"] = mq.broker;
        root["port"] = mq.port;
        root["username"] = mq.username;
        root["password"] = mq.password;
        root["topic_prefix"] = mq.topic_prefix;
        root["publish_interval_ms"] = mq.publish_interval_ms;
        String out;
        serializeJson(doc, out);
        request->send(200, "application/json", out);
    });

    // MQTT config POST
    AsyncCallbackJsonWebHandler* mqHandler = new AsyncCallbackJsonWebHandler("/api/mqtt", [](AsyncWebServerRequest *request, JsonVariant& json) {
        MqttConfig& mq = settings.mqtt();
        JsonObject obj = json.as<JsonObject>();
        mq.enabled = obj["enabled"] | mq.enabled;
        const char* b = obj["broker"]; if (b) strlcpy(mq.broker, b, sizeof(mq.broker));
        mq.port = obj["port"] | mq.port;
        const char* u = obj["username"]; if (u) strlcpy(mq.username, u, sizeof(mq.username));
        const char* p = obj["password"]; if (p) strlcpy(mq.password, p, sizeof(mq.password));
        const char* t = obj["topic_prefix"]; if (t) strlcpy(mq.topic_prefix, t, sizeof(mq.topic_prefix));
        mq.publish_interval_ms = obj["publish_interval_ms"] | mq.publish_interval_ms;
        settings.saveSettings();
        request->send(200, "application/json", F("{\"success\":true}"));
    });
    srv->addHandler(mqHandler);

    // Initialize sensors with configured I2C pins
    sensorManager.begin();

    // Initialize Dome controller (loads NVS, configures pins, determines boot state)
    domeController.begin();
    domeController.setStateChangeCallback(
        [](ShutterStatus s, const char* reason) {
            mqttClient.publishDomeState((int)s, reason);
        });

    // Initialize MQTT (no-op if disabled)
    mqttClient.begin();

    // Register ASCOM devices
    safetyMonitor.begin();
    alpacaServer.addDevice(&safetyMonitor);

    observingConditions.begin();
    alpacaServer.addDevice(&observingConditions);

    dome.begin();
    alpacaServer.addDevice(&dome);

    // Load Alpaca device settings (names, descriptions, thresholds)
    alpacaServer.loadSettings();

    Serial.println(F("# Setup complete"));
    lastUpdate = millis();
}

void loop()
{
    // WiFi management (button check, reconnect, DNS in AP mode)
    wifiManager.loop();

    // MQTT keepalive
    mqttClient.loop();

    // Periodic sensor update and safety evaluation
    unsigned long now = millis();

    // Dome state machine (every iteration -- needs ms resolution for debouncing/timeouts)
    domeController.update((uint32_t)now);

    unsigned long interval = settings.hardware().measure_interval_ms;
    if (interval < 3000) interval = 3000; // enforce minimum

    if (now - lastUpdate >= interval) {
        unsigned long deltaMs = now - lastUpdate;
        lastUpdate = now;

        // Read sensors
        sensorManager.update();

        // Evaluate safety
        safetyEngine.update(deltaMs);
        bool safe = safetyEngine.isSafe();

        // Update ASCOM devices
        safetyMonitor.updateSafetyState(safe);
        observingConditions.update(sensorManager);

        // MQTT notifications on state change
        if (safe != previousSafe) {
            const char* reason = safe ? "All metrics safe" : "Safety threshold exceeded";
            mqttClient.publishSafetyState(safe, reason);
            previousSafe = safe;
        }

        // Periodic MQTT sensor publish
        if (settings.mqtt().enabled && mqttClient.isConnected()) {
            static unsigned long lastMqttPublish = 0;
            if (now - lastMqttPublish >= settings.mqtt().publish_interval_ms) {
                mqttClient.publishSensorValues(sensorManager.healthJson().c_str());
                lastMqttPublish = now;
            }
        }
    }

    delay(50);
}
