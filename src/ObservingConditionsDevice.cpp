#include "ObservingConditionsDevice.h"

// Static array pattern -- keeps up to 4 device instances (matches original project convention)
uint8_t ObservingConditionsDevice::_n_devices = 0;
ObservingConditionsDevice* ObservingConditionsDevice::_device_array[4] = { nullptr, nullptr, nullptr, nullptr };

// ---------------------------------------------------------------------------
ObservingConditionsDevice::ObservingConditionsDevice(SensorManager& sensors)
    : AlpacaObservingConditions()
    , _sensors(sensors)
{
    _device_index = _n_devices++;
}

// ---------------------------------------------------------------------------
bool ObservingConditionsDevice::begin()
{
    _device_array[_device_index] = this;
    return true;
}

// ---------------------------------------------------------------------------
// Copy current sensor readings from SensorManager
// ---------------------------------------------------------------------------
void ObservingConditionsDevice::update(SensorManager& sm)
{
    _temperature    = sm.temperature();
    _humidity       = sm.humidity();
    _pressure       = sm.pressure();
    _dewpoint       = sm.dewpoint();
    _skyTemperature = sm.skyTemperature();
    _cloudCover     = sm.cloudCover();
    _turbulenceDb   = sm.turbulenceDb();

    _timeSinceLastUpdate = 0.0f;  // reset on each fresh reading
}

// ---------------------------------------------------------------------------
// ASCOM Alpaca getters
// ---------------------------------------------------------------------------
void ObservingConditionsDevice::aGetAveragePeriod(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _avgPeriod);
}

void ObservingConditionsDevice::aGetDewPoint(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _dewpoint);
}

void ObservingConditionsDevice::aGetHumidity(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _humidity);
}

void ObservingConditionsDevice::aGetPressure(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _pressure);
}

void ObservingConditionsDevice::aGetRainRate(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _rainRate);
}

void ObservingConditionsDevice::aGetSkyBrightness(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _skyBrightness);
}

void ObservingConditionsDevice::aGetSkyTemperature(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _skyTemperature);
}

void ObservingConditionsDevice::aGetSkyQuality(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _skyQuality);
}

void ObservingConditionsDevice::aGetStarFwhm(AsyncWebServerRequest* request)
{
    // Repurposed as turbulence dB (same convention as original project)
    _alpacaServer->respond(request, _turbulenceDb);
}

void ObservingConditionsDevice::aGetTemperature(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _temperature);
}

void ObservingConditionsDevice::aGetWindDirection(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _windDir);
}

void ObservingConditionsDevice::aGetWindGust(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _windGust);
}

void ObservingConditionsDevice::aGetWindSpeed(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _windSpeed);
}

void ObservingConditionsDevice::aGetCloudCover(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _cloudCover);
}

void ObservingConditionsDevice::aGetSensorDescription(AsyncWebServerRequest* request)
{
    // Build description string including sensor health status
    char desc[128];
    const char* bmeStatus  = _sensors.bme280()   ? healthToString(_sensors.bme280()->health())   : "N/A";
    const char* mlxStatus  = _sensors.mlx90614()  ? healthToString(_sensors.mlx90614()->health()) : "N/A";
    snprintf(desc, sizeof(desc), "ESP32 MLX90614 BME280 [BME280:%s MLX90614:%s]", bmeStatus, mlxStatus);
    _alpacaServer->respond(request, desc);
}

void ObservingConditionsDevice::aGetTimeSinceLastUpdate(AsyncWebServerRequest* request)
{
    _alpacaServer->respond(request, _timeSinceLastUpdate);
}

// ---------------------------------------------------------------------------
// ASCOM Alpaca setters
// ---------------------------------------------------------------------------
void ObservingConditionsDevice::aPutAveragePeriod(AsyncWebServerRequest* request)
{
    _alpacaServer->getParam(request, "averageperiod", _avgPeriod);
    _alpacaServer->respond(request, nullptr);
}

void ObservingConditionsDevice::aPutRefresh(AsyncWebServerRequest* request)
{
    _alpacaServer->getParam(request, "refresh", _refreshPeriod);
    _alpacaServer->respond(request, nullptr);
}

// ---------------------------------------------------------------------------
// JSON read -- deserialize configuration from web UI
// ---------------------------------------------------------------------------
void ObservingConditionsDevice::aReadJson(JsonObject& root)
{
    AlpacaObservingConditions::aReadJson(root);

    if (JsonObject obj_config = root[F("Configuration")]) {
        _avgPeriod     = obj_config[F("Average_Period")]  | _avgPeriod;
        _refreshPeriod = obj_config[F("Refresh_Period")]  | _refreshPeriod;
    }
}

// ---------------------------------------------------------------------------
// JSON write -- serialize current config + state for the /setup web page
// ---------------------------------------------------------------------------
void ObservingConditionsDevice::aWriteJson(JsonObject& root)
{
    AlpacaObservingConditions::aWriteJson(root);

    // Configuration section
    JsonObject obj_config = root.createNestedObject(F("Configuration"));
    obj_config[F("Average_Period")]  = _avgPeriod;
    obj_config[F("Refresh_Period")]  = _refreshPeriod;

    // State section -- all read-only sensor values plus health status
    JsonObject obj_state = root.createNestedObject(F("State"));
    obj_state[F("#Ambient_Temperature")]  = _temperature;
    obj_state[F("#Sky_Temperature")]      = _skyTemperature;
    obj_state[F("#Humidity")]             = _humidity;
    obj_state[F("#Pressure")]             = _pressure;
    obj_state[F("#Dewpoint")]             = _dewpoint;
    obj_state[F("#Cloud_Cover")]          = _cloudCover;
    obj_state[F("#Turbulence_dB")]        = _turbulenceDb;
    obj_state[F("#Sensor_Description")]   = "ESP32 MLX90614 BME280";

    // Sensor health status strings
    obj_state[F("#BME280_Status")]   = _sensors.bme280()   ? healthToString(_sensors.bme280()->health())   : "N/A";
    obj_state[F("#MLX90614_Status")] = _sensors.mlx90614()  ? healthToString(_sensors.mlx90614()->health()) : "N/A";
}
