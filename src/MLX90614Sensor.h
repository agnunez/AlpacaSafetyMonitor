#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <ArduinoJson.h>
#include "ISensor.h"
#include "CircularBuffer.h"

class MLX90614Sensor : public ISensor {
public:
    explicit MLX90614Sensor(TwoWire& wire);

    // ISensor interface
    bool         begin() override;
    bool         read() override;
    const char*  name() const override { return "MLX90614"; }
    SensorHealth health() const override;
    uint16_t     consecutiveFailures() const override { return _consecutiveFailures; }
    bool         tryReconnect() override;
    void         toJson(JsonObject& obj) const override;

    // Typed accessors
    float ambientTemp()    const { return _ambientTemp; }
    float objectTemp()     const { return _objectTemp; }
    float skyTemperature() const { return _skyTemperature; }
    float cloudCover()     const { return _cloudCover; }
    float noiseDb()        const { return _noiseDb; }

private:
    Adafruit_MLX90614 _mlx;
    TwoWire&          _wire;
    bool              _initialized = false;

    // Latest readings
    float _ambientTemp    = NAN;
    float _objectTemp     = NAN;
    float _skyTemperature = NAN;
    float _cloudCover     = NAN;
    float _noiseDb        = 0.0f;

    // Circular buffer for sky temperature noise/turbulence calculation
    CircularBuffer<float, 24> _skyBuffer;
    float _cbNoise[24] = {};

    // Health tracking
    uint16_t      _consecutiveFailures = 0;
    unsigned long _totalReads           = 0;
    unsigned long _failedReads          = 0;
    unsigned long _lastReconnectAttempt = 0;

    static constexpr uint16_t DEGRADED_THRESHOLD     = 1;
    static constexpr uint16_t FAILED_THRESHOLD       = 5;
    static constexpr unsigned long RECONNECT_INTERVAL_MS = 10000;

    // Proprietary sky temperature correction (from original meteo.cpp)
    static float tsky_calc(float ts, float ta);

    // Noise dB calculation from circular buffer
    float noiseDbCalc() const;

    // Add a sky temperature sample and update noise tracking
    void addSkyReading(float skyTemp);
};
