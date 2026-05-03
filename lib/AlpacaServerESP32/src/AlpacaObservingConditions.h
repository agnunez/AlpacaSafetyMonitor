#pragma once
#include "AlpacaDevice.h"

#define ALPACA_OBSERVINGCONDITIONS_INTERFACE_VERSION "3"

class AlpacaObservingConditions : public AlpacaDevice
{
    protected:
        void aGetInterfaceVersion(AsyncWebServerRequest *request);
        virtual void aGetAveragePeriod(AsyncWebServerRequest *request) = 0;
        virtual void aPutAveragePeriod(AsyncWebServerRequest *request) = 0;
        virtual void aGetDewPoint(AsyncWebServerRequest *request) = 0;
        virtual void aGetHumidity(AsyncWebServerRequest *request) = 0;
        virtual void aGetPressure(AsyncWebServerRequest *request) = 0;
        virtual void aGetRainRate(AsyncWebServerRequest *request) = 0;
        virtual void aGetSkyBrightness(AsyncWebServerRequest *request) = 0;
        virtual void aGetSkyTemperature(AsyncWebServerRequest *request) = 0;
        virtual void aGetSkyQuality(AsyncWebServerRequest *request) = 0;
        virtual void aGetStarFwhm(AsyncWebServerRequest *request) = 0;
        virtual void aGetTemperature(AsyncWebServerRequest *request) = 0;
        virtual void aGetWindDirection(AsyncWebServerRequest *request) = 0;
        virtual void aGetWindGust(AsyncWebServerRequest *request) = 0;
        virtual void aGetWindSpeed(AsyncWebServerRequest *request) = 0;
        virtual void aPutRefresh(AsyncWebServerRequest *request) = 0;
        virtual void aGetSensorDescription(AsyncWebServerRequest *request) = 0;
        virtual void aGetTimeSinceLastUpdate(AsyncWebServerRequest *request) = 0;
        virtual void aGetCloudCover(AsyncWebServerRequest *request) = 0;
        AlpacaObservingConditions() {strcpy(_device_type, "observingconditions");}
    public:
        void registerCallbacks();
};
