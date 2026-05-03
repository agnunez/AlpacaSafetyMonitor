#pragma once
#include "AlpacaServer.h"

class AlpacaDevice
{
    protected:
        AlpacaServer* _alpacaServer;
        char _device_name[33] = "";
        char _device_type[30] = "";
        char _device_uid[32] = "";
        char _device_desc[65] = "Alpaca ESP32 driver";
        char _device_url[65] = "";
        char _supported_actions[512] = "[]";
        int8_t _device_number = -1;
        bool _isconnected = false;

        virtual void _setSetupPage();
        void _getJsondata(AsyncWebServerRequest *request);
        void _putJsondata(AsyncWebServerRequest *request);
        void createCallBack(ArRequestHandlerFunction fn, WebRequestMethodComposite type, const char command[], bool deviceMethod=true);

        virtual void aPutAction(AsyncWebServerRequest *request);
        virtual void aPutCommandBlind(AsyncWebServerRequest *request);
        virtual void aPutCommandBool(AsyncWebServerRequest *request);
        virtual void aPutCommandString(AsyncWebServerRequest *request);
        virtual void aGetConnected(AsyncWebServerRequest *request);
        virtual void aPutConnected(AsyncWebServerRequest *request);
        void aGetDescription(AsyncWebServerRequest *request);
        void aGetDriverInfo(AsyncWebServerRequest *request);
        void aGetDriverVersion(AsyncWebServerRequest *request);
        virtual void aGetInterfaceVersion(AsyncWebServerRequest *request)=0;
        void aGetName(AsyncWebServerRequest *request);
        void aGetSupportedActions(AsyncWebServerRequest *request);
    public:
        void virtual registerCallbacks();
        void setAlpacaServer(AlpacaServer *alpaca_server) { _alpacaServer = alpaca_server; }
        void setDeviceNumber( int8_t device_number);
        uint8_t getDeviceNumber() { return _device_number; }
        const char* getDeviceType() { return _device_type; }
        const char* getDeviceName() { return _device_name; };
        const char* getDeviceUID() { return _device_uid; }
        const char* getDeviceURL() { return _device_url; };
        virtual void aReadJson(JsonObject &root);
        virtual void aWriteJson(JsonObject &root);
};
