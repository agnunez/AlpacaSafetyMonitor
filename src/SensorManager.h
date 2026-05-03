#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "ISensor.h"
#include "BME280Sensor.h"
#include "MLX90614Sensor.h"
#include "SettingsManager.h"

class SensorManager {
public:
    explicit SensorManager(SettingsManager& settings);

    // Initialize Wire bus and all sensors
    bool begin();

    // Read all sensors; attempt reconnect on failed ones
    void update();

    // --- Aggregated value accessors ---
    float temperature()      const;   // BME280 temperature
    float humidity()         const;   // BME280 humidity
    float pressure()         const;   // BME280 pressure
    float dewpoint()         const;   // BME280 dewpoint
    float skyTemperature()   const;   // MLX90614 corrected sky temp
    float ambientTemperature() const; // MLX90614 ambient
    float cloudCover()       const;   // MLX90614 cloud cover %
    float turbulenceDb()     const;   // MLX90614 noise dB

    // --- Health ---
    bool         allSensorsOk() const;
    SensorHealth overallHealth() const;

    // JSON health report for web UI / diagnostics
    String healthJson() const;

    // Tear down and re-initialize Wire bus with (possibly new) pin settings
    void reconfigureI2C();

    // Direct sensor access (for advanced callers)
    BME280Sensor*    bme280()    { return _bme; }
    MLX90614Sensor*  mlx90614()  { return _mlx; }

private:
    SettingsManager& _settings;

    TwoWire*        _wire = nullptr;
    BME280Sensor*   _bme  = nullptr;
    MLX90614Sensor* _mlx  = nullptr;

    void _initWire();
    void _destroySensors();
};
