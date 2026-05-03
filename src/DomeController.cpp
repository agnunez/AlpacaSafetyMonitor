#include "DomeController.h"

DomeController::DomeController(SettingsManager& settings, SafetyEngine& safety)
    : _settings(settings)
    , _safety(safety)
{
}

bool DomeController::begin()
{
    // Load cached settings & generate UID if needed (also creates NVS namespace)
    _settings.loadDomeFromNVS();
    reloadTimeoutsFromSettings();

    const DomeConfig& cfg = _settings.dome();
    _pinMotorOpen     = cfg.motor_open_pin;
    _pinMotorClose    = cfg.motor_close_pin;
    _pinMotorPower    = cfg.motor_power_pin;
    _pinEndstopOpen   = cfg.endstop_open_pin;
    _pinEndstopClose  = cfg.endstop_close_pin;
    _endstopActiveLow = cfg.endstop_active_low;

    // Validate hardware configuration
    _hardwareReady = (_pinMotorOpen >= 0 && _pinMotorClose >= 0 &&
                      _pinEndstopOpen >= 0 && _pinEndstopClose >= 0 &&
                      _pinMotorOpen != _pinMotorClose);

    if (!_hardwareReady) {
        Serial.println(F("# DomeController: hardware NOT configured -- pins missing/duplicated"));
        _status = ShutterStatus::Error;
        return false;
    }

    // Configure motor outputs LOW first to avoid spurious assertion
    pinMode(_pinMotorOpen, OUTPUT);
    digitalWrite(_pinMotorOpen, LOW);
    pinMode(_pinMotorClose, OUTPUT);
    digitalWrite(_pinMotorClose, LOW);
    if (_pinMotorPower >= 0) {
        pinMode(_pinMotorPower, OUTPUT);
        digitalWrite(_pinMotorPower, LOW);
    }
    pinMode(_pinEndstopOpen, INPUT_PULLUP);
    pinMode(_pinEndstopClose, INPUT_PULLUP);

    // Initial endstop reading (no debounce needed at boot -- direct sample)
    _endstopOpenLast    = _readEndstopRaw(_pinEndstopOpen);
    _endstopClosedLast  = _readEndstopRaw(_pinEndstopClose);
    _endstopOpenStable  = _endstopOpenLast;
    _endstopClosedStable= _endstopClosedLast;

    _determineBootState();

    Serial.printf("# DomeController: boot state = %d (%s), uid=%s\n",
                  (int)_status,
                  _status == ShutterStatus::Open    ? "Open"    :
                  _status == ShutterStatus::Closed  ? "Closed"  :
                  _status == ShutterStatus::Opening ? "Opening" :
                  _status == ShutterStatus::Closing ? "Closing" : "Error",
                  _settings.dome().unique_id);

    return true;
}

void DomeController::reloadTimeoutsFromSettings()
{
    const DomeConfig& cfg = _settings.dome();
    _moveTimeoutMs = cfg.move_timeout_ms;
    _reversalMs    = cfg.reversal_ms;
    _debounceMs    = cfg.debounce_ms;
    _retryPauseMs  = cfg.retry_pause_ms;
}

void DomeController::_determineBootState()
{
    bool eo = _endstopOpenStable;
    bool ec = _endstopClosedStable;

    if (eo && ec) {
        _status = ShutterStatus::Error;  // ambiguous: both endstops triggered
        return;
    }
    if (eo) { _status = ShutterStatus::Open;   return; }
    if (ec) { _status = ShutterStatus::Closed; return; }

    // Neither endstop triggered -- consult NVS hint, but DO NOT auto-resume motion
    uint8_t hint = _settings.dome().last_state;
    if (hint == (uint8_t)ShutterStatus::Opening || hint == (uint8_t)ShutterStatus::Closing) {
        // Report informationally; motor stays OFF
        _status = (ShutterStatus)hint;
    } else {
        _status = ShutterStatus::Error;
    }
}

bool DomeController::_readEndstopRaw(int8_t pin) const
{
    if (pin < 0) return false;
    int v = digitalRead(pin);
    return _endstopActiveLow ? (v == LOW) : (v == HIGH);
}

void DomeController::_updateEndstops(uint32_t now_ms)
{
    bool rawOpen   = _readEndstopRaw(_pinEndstopOpen);
    bool rawClosed = _readEndstopRaw(_pinEndstopClose);

    if (rawOpen != _endstopOpenLast) {
        _endstopOpenLast    = rawOpen;
        _endstopOpenEdgeMs  = now_ms;
    } else if (rawOpen != _endstopOpenStable &&
               (now_ms - _endstopOpenEdgeMs) >= _debounceMs) {
        _endstopOpenStable = rawOpen;
    }

    if (rawClosed != _endstopClosedLast) {
        _endstopClosedLast    = rawClosed;
        _endstopClosedEdgeMs  = now_ms;
    } else if (rawClosed != _endstopClosedStable &&
               (now_ms - _endstopClosedEdgeMs) >= _debounceMs) {
        _endstopClosedStable = rawClosed;
    }
}

void DomeController::_setMotor(Direction dir)
{
    if (!_hardwareReady) return;

    // SAFETY: always drop both pins LOW first to prevent shoot-through
    digitalWrite(_pinMotorOpen,  LOW);
    digitalWrite(_pinMotorClose, LOW);
    delayMicroseconds(50);

    if (dir == Direction::Open) {
        if (_pinMotorPower >= 0) digitalWrite(_pinMotorPower, HIGH);
        digitalWrite(_pinMotorOpen, HIGH);
    } else if (dir == Direction::Close) {
        if (_pinMotorPower >= 0) digitalWrite(_pinMotorPower, HIGH);
        digitalWrite(_pinMotorClose, HIGH);
    } else {
        if (_pinMotorPower >= 0) digitalWrite(_pinMotorPower, LOW);
    }
    _motorDir = dir;
}

void DomeController::_setStatus(ShutterStatus s, const char* reason)
{
    if (s == _status) return;
    _status = s;

    // Persist Opening/Closing as boot recovery hints; terminal states are re-derivable
    if (s == ShutterStatus::Opening || s == ShutterStatus::Closing) {
        _settings.saveDomeStateToNVS((uint8_t)s);
    }

    Serial.printf("# DomeController: state -> %d (%s) [%s]\n",
                  (int)s,
                  s == ShutterStatus::Open    ? "Open"    :
                  s == ShutterStatus::Closed  ? "Closed"  :
                  s == ShutterStatus::Opening ? "Opening" :
                  s == ShutterStatus::Closing ? "Closing" : "Error",
                  reason ? reason : "");

    if (_onChange) _onChange(s, reason ? reason : "");
}

void DomeController::_enterError(const char* reason)
{
    _setMotor(Direction::None);
    _target = Direction::None;
    _retryAttempt = 0;
    _setStatus(ShutterStatus::Error, reason);
}

bool DomeController::_safetyAllowsOpen() const
{
    if (_settings.dome().safety_override) return true;
    return _safety.isSafe();
}

DomeCommandResult DomeController::openShutter()
{
    if (!_hardwareReady) return DomeCommandResult::HardwareNotConfigured;
    if (!_safetyAllowsOpen()) return DomeCommandResult::BlockedBySafety;

    if (_status == ShutterStatus::Open) return DomeCommandResult::NoOp;
    if (_target == Direction::Open && _status == ShutterStatus::Opening) {
        return DomeCommandResult::NoOp;
    }
    _pendingCommand = Direction::Open;
    return DomeCommandResult::Accepted;
}

DomeCommandResult DomeController::closeShutter()
{
    if (!_hardwareReady) return DomeCommandResult::HardwareNotConfigured;

    if (_status == ShutterStatus::Closed) return DomeCommandResult::NoOp;
    if (_target == Direction::Close && _status == ShutterStatus::Closing) {
        return DomeCommandResult::NoOp;
    }
    _pendingCommand = Direction::Close;
    return DomeCommandResult::Accepted;
}

void DomeController::abortSlew()
{
    _abortPending = true;
}

void DomeController::setSafetyOverride(bool v)
{
    _settings.saveDomeOverrideToNVS(v);
}

void DomeController::update(uint32_t now_ms)
{
    if (!_hardwareReady) return;

    _updateEndstops(now_ms);

    // 1) Handle abort first
    if (_abortPending) {
        _abortPending = false;
        _setMotor(Direction::None);
        _target = Direction::None;
        _retryAttempt = 0;
        _reversalUntilMs = 0;
        _retryUntilMs = 0;
        _setStatus(ShutterStatus::Error, "abort_slew");
        _pendingCommand = Direction::None;
        return;
    }

    // 2) Handle pending command -- start motion or reverse
    if (_pendingCommand != Direction::None) {
        Direction cmd = _pendingCommand;
        _pendingCommand = Direction::None;

        if (_motorDir != Direction::None && _motorDir != cmd) {
            // Reversal: drop motor, schedule dead-time, set target+status now
            _setMotor(Direction::None);
            _reversalUntilMs = now_ms + _reversalMs;
            _target = cmd;
            _retryAttempt = 0;
            _setStatus(cmd == Direction::Open ? ShutterStatus::Opening : ShutterStatus::Closing,
                       "reverse");
            _motionStartMs = now_ms;  // motion timer resets when motor actually asserts
        } else {
            // Same or no current direction
            _target = cmd;
            _retryAttempt = 0;
            _setStatus(cmd == Direction::Open ? ShutterStatus::Opening : ShutterStatus::Closing,
                       "command");
            // If reversal pending, motor will be asserted later; otherwise assert now
            if (now_ms >= _reversalUntilMs) {
                _setMotor(cmd);
                _motionStartMs = now_ms;
            } else {
                _motionStartMs = _reversalUntilMs;
            }
        }
    }

    // 3) Reversal dead-time expiry: if waiting, assert motor when expired
    if (_reversalUntilMs != 0 && now_ms >= _reversalUntilMs && _target != Direction::None
        && _motorDir == Direction::None
        && (_status == ShutterStatus::Opening || _status == ShutterStatus::Closing)) {
        _setMotor(_target);
        _motionStartMs = now_ms;
        _reversalUntilMs = 0;
    }

    // 4) Retry pause expiry
    if (_retryUntilMs != 0 && now_ms >= _retryUntilMs && _target != Direction::None) {
        _setMotor(_target);
        _motionStartMs = now_ms;
        _retryUntilMs = 0;
    }

    // 5) Check endstops to terminate motion
    if (_motorDir == Direction::Open && _endstopOpenStable) {
        _setMotor(Direction::None);
        _target = Direction::None;
        _retryAttempt = 0;
        _setStatus(ShutterStatus::Open, "endstop");
    } else if (_motorDir == Direction::Close && _endstopClosedStable) {
        _setMotor(Direction::None);
        _target = Direction::None;
        _retryAttempt = 0;
        _setStatus(ShutterStatus::Closed, "endstop");
    }

    // 6) Move timeout / retry handling
    if (_motorDir != Direction::None &&
        (now_ms - _motionStartMs) >= _moveTimeoutMs) {
        _setMotor(Direction::None);
        if (_retryAttempt == 0) {
            _retryAttempt = 1;
            _retryUntilMs = now_ms + _retryPauseMs;
            Serial.println(F("# DomeController: timeout -- retry attempt 1"));
            // _target preserved, _status remains Opening/Closing
        } else {
            _enterError("move_timeout");
        }
    }

    // 7) Sanity: both endstops triggered = error
    if (_endstopOpenStable && _endstopClosedStable) {
        if (_status != ShutterStatus::Error) {
            _enterError("both_endstops");
        }
    }
}
