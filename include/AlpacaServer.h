#pragma once
#include <Arduino.h>
#include <AsyncUDP.h>
#include <WebServer.h>
#include <esp_system.h>
#include "AlpacaHelpers.h"
#include "config.h"

class AlpacaServer
{
    private:
        AsyncUDP    _serverUDP;
        WebServer   _serverTCP;
        uint16_t    _portTCP;
        uint16_t    _portUDP;
        int         _serverTransactionID = 0;
        int         _serverID;
        char        _uid[12];
        char        _name[32];

    public:
        AlpacaServer(const char* name);
        void begin(uint16_t udp_port, uint16_t tcp_port);
        void update();
        void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);
        WebServer* getTCPServer() { return &_serverTCP; }
};
