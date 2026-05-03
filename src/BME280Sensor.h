#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include "ISensor.h"
#include "Config.h"

class BME280Sensor : public ISensor {
public:
    BME280Sensor(TwoWire& wire, uint8_t address = DEFAULT_BME280_ADDR);

    // ISensor interface
    bool         begin() override;
    bool         read() override;
    const char*  name() const override { return "BME280"; }
    SensorHealth health() const override;
    uint16_t     consecutiveFailures() const override { return _consecutiveFailures; }
    bool         tryReconnect() override;
    void         toJson(JsonObject& obj) const override;

    // Typed accessors
    float temperature() const { return _temperature; }
    float humidity()    const { return _humidity; }
    float pressure()    const { return _pressure; }
    float dewpoint()    const { return _dewpoint; }

private:
    Adafruit_BME280 _bme;
    TwoWire&        _wire;
    uint8_t         _address;
    bool            _initialized = false;

    // Latest readings
    float _temperature = NAN;
    float _humidity    = NAN;
    float _pressure    = NAN;
    float _dewpoint    = NAN;

    // Health tracking
    uint16_t      _consecutiveFailures = 0;
    unsigned long _totalReads           = 0;
    unsigned long _failedReads          = 0;
    unsigned long _lastReconnectAttempt = 0;

    static constexpr uint16_t DEGRADED_THRESHOLD     = 1;
    static constexpr uint16_t FAILED_THRESHOLD       = 5;
    static constexpr unsigned long RECONNECT_INTERVAL_MS = 10000;
};
