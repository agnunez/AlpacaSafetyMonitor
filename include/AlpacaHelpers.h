#pragma once
#include <Arduino.h>

#define ALPACA_DISCOVERY_HEADER "alpacadiscovery"
#define ALPACA_DISCOVERY_LENGTH 64


// discovery package
typedef struct
{
    private:
        char _header[15];
        char _version;
        char _reserved[48];
    public:
        bool valid() { return strncmp(_header, ALPACA_DISCOVERY_HEADER, 15) == 0; }
        char version() { return _version; }
} AlpacaDiscoveryPacket;
static_assert(sizeof(AlpacaDiscoveryPacket) == ALPACA_DISCOVERY_LENGTH, "Wrong size of struct");

typedef union {
    AlpacaDiscoveryPacket data;
    char buffer[ALPACA_DISCOVERY_LENGTH];
} AlpacaDiscoveryBuffer;

// return

#define ALPACA_DEVICE_COMMAND "/api/v1/%s/%d/%s"
const String alpaca_focuser_uri = F(R"(^/api/v1/focuser/[0-9]/[a-z]+$)");
const String alpaca_http_header = F("HTTP/1.1 200 OK\nContent-Type: application/json\nConnection: close\n");