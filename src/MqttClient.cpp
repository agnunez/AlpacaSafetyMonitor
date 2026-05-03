#include "MqttClient.h"

static constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 30000;

MqttClient::MqttClient(SettingsManager& settings)
    : _settings(settings)
    , _mqttClient(_wifiClient)
    , _lastReconnectAttempt(0)
    , _initialized(false)
{
}

void MqttClient::begin()
{
    const MqttConfig& cfg = _settings.mqtt();

    if (!cfg.enabled) {
        Serial.println(F("# MqttClient: MQTT disabled, skipping init"));
        return;
    }

    if (strlen(cfg.broker) == 0) {
        Serial.println(F("# MqttClient: No broker configured, skipping init"));
        return;
    }

    _mqttClient.setServer(cfg.broker, cfg.port);
    _initialized = true;

    Serial.printf("# MqttClient: Configured for %s:%d, topic prefix '%s'\n",
                  cfg.broker, cfg.port, cfg.topic_prefix);
}

void MqttClient::loop()
{
    if (!_initialized || !_settings.mqtt().enabled) {
        return;
    }

    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= MQTT_RECONNECT_INTERVAL_MS) {
            _lastReconnectAttempt = now;
            _reconnect();
        }
    } else {
        _mqttClient.loop();
    }
}

void MqttClient::publishSafetyState(bool isSafe, const char* reason)
{
    if (!_initialized || !_settings.mqtt().enabled || !_mqttClient.connected()) {
        return;
    }

    const MqttConfig& cfg = _settings.mqtt();
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/state", cfg.topic_prefix);

    // Build JSON payload
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"isSafe\":%s,\"reason\":\"%s\"}",
             isSafe ? "true" : "false",
             reason ? reason : "");

    _mqttClient.publish(topic, payload);
}

void MqttClient::publishSensorValues(const char* json)
{
    if (!_initialized || !_settings.mqtt().enabled || !_mqttClient.connected()) {
        return;
    }

    const MqttConfig& cfg = _settings.mqtt();
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/sensors", cfg.topic_prefix);

    _mqttClient.publish(topic, json);
}

void MqttClient::publishSensorFailure(const char* sensorName, bool failed)
{
    if (!_initialized || !_settings.mqtt().enabled || !_mqttClient.connected()) {
        return;
    }

    const MqttConfig& cfg = _settings.mqtt();
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/sensors/%s/health", cfg.topic_prefix, sensorName);

    char payload[64];
    snprintf(payload, sizeof(payload),
             "{\"failed\":%s}", failed ? "true" : "false");

    _mqttClient.publish(topic, payload);
}

const char* MqttClient::domeStateName(int state)
{
    switch (state) {
        case 0: return "Open";
        case 1: return "Closed";
        case 2: return "Opening";
        case 3: return "Closing";
        case 4: return "Error";
        default: return "Unknown";
    }
}

void MqttClient::publishDomeState(int state, const char* reason)
{
    if (!_initialized || !_settings.mqtt().enabled || !_mqttClient.connected()) {
        return;
    }

    const MqttConfig& cfg = _settings.mqtt();
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/dome/state", cfg.topic_prefix);

    char payload[192];
    snprintf(payload, sizeof(payload),
             "{\"state\":%d,\"name\":\"%s\",\"reason\":\"%s\"}",
             state, domeStateName(state), reason ? reason : "");

    _mqttClient.publish(topic, payload);
}

bool MqttClient::isConnected()
{
    if (!_initialized || !_settings.mqtt().enabled) {
        return false;
    }
    return _mqttClient.connected();
}

bool MqttClient::_reconnect()
{
    const MqttConfig& cfg = _settings.mqtt();

    Serial.printf("# MqttClient: Connecting to %s:%d...\n", cfg.broker, cfg.port);

    bool connected = false;

    // Generate a unique client ID from the hostname
    char clientId[48];
    snprintf(clientId, sizeof(clientId), "%s_%04X",
             _settings.hardware().hostname,
             (uint16_t)(ESP.getEfuseMac() & 0xFFFF));

    if (strlen(cfg.username) > 0) {
        connected = _mqttClient.connect(clientId, cfg.username, cfg.password);
    } else {
        connected = _mqttClient.connect(clientId);
    }

    if (connected) {
        Serial.println(F("# MqttClient: Connected"));
    } else {
        Serial.printf("# MqttClient: Connection failed, rc=%d\n", _mqttClient.state());
    }

    return connected;
}
