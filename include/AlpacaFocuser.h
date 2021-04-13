#pragma once
#include "AlpacaDevice.h"

class AlpacaFocuser : public AlpacaDevice
{
    protected:
        // naming and numbering
        static uint8_t _n_focusers;   

        // alpaca commands
        void getAbsolute();
        void getIsMoving();
        void getMaxIncrement();
        void getMaxStep();
        void getPosition();
        void getStepSize();
        void getTempComp();
        void putTempComp();
        void getTempCompAvailable();
        void getTemperature();
        void putHalt();
        void putMove();
        AlpacaFocuser() : AlpacaDevice()
        {
            strcpy(_device_type, "focuser");
            _device_number = _n_focusers++;
        }
    public:
        void registerCallbacks();
};