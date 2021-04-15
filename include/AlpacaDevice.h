#pragma once
#include "AlpacaServer.h"

class AlpacaDevice
{
    protected:
        // pointer to server
        AlpacaServer* _alpacaServer;
        // naming and numbering
        char _device_name[33] = "";
        char _device_type[17] = "";
        char _device_uid[17] = "";
        char _supported_actions[512] = "[]";
        int8_t _device_number = -1;

        // common functions
        void createCallBack(WebServer::THandlerFunction fn, http_method type, const char command[], bool deviceMethod=true);
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
    public:
        void virtual registerCallbacks();
        void setAlpacaServer(AlpacaServer *alpaca_server) { _alpacaServer = alpaca_server; }
        void setDeviceNumber( int8_t device_number) { _device_number = device_number; }
        uint8_t getDeviceNumber() { return _device_number; }
        const char* getDeviceType() { return _device_type; }
        const char* getDeviceName();
        const char* getDeviceUID();
        //static AlpacaDevice* getDevice(int index) { return _devicelist[index];}
};