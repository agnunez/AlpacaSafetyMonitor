#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "CircularBuffer.h"
#include "SensorManager.h"
#include "SettingsManager.h"

// Metric indices for the _metricSafe array
enum MetricIndex : uint8_t {
    METRIC_TEMPERATURE = 0,
    METRIC_SKY_TEMP    = 1,
    METRIC_HUMIDITY    = 2,
    METRIC_DEW_MARGIN  = 3,
    METRIC_COUNT       = 4
};

enum class SafetyState : uint8_t {
    SAFE,
    UNSAFE,
    TRANSITIONING_TO_SAFE,
    TRANSITIONING_TO_UNSAFE
};

class SafetyEngine {
public:
    SafetyEngine(SensorManager& sensors, SettingsManager& settings);

    // Main evaluation method -- call every measurement cycle
    void update(unsigned long deltaMs);

    // Overall safety result
    bool isSafe() const { return _state == SafetyState::SAFE; }
    SafetyState state() const { return _state; }

    // JSON status snapshot for diagnostics / web UI
    void statusJson(JsonObject& root) const;

    // Accessor for individual metric status
    bool metricSafe(MetricIndex idx) const { return _metricSafe[idx]; }

    // Trend rates (units / minute)
    float tempTrend()     const { return _tempRate; }
    float humidityTrend() const { return _humidRate; }
    float skyTempTrend()  const { return _skyRate; }

    // Transition timer accessors (seconds remaining)
    float timeToOpen()  const { return _timeToOpenS; }
    float timeToClose() const { return _timeToCloseS; }

    // Weighted aggregation score (0.0 .. 1.0)
    float aggregateScore() const { return _aggregateScore; }

    // Convert state enum to string
    static const char* stateToString(SafetyState s);

    // Public references so device classes can read sensors/settings for JSON
    SensorManager&  _sensors;
    SettingsManager& _settings;

private:

    SafetyState _state = SafetyState::UNSAFE;

    // Per-metric hysteresis state -- true = currently considered safe
    bool _metricSafe[METRIC_COUNT] = { false, false, false, false };

    // Weighted aggregation result
    float _aggregateScore = 0.0f;

    // Transition delay timers (seconds, counting down)
    float _timeToOpenS  = 0.0f;  // UNSAFE -> SAFE countdown
    float _timeToCloseS = 0.0f;  // SAFE -> UNSAFE countdown

    // Trend analysis circular buffers -- 12 samples spanning trend_window_s
    CircularBuffer<float, 12> _tempHistory;
    CircularBuffer<float, 12> _humidHistory;
    CircularBuffer<float, 12> _skyHistory;

    unsigned long _trendSampleAccumMs = 0;  // ms accumulator for trend sampling
    float _trendSampleIntervalS = 0.0f;    // computed from trend_window_s / 12

    float _tempRate  = 0.0f;  // deg C / min
    float _humidRate = 0.0f;  // % RH  / min
    float _skyRate   = 0.0f;  // deg C / min

    // Internal helpers
    void _evaluateMetrics();
    void _computeAggregate();
    void _updateTrends(unsigned long deltaMs);
    float _computeRate(const CircularBuffer<float, 12>& buf, float windowS) const;
    bool _trendForceUnsafe() const;
};
