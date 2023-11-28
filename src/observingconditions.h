#pragma once
#include <Arduino.h>
#include "config.h"
#include "shared_variables.h"
#include "AlpacaObservingConditions.h" 
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>



// mlx static and float variables
#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))          // missing math function sign of number
static float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};  // sky temperature corrections polynomial coefficients
static float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint;
static float rainrate, skybrightness, winddirection, windgust, windspeed = 0;      // not implemented yet
static float starfwhm = 0;  // asociated to turbulence
static float _AveragePeriod = 5.;
static float _Refresh = 5.;


class ObservingConditions : public AlpacaObservingConditions {
    private:
        static uint8_t _n_observingconditionss;
        static ObservingConditions *_observingconditions_array[4];
        uint8_t _observingconditions_index;
        
    public:
        ObservingConditions()  : AlpacaObservingConditions()
        { _observingconditions_index = _n_observingconditionss++; } 
        bool begin();
        void update();

        // alpaca getters
        void aGetAveragePeriod(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  _AveragePeriod); }
        void aGetDewPoint(AsyncWebServerRequest *request)       { _alpacaServer->respond(request,  dewpoint); }
        void aGetHumidity(AsyncWebServerRequest *request)       { _alpacaServer->respond(request,  bme_humidity); }
        void aGetPressure(AsyncWebServerRequest *request)       { _alpacaServer->respond(request,  bme_pressure); }
        void aGetRainRate(AsyncWebServerRequest *request)       { _alpacaServer->respond(request,  rainrate); }
        void aGetSkyBrightness(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  skybrightness); }
        void aGetSkyTemperature(AsyncWebServerRequest *request) { _alpacaServer->respond(request,  tempsky); }
        void aGetStarFwhm(AsyncWebServerRequest *request)       { _alpacaServer->respond(request,  noise_db); }      // starfwhm missing. asociated to turbulence
        void aGetTemperature(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  bme_temperature); } 
        void aGetWindDirection(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  winddirection); } 
        void aGetWindGust(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  windgust); } 
        void aGetWindSpeed(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  windspeed); } 
        void aGetSensorDescription(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  "ESP32 @agnuca"); } 
        void aGetTimeSinceLastUpdate(AsyncWebServerRequest *request)    { _alpacaServer->respond(request,  5); } 

        // alpaca setters
        void aPutAveragePeriod(AsyncWebServerRequest *request)  { _alpacaServer->getParam(request, "AveragePeriod", _AveragePeriod); _alpacaServer->respond(request, nullptr); }
        void aPutRefresh(AsyncWebServerRequest *request)        { _alpacaServer->getParam(request, "Refresh", _Refresh); _alpacaServer->respond(request, nullptr); }

        // alpaca json
        void aReadJson(JsonObject &root);
        void aWriteJson(JsonObject &root);
};