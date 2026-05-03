#pragma once
#include <Arduino.h>
#include <functional>
#include "SettingsManager.h"
#include "SafetyEngine.h"

// ASCOM IDome ShutterStatus enum (https://ascom-standards.org/api/)
enum class ShutterStatus : uint8_t {
    Open    = 0,
    Closed  = 1,
    Opening = 2,
    Closing = 3,
    Error   = 4
};

enum class DomeCommandResult : uint8_t {
    Accepted,                 // command honored
    NoOp,                     // already in target state / direction
    BlockedBySafety,          // open rejected: IsSafe=false and override=false
    HardwareNotConfigured     // a required pin is unset (-1)
};

class DomeController {
public:
    DomeController(SettingsManager& settings, SafetyEngine& safety);

    bool begin();
    void update(uint32_t now_ms);

    // Public commands
    DomeCommandResult openShutter();
    DomeCommandResult closeShutter();
    void abortSlew();

    // Read-only state accessors
    ShutterStatus shutterStatus() const { return _status; }
    bool endstopOpen() const { return _endstopOpenStable; }
    bool endstopClosed() const { return _endstopClosedStable; }
    bool safetyOverride() const { return _settings.dome().safety_override; }
    void setSafetyOverride(bool v);

    const char* uniqueId() const { return _settings.dome().unique_id; }

    // Hot-reload of timeouts (e.g., from web UI POST)
    void reloadTimeoutsFromSettings();

    // Notify owner of every status transition
    using StateChangeCallback = std::function<void(ShutterStatus newState, const char* reason)>;
    void setStateChangeCallback(StateChangeCallback cb) { _onChange = cb; }

private:
    SettingsManager& _settings;
    SafetyEngine&    _safety;
    StateChangeCallback _onChange;

    // Cached pin assignments + timeouts (refreshed from settings on begin / reload)
    int8_t   _pinMotorOpen   = -1;
    int8_t   _pinMotorClose  = -1;
    int8_t   _pinMotorPower  = -1;
    int8_t   _pinEndstopOpen = -1;
    int8_t   _pinEndstopClose= -1;
    bool     _endstopActiveLow = true;
    uint32_t _moveTimeoutMs  = 60000;
    uint32_t _reversalMs     = 1500;
    uint32_t _debounceMs     = 40;
    uint32_t _retryPauseMs   = 1500;

    // Current motor direction
    enum class Direction : uint8_t { None, Open, Close };
    Direction _motorDir = Direction::None;

    // State machine
    ShutterStatus _status = ShutterStatus::Error;
    Direction     _target = Direction::None;
    uint32_t _motionStartMs = 0;
    uint32_t _reversalUntilMs = 0;   // wait until this absolute ms before re-asserting motor
    uint32_t _retryUntilMs   = 0;
    uint8_t  _retryAttempt   = 0;     // 0 = first try, 1 = one retry done
    bool     _hardwareReady  = false;

    // Endstop debouncing
    bool _endstopOpenStable    = false;
    bool _endstopClosedStable  = false;
    bool _endstopOpenLast      = false;
    bool _endstopClosedLast    = false;
    uint32_t _endstopOpenEdgeMs   = 0;
    uint32_t _endstopClosedEdgeMs = 0;

    // Pending command flag (set by openShutter/closeShutter, consumed in update())
    Direction _pendingCommand = Direction::None;
    bool      _abortPending   = false;

    // Helpers
    bool _readEndstopRaw(int8_t pin) const;
    void _updateEndstops(uint32_t now_ms);
    void _setMotor(Direction dir);
    void _setStatus(ShutterStatus s, const char* reason);
    void _enterError(const char* reason);
    bool _safetyAllowsOpen() const;
    void _determineBootState();
};
