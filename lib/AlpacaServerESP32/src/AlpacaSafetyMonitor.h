#pragma once
#include "AlpacaDevice.h"

#define ALPACA_SAFETYMONITOR_INTERFACE_VERSION "3"

class AlpacaSafetyMonitor : public AlpacaDevice
{
    protected:
        void aGetInterfaceVersion(AsyncWebServerRequest *request);
        virtual void aGetIsSafe(AsyncWebServerRequest *request) = 0;
        AlpacaSafetyMonitor() {strcpy(_device_type, "safetymonitor");}
    public:
        void registerCallbacks();
};
