#pragma once
#include "AlpacaDevice.h"

#define ALPACA_DOME_INTERFACE_VERSION "3"

class AlpacaDome : public AlpacaDevice
{
    protected:
        // Capabilities
        virtual void aGetCanSetShutter(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanSetAzimuth(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanSetAltitude(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanSetPark(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanPark(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanFindHome(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanSlave(AsyncWebServerRequest *request) = 0;
        virtual void aGetCanSyncAzimuth(AsyncWebServerRequest *request) = 0;

        // Status
        virtual void aGetSlewing(AsyncWebServerRequest *request) = 0;
        virtual void aGetSlaved(AsyncWebServerRequest *request) = 0;
        virtual void aPutSlaved(AsyncWebServerRequest *request) = 0;
        virtual void aGetShutterStatus(AsyncWebServerRequest *request) = 0;
        virtual void aGetAzimuth(AsyncWebServerRequest *request) = 0;
        virtual void aGetAltitude(AsyncWebServerRequest *request) = 0;
        virtual void aGetAtHome(AsyncWebServerRequest *request) = 0;
        virtual void aGetAtPark(AsyncWebServerRequest *request) = 0;

        // Commands
        virtual void aPutOpenShutter(AsyncWebServerRequest *request) = 0;
        virtual void aPutCloseShutter(AsyncWebServerRequest *request) = 0;
        virtual void aPutAbortSlew(AsyncWebServerRequest *request) = 0;
        virtual void aPutSlewToAzimuth(AsyncWebServerRequest *request) = 0;
        virtual void aPutSlewToAltitude(AsyncWebServerRequest *request) = 0;
        virtual void aPutPark(AsyncWebServerRequest *request) = 0;
        virtual void aPutSetPark(AsyncWebServerRequest *request) = 0;
        virtual void aPutFindHome(AsyncWebServerRequest *request) = 0;
        virtual void aPutSyncToAzimuth(AsyncWebServerRequest *request) = 0;

        void aGetInterfaceVersion(AsyncWebServerRequest *request);
        AlpacaDome() { strcpy(_device_type, "dome"); }
    public:
        void registerCallbacks();
};
