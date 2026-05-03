#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include "SettingsManager.h"
#include "Config.h"

class MqttClient {
public:
    explicit MqttClient(SettingsManager& settings);

    void begin();
    void loop();

    void publishSafetyState(bool isSafe, const char* reason);
    void publishSensorValues(const char* json);
    void publishSensorFailure(const char* sensorName, bool failed);
    void publishDomeState(int state, const char* reason);

    static const char* domeStateName(int state);

    bool isConnected();

private:
    SettingsManager& _settings;
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    unsigned long _lastReconnectAttempt;
    bool _initialized;

    bool _reconnect();
};
