#pragma once
#include <Arduino.h>
#include <AsyncUDP.h>
#include <WebServer.h>
#include <esp_system.h>
#include "AlpacaHelpers.h"
#include "config.h"

// Lambda Handler Function for calling object function
#define LHF(method) [this](){this->method();}

// Forward declare AlpacaDevice to avoid circular includes
class AlpacaDevice;

class AlpacaServer
{
    private:
        AsyncUDP        _serverUDP;
        WebServer       _serverTCP;
        uint16_t        _portTCP;
        uint16_t        _portUDP;
        int             _clientID;
        int             _clientTransactionID;
        int             _serverTransactionID = 0;
        int             _serverID;
        char            _uid[13];
        char            _name[32];
        AlpacaDevice*   _device[ALPACA_MAX_DEVICES];
        int             _n_devices = 0;

        void _registerCallbacks();
        void _getApiVersions();
        void _getDescription();
        void _getConfiguredDevices();
        void _getTransactionData();
        int _paramIndex(const char* name);

    public:
        bool            debug = false;
        Stream*         debugstream = &Serial;

        AlpacaServer(const char* name);
        void begin(uint16_t udp_port, uint16_t tcp_port);
        void update();
        void addDevice(AlpacaDevice *device);
        bool getParam(const char* name, bool &value);
        bool getParam(const char* name, float &value);
        bool getParam(const char* name, int &value);
        bool getParam(const char* name, char *buffer, int buffer_size);
        void respond(bool value, int32_t error_number=0, const char* error_message="");
        void respond(int value, int32_t error_number=0, const char* error_message="");
        void respond(float value, int32_t error_number=0, const char* error_message="");
        void respond(const char* value, int32_t error_number=0, const char* error_message="");
        void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);
        const char* getUID() { return _uid; }
        WebServer* getTCPServer() { return &_serverTCP; }
};
