#pragma once
#include <Arduino.h>

// settings
#define ALPACA_MAX_DEVICES 8

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
#define ALPACA_API_VERSIONS "[1]"
#define ALPACA_DRIVER_VER "v2.0.0"
#define ALPACA_DRIVER_DESC "ESP32 Ascom Alpaca driver"
#define ALPACA_DRIVER_INFO "ESP32 Ascom Alpaca driver by Njaal Brekke & @agnuca"
#define ALPACA_DESCRIPTION "{\"ServerName\":\"ESP32 Ascom Alpaca Server\",\"Authors\":\"Njaal Brekke & @agnuca\",\"ServerVersion\":\"v2.0\"}"
#define ALPACA_DEFAULT_NAME "%s-%i"
#define ALPACA_UNIQUE_NAME "%s#%s%02X"
#define ALPACA_JSON_TYPE "application/json"
#define ALPACA_DEVICE_COMMAND "/api/v1/%s/%d/%s"
#define ALPACA_DEVICE_LIST "{\"DeviceName\":\"%s\",\"DeviceType\":\"%s\",\"DeviceNumber\":%i,\"UniqueID\":\"%s\"}"
#define ALPACA_RESPOSE_ERROR "{\n\t\"ClientTransactionID\": %i,\n\t\"ServerTransactionID\": %i,\n\t\"ErrorNumber\": %i,\n\t\"ErrorMessage\": \"%s\"\n}"
#define ALPACA_RESPOSE_VALUE_ERROR "{\n\t\"Value\": %s,\n\t\"ClientTransactionID\": %i,\n\t\"ServerTransactionID\": %i,\n\t\"ErrorNumber\": %i,\n\t\"ErrorMessage\": \"%s\"\n}"
#define ALPACA_RESPOSE_VALUE_ERROR_STR "{\n\t\"Value\": \"%s\",\n\t\"ClientTransactionID\": %i,\n\t\"ServerTransactionID\": %i,\n\t\"ErrorNumber\": %i,\n\t\"ErrorMessage\": \"%s\"\n}"


enum AscomErrorCode : int64_t {
    ActionNotImplementedException = 0x8004040C,
    DriverBase = 0x80040500,
    DriverMax = 0x80040FFF,
    InvalidOperationException = 0x8004040B,
    InvalidValue = 0x80040401,
    InvalidWhileParked = 0x80040408,
    InvalidWhileSlaved = 0x80040409,
    NotConnected = 0x80040407,
    NotImplemented = 0x80040400,
    NotInCacheException = 0x8004040D,
    SettingsProviderError = 0x8004040A,
    UnspecifiedError = 0x800404FF,
    ValueNotSet = 0x80040402
};
