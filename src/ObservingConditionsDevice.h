#pragma once
#include <Arduino.h>
#include "AlpacaObservingConditions.h"
#include "SensorManager.h"

class ObservingConditionsDevice : public AlpacaObservingConditions {
private:
    static uint8_t _n_devices;
    static ObservingConditionsDevice* _device_array[4];
    uint8_t _device_index;

    SensorManager& _sensors;

    // Cached sensor readings (copied each update cycle)
    float _temperature    = 0.0f;
    float _humidity       = 0.0f;
    float _pressure       = 0.0f;
    float _dewpoint       = 0.0f;
    float _skyTemperature = 0.0f;
    float _cloudCover     = 0.0f;
    float _turbulenceDb   = 0.0f;

    // Not measured by this hardware -- always zero
    float _skyBrightness  = 0.0f;
    float _skyQuality     = 0.0f;
    float _rainRate       = 0.0f;
    float _windDir        = 0.0f;
    float _windGust       = 0.0f;
    float _windSpeed      = 0.0f;

    float _avgPeriod      = 5.0f;   // seconds
    int   _refreshPeriod  = 0;      // seconds
    float _timeSinceLastUpdate = 0.0f;

public:
    ObservingConditionsDevice(SensorManager& sensors);

    bool begin();

    // Copy current sensor readings -- call every measurement cycle
    void update(SensorManager& sm);

    // ASCOM Alpaca getters
    void aGetAveragePeriod(AsyncWebServerRequest* request);
    void aGetDewPoint(AsyncWebServerRequest* request);
    void aGetHumidity(AsyncWebServerRequest* request);
    void aGetPressure(AsyncWebServerRequest* request);
    void aGetRainRate(AsyncWebServerRequest* request);
    void aGetSkyBrightness(AsyncWebServerRequest* request);
    void aGetSkyTemperature(AsyncWebServerRequest* request);
    void aGetSkyQuality(AsyncWebServerRequest* request);
    void aGetStarFwhm(AsyncWebServerRequest* request);
    void aGetTemperature(AsyncWebServerRequest* request);
    void aGetWindDirection(AsyncWebServerRequest* request);
    void aGetWindGust(AsyncWebServerRequest* request);
    void aGetWindSpeed(AsyncWebServerRequest* request);
    void aGetSensorDescription(AsyncWebServerRequest* request);
    void aGetTimeSinceLastUpdate(AsyncWebServerRequest* request);
    void aGetCloudCover(AsyncWebServerRequest* request);

    // ASCOM Alpaca setters
    void aPutAveragePeriod(AsyncWebServerRequest* request);
    void aPutRefresh(AsyncWebServerRequest* request);

    // JSON serialization for /setup web page (compatible with jsonFormer)
    void aReadJson(JsonObject& root);
    void aWriteJson(JsonObject& root);
};
