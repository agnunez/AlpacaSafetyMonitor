#pragma once
#include "AlpacaServer.h"

// Lambda Handler Function for calling object function
#define LHF(method) [this](){this->method();}

class AlpacaDevice
{
    protected:
        // naming and numbering
        static AlpacaDevice* _devicelist[16];
        static uint8_t _n_devices;
        uint8_t _index;
        char _device_type[16] = "";
        uint8_t _device_number;
        void createCallBack(WebServer::THandlerFunction fn, http_method type, const char command[]);
        void getClientArgs();

        // alpaca commands
        void putAction();
        void putCommandBlind();
        void putCommandBool();
        void putCommandString();
        void getConnected();
        void putConnected();
        void getDescription();
        void getDriverInfo();
        void getDriverVersion();
        void getInterfaceVersion();
        void getName();
        void getSupportedActions();
        AlpacaDevice()
        {
            _index = _n_devices;
            //_devicelist[_n_devices++] = this;
        }
        public:
        void registerCallbacks();
        uint8_t getDeviceNumber() { return _device_number; }
        const char* getDeviceType() { return _device_type; }
        //static AlpacaDevice* getDevice(int index) { return _devicelist[index];}
};