#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// Health state for any sensor
enum class SensorHealth : uint8_t {
    OK       = 0,   // No consecutive failures
    DEGRADED = 1,   // 1-4 consecutive failures
    FAILED   = 2    // 5+ consecutive failures
};

// A single sensor reading snapshot
struct SensorReading {
    float         value;
    bool          valid;
    unsigned long timestamp;   // millis() when read
};

// Abstract sensor interface -- all hardware sensors implement this
class ISensor {
public:
    virtual ~ISensor() = default;

    // Initialize the sensor hardware. Returns true on success.
    virtual bool begin() = 0;

    // Perform a read cycle. Returns true if the reading is valid.
    virtual bool read() = 0;

    // Human-readable sensor name (e.g. "BME280", "MLX90614")
    virtual const char* name() const = 0;

    // Current health state derived from consecutive failure count
    virtual SensorHealth health() const = 0;

    // Number of consecutive read failures (resets on success)
    virtual uint16_t consecutiveFailures() const = 0;

    // Attempt to re-initialize the sensor after failure.
    // Returns true if reconnect succeeded.
    virtual bool tryReconnect() = 0;

    // Serialize sensor state into a JSON object for web UI / MQTT
    virtual void toJson(JsonObject& obj) const = 0;
};

// Helper: convert SensorHealth enum to a C string
inline const char* healthToString(SensorHealth h) {
    switch (h) {
        case SensorHealth::OK:       return "OK";
        case SensorHealth::DEGRADED: return "DEGRADED";
        case SensorHealth::FAILED:   return "FAILED";
        default:                     return "UNKNOWN";
    }
}
