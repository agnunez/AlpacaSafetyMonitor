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

        void registerCallbacks();
        void getApiVersions();
        void getDescription();
        void getConfiguredDevices();
        void getTransactionData();

    public:
        AlpacaServer(const char* name);
        void begin(uint16_t udp_port, uint16_t tcp_port);
        void update();
        void addDevice(AlpacaDevice *device);
        void respond(bool value, const char* error_message, int32_t error_number=0);
        void respond(int32_t value, const char* error_message, int32_t error_number=0);
        void respond(uint32_t value, const char* error_message, int32_t error_number=0);
        void respond(float value, const char* error_message, int32_t error_number=0);
        void respond(const char* value, const char* error_message, int32_t error_number=0);
        void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);
        const char* getUID() { return _uid; }
        WebServer* getTCPServer() { return &_serverTCP; }
};
