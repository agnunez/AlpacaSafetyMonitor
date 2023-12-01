#pragma once
#include <Arduino.h>
#include "config.h"
#include "AlpacaObservingConditions.h"
#include "meteo.h"

class ObservingConditions : public AlpacaObservingConditions {
    private:
        static uint8_t _n_observingconditionss;
        static ObservingConditions *_observingconditions_array[4];
        uint8_t _observingconditions_index;
        float temperature, humidity, pressure, dewpoint, tempsky, noise_db;
        float skybrightness, winddir, windgust, windspeed, timelastupdate, rainrate, cloudcover, skyquality = 0;
        const char * sensordescription = "ESP32 MLX90614 BME280" ; 
        float _avgperiod = 5;
        int  _refresh = 0;
    public:
        ObservingConditions()  : AlpacaObservingConditions() {  _observingconditions_index = _n_observingconditionss++; }
        bool begin();
        void update(Meteo meteo, unsigned long measureDelay);

        // alpaca getters
        void aGetDewPoint(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  dewpoint); }
        void aGetHumidity(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  humidity); }
        void aGetPressure(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  pressure); }
        void aGetSkyBrightness(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  skybrightness); }
        void aGetSkyTemperature(AsyncWebServerRequest *request)  { _alpacaServer->respond(request, tempsky); }
        void aGetSkyQuality(AsyncWebServerRequest *request)  { _alpacaServer->respond(request, skyquality); }
        void aGetStarFwhm(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  noise_db); }
        void aGetTemperature(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  temperature); }
        void aGetWindDirection(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  winddir); }
        void aGetWindGust(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  windgust); }
        void aGetWindSpeed(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  windspeed); }
        void aGetSensorDescription(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  sensordescription); }
        void aGetTimeSinceLastUpdate(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  timelastupdate); }
        void aGetAveragePeriod(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  _avgperiod); }
        void aGetRainRate(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  rainrate); }
        void aGetCloudCover(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  cloudcover); }
    
        // alpaca setters
        void aPutAveragePeriod(AsyncWebServerRequest *request)  { _alpacaServer->getParam(request, "averageperiod", _avgperiod); _alpacaServer->respond(request, nullptr); }
        void aPutRefresh(AsyncWebServerRequest *request)  { _alpacaServer->getParam(request, "refresh", _refresh); _alpacaServer->respond(request, nullptr); }

        // alpaca json
        void aReadJson(JsonObject &root);
        void aWriteJson(JsonObject &root);

};
