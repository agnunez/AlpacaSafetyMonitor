#include "SafetyEngine.h"

// Helper: check if a sensor pointer is null or in FAILED health state
static inline bool _isFailed(ISensor* sensor)
{
    return (sensor == nullptr) || (sensor->health() == SensorHealth::FAILED);
}

SafetyEngine::SafetyEngine(SensorManager& sensors, SettingsManager& settings)
    : _sensors(sensors)
    , _settings(settings)
{
    // Compute trend sample interval from config
    _trendSampleIntervalS = settings.safety().trend_window_s / 12.0f;
}

// ---------------------------------------------------------------------------
// Main entry point -- called every measurement cycle
// ---------------------------------------------------------------------------
void SafetyEngine::update(unsigned long deltaMs)
{
    // 1. Evaluate individual metric hysteresis
    _evaluateMetrics();

    // 2. Update trend buffers and compute rates
    _updateTrends(deltaMs);

    // 3. Compute weighted aggregate score
    _computeAggregate();

    // Determine instantaneous safety verdict
    bool instantSafe = (_aggregateScore > 0.5f) && !_trendForceUnsafe();

    // Re-read timing config each cycle so live changes take effect
    const SafetyConfig& cfg = _settings.safety();
    float deltaSec = deltaMs / 1000.0f;

    // 4. State machine with transition delays
    switch (_state) {
        case SafetyState::SAFE:
            if (!instantSafe) {
                _timeToCloseS = cfg.delay_to_close_s;
                _state = SafetyState::TRANSITIONING_TO_UNSAFE;
                Serial.println(F("# SafetyEngine: SAFE -> TRANSITIONING_TO_UNSAFE"));
            }
            break;

        case SafetyState::UNSAFE:
            if (instantSafe) {
                _timeToOpenS = cfg.delay_to_open_s;
                _state = SafetyState::TRANSITIONING_TO_SAFE;
                Serial.println(F("# SafetyEngine: UNSAFE -> TRANSITIONING_TO_SAFE"));
            }
            break;

        case SafetyState::TRANSITIONING_TO_SAFE:
            if (!instantSafe) {
                // Conditions reverted -- abort transition
                _timeToOpenS = 0.0f;
                _state = SafetyState::UNSAFE;
                Serial.println(F("# SafetyEngine: TRANSITIONING_TO_SAFE aborted -> UNSAFE"));
            } else {
                _timeToOpenS -= deltaSec;
                if (_timeToOpenS <= 0.0f) {
                    _timeToOpenS = 0.0f;
                    _state = SafetyState::SAFE;
                    Serial.println(F("# SafetyEngine: Transition complete -> SAFE"));
                }
            }
            break;

        case SafetyState::TRANSITIONING_TO_UNSAFE:
            if (instantSafe) {
                // Conditions recovered -- abort transition
                _timeToCloseS = 0.0f;
                _state = SafetyState::SAFE;
                Serial.println(F("# SafetyEngine: TRANSITIONING_TO_UNSAFE aborted -> SAFE"));
            } else {
                _timeToCloseS -= deltaSec;
                if (_timeToCloseS <= 0.0f) {
                    _timeToCloseS = 0.0f;
                    _state = SafetyState::UNSAFE;
                    Serial.println(F("# SafetyEngine: Transition complete -> UNSAFE"));
                }
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Hysteresis evaluation for each metric
// ---------------------------------------------------------------------------
void SafetyEngine::_evaluateMetrics()
{
    const SafetyConfig& cfg = _settings.safety();
    bool bmeFailed  = _isFailed(_sensors.bme280());
    bool mlxFailed  = _isFailed(_sensors.mlx90614());

    // --- Temperature: safe when ambient > arm, unsafe when ambient < disarm ---
    if (cfg.temperature.enabled && !bmeFailed) {
        float ambient = _sensors.temperature();
        if (_metricSafe[METRIC_TEMPERATURE]) {
            // Currently safe -- flip to unsafe only when crossing disarm
            if (ambient < cfg.temperature.disarm_value) {
                _metricSafe[METRIC_TEMPERATURE] = false;
            }
        } else {
            // Currently unsafe -- flip to safe only when crossing arm
            if (ambient > cfg.temperature.arm_value) {
                _metricSafe[METRIC_TEMPERATURE] = true;
            }
        }
    }

    // --- Sky temperature: safe when sky_temp < arm (clear), unsafe when sky_temp > disarm (cloudy) ---
    if (cfg.sky_temp.enabled && !mlxFailed) {
        float sky = _sensors.skyTemperature();
        if (_metricSafe[METRIC_SKY_TEMP]) {
            if (sky > cfg.sky_temp.disarm_value) {
                _metricSafe[METRIC_SKY_TEMP] = false;
            }
        } else {
            if (sky < cfg.sky_temp.arm_value) {
                _metricSafe[METRIC_SKY_TEMP] = true;
            }
        }
    }

    // --- Humidity: safe when humidity < arm, unsafe when humidity > disarm ---
    if (cfg.humidity.enabled && !bmeFailed) {
        float hum = _sensors.humidity();
        if (_metricSafe[METRIC_HUMIDITY]) {
            if (hum > cfg.humidity.disarm_value) {
                _metricSafe[METRIC_HUMIDITY] = false;
            }
        } else {
            if (hum < cfg.humidity.arm_value) {
                _metricSafe[METRIC_HUMIDITY] = true;
            }
        }
    }

    // --- Dew margin: safe when (temp - dewpoint) > arm, unsafe when < disarm ---
    if (cfg.dew_margin.enabled && !bmeFailed) {
        float margin = _sensors.temperature() - _sensors.dewpoint();
        if (_metricSafe[METRIC_DEW_MARGIN]) {
            if (margin < cfg.dew_margin.disarm_value) {
                _metricSafe[METRIC_DEW_MARGIN] = false;
            }
        } else {
            if (margin > cfg.dew_margin.arm_value) {
                _metricSafe[METRIC_DEW_MARGIN] = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Weighted aggregation -- skip failed / disabled metrics
// ---------------------------------------------------------------------------
void SafetyEngine::_computeAggregate()
{
    const SafetyConfig& cfg = _settings.safety();
    const SafetyThreshold* thresholds[METRIC_COUNT] = {
        &cfg.temperature,
        &cfg.sky_temp,
        &cfg.humidity,
        &cfg.dew_margin
    };

    bool bmeFailed = _isFailed(_sensors.bme280());
    bool mlxFailed = _isFailed(_sensors.mlx90614());

    // Determine which sensors are failed per metric
    bool sensorFailed[METRIC_COUNT] = {
        bmeFailed,    // temperature
        mlxFailed,    // sky_temp
        bmeFailed,    // humidity
        bmeFailed     // dew_margin
    };

    float weightedSum = 0.0f;
    float totalWeight = 0.0f;
    uint8_t activeCount = 0;

    for (uint8_t i = 0; i < METRIC_COUNT; i++) {
        if (!thresholds[i]->enabled) continue;   // metric disabled by user
        if (sensorFailed[i])         continue;   // sensor failed -- exclude

        float w = thresholds[i]->weight;
        totalWeight += w;
        weightedSum += w * (_metricSafe[i] ? 1.0f : 0.0f);
        activeCount++;
    }

    if (activeCount == 0) {
        // All sensors failed or all metrics disabled -- fail safe
        _aggregateScore = 0.0f;
        return;
    }

    _aggregateScore = (totalWeight > 0.0f) ? (weightedSum / totalWeight) : 0.0f;
}

// ---------------------------------------------------------------------------
// Trend analysis -- sample into circular buffers, compute rates
// ---------------------------------------------------------------------------
void SafetyEngine::_updateTrends(unsigned long deltaMs)
{
    const SafetyConfig& cfg = _settings.safety();

    // Recompute sample interval in case config changed
    _trendSampleIntervalS = cfg.trend_window_s / 12.0f;
    if (_trendSampleIntervalS < 1.0f) _trendSampleIntervalS = 1.0f;

    _trendSampleAccumMs += deltaMs;
    unsigned long intervalMs = (unsigned long)(_trendSampleIntervalS * 1000.0f);

    if (_trendSampleAccumMs >= intervalMs) {
        _trendSampleAccumMs = 0;

        // Sample current values (only if sensor is not failed)
        if (!_isFailed(_sensors.bme280())) {
            _tempHistory.add(_sensors.temperature());
            _humidHistory.add(_sensors.humidity());
        }
        if (!_isFailed(_sensors.mlx90614())) {
            _skyHistory.add(_sensors.skyTemperature());
        }
    }

    // Compute rates (units / minute)
    _tempRate  = _computeRate(_tempHistory,  cfg.trend_window_s);
    _humidRate = _computeRate(_humidHistory, cfg.trend_window_s);
    _skyRate   = _computeRate(_skyHistory,   cfg.trend_window_s);
}

float SafetyEngine::_computeRate(const CircularBuffer<float, 12>& buf, float windowS) const
{
    if (buf.count() < 2) return 0.0f;
    float deltaVal = buf.newest() - buf.oldest();
    float windowMin = windowS / 60.0f;
    if (windowMin < 0.001f) return 0.0f;
    return deltaVal / windowMin;
}

bool SafetyEngine::_trendForceUnsafe() const
{
    float threshold = _settings.safety().trend_threshold;
    // Rapid humidity increase or sky temperature increase -> force unsafe
    if (_humidRate > threshold) return true;
    if (_skyRate > threshold)   return true;
    return false;
}

// ---------------------------------------------------------------------------
// JSON status snapshot
// ---------------------------------------------------------------------------
void SafetyEngine::statusJson(JsonObject& root) const
{
    root[F("state")] = stateToString(_state);
    root[F("is_safe")] = isSafe();
    root[F("aggregate_score")] = _aggregateScore;

    // Per-metric status
    JsonObject metrics = root.createNestedObject(F("metrics"));
    metrics[F("temperature_safe")] = _metricSafe[METRIC_TEMPERATURE];
    metrics[F("sky_temp_safe")]    = _metricSafe[METRIC_SKY_TEMP];
    metrics[F("humidity_safe")]    = _metricSafe[METRIC_HUMIDITY];
    metrics[F("dew_margin_safe")]  = _metricSafe[METRIC_DEW_MARGIN];

    // Trends
    JsonObject trends = root.createNestedObject(F("trends"));
    trends[F("temp_rate_per_min")]     = _tempRate;
    trends[F("humidity_rate_per_min")] = _humidRate;
    trends[F("sky_temp_rate_per_min")] = _skyRate;
    trends[F("trend_forcing_unsafe")]  = _trendForceUnsafe();

    // Timers
    JsonObject timers = root.createNestedObject(F("timers"));
    timers[F("time_to_open_s")]  = _timeToOpenS;
    timers[F("time_to_close_s")] = _timeToCloseS;
}

// ---------------------------------------------------------------------------
// State enum to string conversion
// ---------------------------------------------------------------------------
const char* SafetyEngine::stateToString(SafetyState s)
{
    switch (s) {
        case SafetyState::SAFE:                     return "SAFE";
        case SafetyState::UNSAFE:                   return "UNSAFE";
        case SafetyState::TRANSITIONING_TO_SAFE:    return "TRANSITIONING_TO_SAFE";
        case SafetyState::TRANSITIONING_TO_UNSAFE:  return "TRANSITIONING_TO_UNSAFE";
        default:                                    return "UNKNOWN";
    }
}
