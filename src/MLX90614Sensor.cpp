#include "MLX90614Sensor.h"

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

MLX90614Sensor::MLX90614Sensor(TwoWire& wire)
    : _wire(wire)
{
}

bool MLX90614Sensor::begin()
{
    _initialized = _mlx.begin(0x5A, &_wire);
    if (_initialized) {
        _consecutiveFailures = 0;
        Serial.println(F("# MLX90614: initialized OK"));
    } else {
        Serial.println(F("# MLX90614: init FAILED"));
    }
    return _initialized;
}

bool MLX90614Sensor::read()
{
    _totalReads++;

    if (!_initialized) {
        _failedReads++;
        _consecutiveFailures++;
        return false;
    }

    float ta = _mlx.readAmbientTempC();
    float to = _mlx.readObjectTempC();

    // NaN check -- MLX90614 returns NaN on bus error
    if (isnan(ta) || isnan(to)) {
        _failedReads++;
        _consecutiveFailures++;
        Serial.println(F("# MLX90614: NaN detected in reading"));
        return false;
    }

    _ambientTemp = ta;
    _objectTemp  = to;

    // Compute corrected sky temperature
    _skyTemperature = tsky_calc(_objectTemp, _ambientTemp);

    // Feed circular buffer for turbulence/noise estimation
    addSkyReading(_skyTemperature);
    _noiseDb = noiseDbCalc();

    // Cloud cover: linear mapping clamped to 0-100%
    _cloudCover = 100.0f + (_skyTemperature * 6.0f);
    if (_cloudCover > 100.0f) _cloudCover = 100.0f;
    if (_cloudCover < 0.0f)   _cloudCover = 0.0f;

    _consecutiveFailures = 0;
    return true;
}

SensorHealth MLX90614Sensor::health() const
{
    if (_consecutiveFailures >= FAILED_THRESHOLD)   return SensorHealth::FAILED;
    if (_consecutiveFailures >= DEGRADED_THRESHOLD) return SensorHealth::DEGRADED;
    return SensorHealth::OK;
}

bool MLX90614Sensor::tryReconnect()
{
    unsigned long now = millis();
    if (now - _lastReconnectAttempt < RECONNECT_INTERVAL_MS) {
        return false;   // Too soon since last attempt
    }
    _lastReconnectAttempt = now;

    Serial.println(F("# MLX90614: attempting reconnect..."));
    return begin();
}

void MLX90614Sensor::toJson(JsonObject& obj) const
{
    obj["ambient_temp"] = serialized(isnan(_ambientTemp)    ? "null" : String(_ambientTemp, 2));
    obj["object_temp"]  = serialized(isnan(_objectTemp)     ? "null" : String(_objectTemp, 2));
    obj["sky_temp"]     = serialized(isnan(_skyTemperature) ? "null" : String(_skyTemperature, 2));
    obj["cloud_cover"]  = serialized(isnan(_cloudCover)     ? "null" : String(_cloudCover, 1));
    obj["noise_db"]     = serialized(String(_noiseDb, 2));
    obj["health"]       = healthToString(health());
    obj["total_reads"]  = _totalReads;
    obj["failed_reads"] = _failedReads;
}

// ---------------------------------------------------------------------------
// Proprietary sky temperature correction -- ported from original meteo.cpp
// k[] coefficients define the correction curve for atmospheric conditions.
// ---------------------------------------------------------------------------
float MLX90614Sensor::tsky_calc(float ts, float ta)
{
    float t67, td = 0;
    float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};

    if (abs(k[2] / 10.0f - ta) < 1) {
        t67 = sgn(k[6]) * sgn(ta - k[2] / 10.0f) * abs((k[2] / 10.0f - ta));
    } else {
        t67 = k[6] * sgn(ta - k[2] / 10.0f)
            * (log(abs((k[2] / 10.0f - ta))) / log(10.0f) + k[7] / 100.0f);
    }

    td = (k[1] / 100.0f) * (ta - k[2] / 10.0f)
       + (k[3] / 100.0f) * pow(exp(k[4] / 1000.0f * ta), (k[5] / 100.0f))
       + t67;

    return (ts - td);
}

// ---------------------------------------------------------------------------
// Circular buffer noise calculation
// Mirrors the original cb_add / cb_noise_db_calc logic from meteo.cpp
// ---------------------------------------------------------------------------
void MLX90614Sensor::addSkyReading(float skyTemp)
{
    // Compute RMS before adding (over current buffer contents)
    float rmsVal = _skyBuffer.rms();

    // Record position before adding
    size_t idx = _skyBuffer.count() < 24 ? _skyBuffer.count() : 0;
    // We track index manually because CircularBuffer doesn't expose it
    // Use a simpler approach: compute noise from deviation at current position
    _skyBuffer.add(skyTemp);

    // Noise at this sample = |value| - RMS  (original formula)
    // Store in parallel array at the same index the buffer would use
    static size_t cbIdx = 0;
    _cbNoise[cbIdx] = fabs(skyTemp) - rmsVal;
    cbIdx = (cbIdx + 1) % 24;
}

float MLX90614Sensor::noiseDbCalc() const
{
    float n = 0;
    size_t count = _skyBuffer.count();
    if (count == 0) return 0;

    for (size_t i = 0; i < 24; i++) {
        n += _cbNoise[i] * _cbNoise[i];
    }
    if (n == 0) return 0;
    return 10.0f * log10(n);
}
