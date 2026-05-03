#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include <esp_system.h>
#include <AsyncUDP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#include "AlpacaHelpers.h"

// Lambda Handler Function for calling object function
#define LHF(method) (ArRequestHandlerFunction)[this](AsyncWebServerRequest *request){this->method(request);}

// Forward declare AlpacaDevice to avoid circular includes
class AlpacaDevice;

class AlpacaServer
{
    private:
        AsyncWebServer* _serverTCP;
        AsyncUDP        _serverUDP;
        uint16_t        _portTCP;
        uint16_t        _portUDP;
        volatile int    _serverTransactionID = 0;
        int             _serverID;
        char            _uid[13];
        char            _name[32];
        AlpacaDevice*   _device[ALPACA_MAX_DEVICES];
        int             _n_devices = 0;

        void _registerCallbacks();
        void _getApiVersions(AsyncWebServerRequest *request);
        void _getDescription(AsyncWebServerRequest *request);
        void _getConfiguredDevices(AsyncWebServerRequest *request);
        int  _paramIndex(AsyncWebServerRequest *request, const char* name);
        void _readJson(JsonObject &root);
        void _writeJson(JsonObject &root);
        void _getJsondata(AsyncWebServerRequest *request);
        void _getLinks(AsyncWebServerRequest *request);

    public:
        bool            debug = true;
        Stream*         debugstream = &Serial;

        AlpacaServer(const char* name);
        void begin(uint16_t udp_port, uint16_t tcp_port);
        void addDevice(AlpacaDevice *device);
        bool getParam(AsyncWebServerRequest *request, const char* name, bool &value);
        bool getParam(AsyncWebServerRequest *request, const char* name, float &value);
        bool getParam(AsyncWebServerRequest *request, const char* name, int &value);
        bool getParam(AsyncWebServerRequest *request, const char* name, char *buffer, int buffer_size);
        void respond(AsyncWebServerRequest *request, bool value, int32_t error_number=0, const char* error_message="");
        void respond(AsyncWebServerRequest *request, int value, int32_t error_number=0, const char* error_message="");
        void respond(AsyncWebServerRequest *request, float value, int32_t error_number=0, const char* error_message="");
        void respond(AsyncWebServerRequest *request, const char* value, int32_t error_number=0, const char* error_message="");
        bool loadSettings();
        bool saveSettings();
        void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);
        AsyncWebServer* getServerTCP() { return _serverTCP; }
        const char* getUID() { return _uid; }
};
