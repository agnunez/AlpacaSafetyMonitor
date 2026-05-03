#include "SafetyMonitorDevice.h"

// Static array pattern -- keeps up to 4 device instances (matches original project convention)
uint8_t SafetyMonitorDevice::_n_devices = 0;
SafetyMonitorDevice* SafetyMonitorDevice::_device_array[4] = { nullptr, nullptr, nullptr, nullptr };

// ---------------------------------------------------------------------------
SafetyMonitorDevice::SafetyMonitorDevice(SafetyEngine& engine, DomeController& dome)
    : AlpacaSafetyMonitor()
    , _engine(engine)
    , _dome(dome)
{
    _device_index = _n_devices++;
}

// ---------------------------------------------------------------------------
bool SafetyMonitorDevice::begin()
{
    _device_array[_device_index] = this;
    return true;
}

// ---------------------------------------------------------------------------
// Propagate safety verdict from SafetyEngine. Auto-close-only: never auto-opens.
// Opening is allowed only via explicit ASCOM Dome client command (NINA, etc.).
// ---------------------------------------------------------------------------
void SafetyMonitorDevice::updateSafetyState(bool safe)
{
    bool previous = _issafe;
    _issafe = safe;

    if (!safe && previous) {
        // Transition to unsafe -- close shutter (dome will ignore if already Closed)
        _dome.closeShutter();
        Serial.println(F("# SafetyMonitorDevice: UNSAFE -> closing shutter"));
    }
}

// ---------------------------------------------------------------------------
// ASCOM Alpaca: GET /issafe
// ---------------------------------------------------------------------------
void SafetyMonitorDevice::aGetIsSafe(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _issafe);
}

// ---------------------------------------------------------------------------
// JSON read -- deserialize config from web UI (jsonFormer compatible)
// ---------------------------------------------------------------------------
void SafetyMonitorDevice::aReadJson(JsonObject& root)
{
    AlpacaSafetyMonitor::aReadJson(root);

    // Configuration section -- safety thresholds and delays
    if (JsonObject obj_config = root[F("Configuration")]) {
        SafetyConfig& cfg = _engine._settings.safety();

        cfg.temperature.arm_value    = obj_config[F("Temp_Arm")]       | cfg.temperature.arm_value;
        cfg.temperature.disarm_value = obj_config[F("Temp_Disarm")]    | cfg.temperature.disarm_value;
        cfg.temperature.weight       = obj_config[F("Temp_Weight")]    | cfg.temperature.weight;
        cfg.temperature.enabled      = obj_config[F("Temp_Enabled")]   | cfg.temperature.enabled;

        cfg.sky_temp.arm_value    = obj_config[F("Sky_Arm")]       | cfg.sky_temp.arm_value;
        cfg.sky_temp.disarm_value = obj_config[F("Sky_Disarm")]    | cfg.sky_temp.disarm_value;
        cfg.sky_temp.weight       = obj_config[F("Sky_Weight")]    | cfg.sky_temp.weight;
        cfg.sky_temp.enabled      = obj_config[F("Sky_Enabled")]   | cfg.sky_temp.enabled;

        cfg.humidity.arm_value    = obj_config[F("Humid_Arm")]     | cfg.humidity.arm_value;
        cfg.humidity.disarm_value = obj_config[F("Humid_Disarm")]  | cfg.humidity.disarm_value;
        cfg.humidity.weight       = obj_config[F("Humid_Weight")]  | cfg.humidity.weight;
        cfg.humidity.enabled      = obj_config[F("Humid_Enabled")] | cfg.humidity.enabled;

        cfg.dew_margin.arm_value    = obj_config[F("Dew_Arm")]     | cfg.dew_margin.arm_value;
        cfg.dew_margin.disarm_value = obj_config[F("Dew_Disarm")]  | cfg.dew_margin.disarm_value;
        cfg.dew_margin.weight       = obj_config[F("Dew_Weight")]  | cfg.dew_margin.weight;
        cfg.dew_margin.enabled      = obj_config[F("Dew_Enabled")] | cfg.dew_margin.enabled;

        cfg.delay_to_open_s  = obj_config[F("Delay_to_Open")]  | cfg.delay_to_open_s;
        cfg.delay_to_close_s = obj_config[F("Delay_to_Close")] | cfg.delay_to_close_s;
        cfg.trend_window_s   = obj_config[F("Trend_Window")]   | cfg.trend_window_s;
        cfg.trend_threshold  = obj_config[F("Trend_Threshold")]| cfg.trend_threshold;
    }

    // Restore persisted roof state if present
    _issafe = root[F("State")][F("Safety_Monitor_Status")] | _issafe;
}

// ---------------------------------------------------------------------------
// JSON write -- serialize current config + state for the /setup web page
// ---------------------------------------------------------------------------
void SafetyMonitorDevice::aWriteJson(JsonObject& root)
{
    AlpacaSafetyMonitor::aWriteJson(root);

    const SafetyConfig& cfg = _engine._settings.safety();

    // Configuration section
    JsonObject obj_config = root.createNestedObject(F("Configuration"));

    obj_config[F("Temp_Arm")]       = cfg.temperature.arm_value;
    obj_config[F("Temp_Disarm")]    = cfg.temperature.disarm_value;
    obj_config[F("Temp_Weight")]    = cfg.temperature.weight;
    obj_config[F("Temp_Enabled")]   = cfg.temperature.enabled;

    obj_config[F("Sky_Arm")]        = cfg.sky_temp.arm_value;
    obj_config[F("Sky_Disarm")]     = cfg.sky_temp.disarm_value;
    obj_config[F("Sky_Weight")]     = cfg.sky_temp.weight;
    obj_config[F("Sky_Enabled")]    = cfg.sky_temp.enabled;

    obj_config[F("Humid_Arm")]      = cfg.humidity.arm_value;
    obj_config[F("Humid_Disarm")]   = cfg.humidity.disarm_value;
    obj_config[F("Humid_Weight")]   = cfg.humidity.weight;
    obj_config[F("Humid_Enabled")]  = cfg.humidity.enabled;

    obj_config[F("Dew_Arm")]        = cfg.dew_margin.arm_value;
    obj_config[F("Dew_Disarm")]     = cfg.dew_margin.disarm_value;
    obj_config[F("Dew_Weight")]     = cfg.dew_margin.weight;
    obj_config[F("Dew_Enabled")]    = cfg.dew_margin.enabled;

    obj_config[F("Delay_to_Open")]  = cfg.delay_to_open_s;
    obj_config[F("Delay_to_Close")] = cfg.delay_to_close_s;
    obj_config[F("Trend_Window")]   = cfg.trend_window_s;
    obj_config[F("Trend_Threshold")]= cfg.trend_threshold;

    // State section -- read-only live values (keys prefixed with # are read-only by jsonFormer convention)
    JsonObject obj_state = root.createNestedObject(F("State"));
    obj_state[F("#Ambient_Temperature")]  = _engine._sensors.temperature();
    obj_state[F("#Sky_Temperature")]      = _engine._sensors.skyTemperature();
    obj_state[F("#Humidity")]             = _engine._sensors.humidity();
    obj_state[F("#Pressure")]             = _engine._sensors.pressure();
    obj_state[F("#Time_to_Open")]         = _engine.timeToOpen();
    obj_state[F("#Time_to_Close")]        = _engine.timeToClose();
    obj_state[F("Safety_Monitor_Status")] = _issafe;
    obj_state[F("#Safety_State")]         = SafetyEngine::stateToString(_engine.state());
}
