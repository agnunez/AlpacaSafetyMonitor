#pragma once
#include <Arduino.h>
#include <AsyncUDP.h>
#include <WebServer.h>
#include <esp_system.h>
#include "alpaca_defines.h"
#include "config.h"

//void onAlpacaDiscovery(AsyncUDPPacket& packet);

class AlpacaManagement
{

};

class AlpacaDevice
{
    protected:
        // naming and numbering
        static AlpacaDevice* _devicelist[16];
        static uint8_t _n_devices;
        uint8_t _index;
        char _device_type[16] = "";
        uint8_t _device_number;
        void createCallBack(void (AlpacaDevice::*method)(), http_method type, const char command[]);

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
        AlpacaDevice()
        {
            _index = _n_devices;
            //_devicelist[_n_devices++] = this;
        }
        void registerCallbacks();
        uint8_t getDeviceNumber() { return _device_number; }
        const char* getDeviceType() { return _device_type; }
        //static AlpacaDevice* getDevice(int index) { return _devicelist[index];}
};

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
};

void genUID();
void setup_alpaca(uint16_t udp_port, uint16_t tcp_port);
void update_alpaca();
void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);