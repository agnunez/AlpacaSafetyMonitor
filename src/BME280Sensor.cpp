#include "BME280Sensor.h"

BME280Sensor::BME280Sensor(TwoWire& wire, uint8_t address)
    : _wire(wire)
    , _address(address)
{
}

bool BME280Sensor::begin()
{
    _initialized = _bme.begin(_address, &_wire);
    if (_initialized) {
        _consecutiveFailures = 0;
        Serial.println(F("# BME280: initialized OK"));
    } else {
        Serial.printf("# BME280: init FAILED at address 0x%02X\n", _address);
    }
    return _initialized;
}

bool BME280Sensor::read()
{
    _totalReads++;

    if (!_initialized) {
        _failedReads++;
        _consecutiveFailures++;
        return false;
    }

    float t = _bme.readTemperature();
    float h = _bme.readHumidity();
    float p = _bme.readPressure() / 100.0f;   // Pa -> hPa

    // NaN check -- BME280 returns NaN on bus error
    if (isnan(t) || isnan(h) || isnan(p)) {
        _failedReads++;
        _consecutiveFailures++;
        Serial.println(F("# BME280: NaN detected in reading"));
        return false;
    }

    _temperature = t;
    _humidity    = h;
    _pressure    = p;
    _dewpoint    = _temperature - (100.0f - _humidity) / 5.0f;

    _consecutiveFailures = 0;
    return true;
}

SensorHealth BME280Sensor::health() const
{
    if (_consecutiveFailures >= FAILED_THRESHOLD)   return SensorHealth::FAILED;
    if (_consecutiveFailures >= DEGRADED_THRESHOLD) return SensorHealth::DEGRADED;
    return SensorHealth::OK;
}

bool BME280Sensor::tryReconnect()
{
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL_MS) {
        return false;   // Too soon since last attempt
    }
    _lastReconnectAttempt = now;

    Serial.println(F("# BME280: attempting reconnect..."));
    return begin();
}

void BME280Sensor::toJson(JsonObject& obj) const
{
    obj["temperature"] = serialized(isnan(_temperature) ? "null" : String(_temperature, 2));
    obj["humidity"]    = serialized(isnan(_humidity)    ? "null" : String(_humidity, 1));
    obj["pressure"]    = serialized(isnan(_pressure)    ? "null" : String(_pressure, 1));
    obj["dewpoint"]    = serialized(isnan(_dewpoint)    ? "null" : String(_dewpoint, 2));
    obj["health"]      = healthToString(health());
    obj["total_reads"]  = _totalReads;
    obj["failed_reads"] = _failedReads;
}
