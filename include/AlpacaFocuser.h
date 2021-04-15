#pragma once
#include "AlpacaDevice.h"

#define ALPACA_FOCUSER_INTERFACE_VERSION "3"

class AlpacaFocuser : public AlpacaDevice
{
    protected:
        // alpaca commands
        void aGetInterfaceVersion();
        virtual void aGetAbsolute() = 0;
        virtual void aGetIsMoving() = 0;
        virtual void aGetMaxIncrement() = 0;
        virtual void aGetMaxStep() = 0;
        virtual void aGetPosition() = 0;
        virtual void aGetStepSize() = 0;
        virtual void aGetTempComp() = 0;
        virtual void aPutTempComp() = 0;
        virtual void aGetTempCompAvailable() = 0;
        virtual void aGetTemperature() = 0;
        virtual void aPutHalt() = 0;
        virtual void aPutMove() = 0;
        AlpacaFocuser() {strcpy(_device_type, "focuser");}
    public:
        void registerCallbacks();
};