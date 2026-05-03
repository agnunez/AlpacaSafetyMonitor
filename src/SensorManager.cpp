#include "SensorManager.h"

SensorManager::SensorManager(SettingsManager& settings)
    : _settings(settings)
{
}

// ---------------------------------------------------------------------------
// Wire bus initialization using configurable SDA/SCL from SettingsManager
// ---------------------------------------------------------------------------
void SensorManager::_initWire()
{
    const HardwareConfig& hw = _settings.hardware();

    Wire.end();
    Wire.setPins(hw.i2c_sda, hw.i2c_scl);
    Wire.begin();
    _wire = &Wire;

    Serial.printf("# SensorManager: Wire started on SDA=%d SCL=%d\n",
                  hw.i2c_sda, hw.i2c_scl);
}

void SensorManager::_destroySensors()
{
    if (_bme) { delete _bme; _bme = nullptr; }
    if (_mlx) { delete _mlx; _mlx = nullptr; }
}

// ---------------------------------------------------------------------------
// begin() -- call once from setup()
// ---------------------------------------------------------------------------
bool SensorManager::begin()
{
    _initWire();

    const HardwareConfig& hw = _settings.hardware();

    // Create sensor instances
    _bme = new BME280Sensor(*_wire, hw.bme280_addr);
    _mlx = new MLX90614Sensor(*_wire);

    bool bmeOk = _bme->begin();
    bool mlxOk = _mlx->begin();

    if (bmeOk && mlxOk) {
        Serial.println(F("# SensorManager: all sensors initialized OK"));
    } else {
        Serial.println(F("# SensorManager: one or more sensors FAILED to initialize"));
    }

    return bmeOk && mlxOk;
}

// ---------------------------------------------------------------------------
// update() -- call periodically from loop()
// ---------------------------------------------------------------------------
void SensorManager::update()
{
    if (_bme) {
        if (_bme->health() == SensorHealth::FAILED) {
            _bme->tryReconnect();
        }
        _bme->read();
    }

    if (_mlx) {
        if (_mlx->health() == SensorHealth::FAILED) {
            _mlx->tryReconnect();
        }
        _mlx->read();
    }
}

// ---------------------------------------------------------------------------
// Aggregated accessors -- delegate to individual sensors
// ---------------------------------------------------------------------------
float SensorManager::temperature() const
{
    return _bme ? _bme->temperature() : NAN;
}

float SensorManager::humidity() const
{
    return _bme ? _bme->humidity() : NAN;
}

float SensorManager::pressure() const
{
    return _bme ? _bme->pressure() : NAN;
}

float SensorManager::dewpoint() const
{
    return _bme ? _bme->dewpoint() : NAN;
}

float SensorManager::skyTemperature() const
{
    return _mlx ? _mlx->skyTemperature() : NAN;
}

float SensorManager::ambientTemperature() const
{
    return _mlx ? _mlx->ambientTemp() : NAN;
}

float SensorManager::cloudCover() const
{
    return _mlx ? _mlx->cloudCover() : NAN;
}

float SensorManager::turbulenceDb() const
{
    return _mlx ? _mlx->noiseDb() : NAN;
}

// ---------------------------------------------------------------------------
// Health
// ---------------------------------------------------------------------------
bool SensorManager::allSensorsOk() const
{
    if (!_bme || !_mlx) return false;
    return _bme->health() == SensorHealth::OK
        && _mlx->health() == SensorHealth::OK;
}

SensorHealth SensorManager::overallHealth() const
{
    if (!_bme || !_mlx) return SensorHealth::FAILED;

    SensorHealth bh = _bme->health();
    SensorHealth mh = _mlx->health();

    // Worst-of: if either is FAILED, overall is FAILED; etc.
    if (bh == SensorHealth::FAILED || mh == SensorHealth::FAILED)
        return SensorHealth::FAILED;
    if (bh == SensorHealth::DEGRADED || mh == SensorHealth::DEGRADED)
        return SensorHealth::DEGRADED;
    return SensorHealth::OK;
}

// ---------------------------------------------------------------------------
// JSON health report for web UI
// ---------------------------------------------------------------------------
String SensorManager::healthJson() const
{
    DynamicJsonDocument doc(512);
    JsonObject root = doc.to<JsonObject>();

    root["overall"] = healthToString(overallHealth());

    if (_bme) {
        JsonObject bmeObj = root.createNestedObject("bme280");
        _bme->toJson(bmeObj);
    }

    if (_mlx) {
        JsonObject mlxObj = root.createNestedObject("mlx90614");
        _mlx->toJson(mlxObj);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// ---------------------------------------------------------------------------
// reconfigureI2C() -- tear down and re-init with new pin settings
// ---------------------------------------------------------------------------
void SensorManager::reconfigureI2C()
{
    Serial.println(F("# SensorManager: reconfiguring I2C bus..."));

    _destroySensors();
    Wire.end();

    // Re-init from current settings
    begin();
}
