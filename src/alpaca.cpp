#include "alpaca.h"
#include <uri/UriRegex.h>
#include <uri/UriGlob.h>

AsyncUDP  alpacaUDP;
WebServer alpacaTCP;

char uid[12];

uint8_t AlpacaDevice::_n_devices = 0;
uint8_t AlpacaFocuser::_n_focusers = 0;

void AlpacaDevice::createCallBack(void (AlpacaDevice::*method)(), http_method type, const char command[])
{
    char url[64];
    sprintf(url, ALPACA_DEVICE_COMMAND, _device_type, _index, command);
    Serial.print("# Register handler for \"");
    Serial.print(url);
    Serial.print("\" to ");
    Serial.println(command);
    
    //(this->*method)();

    // create lambda function
    alpacaTCP.on(url, type, [this, method]() {
        (this->*method)();
    });
    // alpacaTCP.on(url, type, []() {
    //     alpacaTCP.send(200, "text/plain", "Lambda");
    // });
}

void AlpacaDevice::registerCallbacks()
{
    this->createCallBack(&AlpacaDevice::putAction, HTTP_PUT, "action");
    this->createCallBack(&AlpacaDevice::putCommandBlind, HTTP_PUT, "commandblind");
    this->createCallBack(&AlpacaDevice::putCommandBool, HTTP_PUT, "commandbool");
    this->createCallBack(&AlpacaDevice::putCommandString, HTTP_PUT, "commandstring");
    this->createCallBack(&AlpacaDevice::getConnected, HTTP_GET, "connected");
    this->createCallBack(&AlpacaDevice::putConnected, HTTP_PUT, "connected");
    this->createCallBack(&AlpacaDevice::getDescription, HTTP_GET, "description");
    this->createCallBack(&AlpacaDevice::getDriverInfo, HTTP_GET, "driverinfo");
    this->createCallBack(&AlpacaDevice::getDriverVersion, HTTP_GET, "driverversion");
    this->createCallBack(&AlpacaDevice::getInterfaceVersion, HTTP_GET, "interfaceversion");
    this->createCallBack(&AlpacaDevice::getName, HTTP_GET, "name");
    this->createCallBack(&AlpacaDevice::getSupportedActions, HTTP_GET, "supportedactions");
}

// alpaca commands
void AlpacaDevice::putAction()
{
    alpacaTCP.send(200, "text/plain", "Action");
};
void AlpacaDevice::putCommandBlind()
{
    alpacaTCP.send(200, "text/plain", "CommandBlind");
};
void AlpacaDevice::putCommandBool(){
    alpacaTCP.send(200, "text/plain", "CommandBool");
};
void AlpacaDevice::putCommandString(){
    alpacaTCP.send(200, "text/plain", "CommandString");
};
void AlpacaDevice::getConnected(){
    alpacaTCP.send(200, "text/plain", "Connected");
};
void AlpacaDevice::putConnected(){

};
void AlpacaDevice::getDescription(){
    alpacaTCP.send(200, "text/plain", "Description");
};
void AlpacaDevice::getDriverInfo(){
    alpacaTCP.send(200, "text/plain", "DriverInfo");
};
void AlpacaDevice::getDriverVersion(){
    alpacaTCP.send(200, "text/plain", "DriverVersion");
};
void AlpacaDevice::getInterfaceVersion(){
    alpacaTCP.send(200, "text/plain", "InterfaceVersion");
};
void AlpacaDevice::getName(){
    alpacaTCP.send(200, "text/plain", "astrofocuser[0]");
};
void AlpacaDevice::getSupportedActions(){
    alpacaTCP.send(200, "text/plain", "SupportedActions");
};

// alpaca server stuff

void setup_alpaca(uint16_t udp_port, uint16_t tcp_port)
{
    // setup ports
    Serial.print("# Ascom Alpaca discovery port (UDP): ");
    Serial.println(udp_port);
    alpacaUDP.listen(udp_port);
    alpacaUDP.onPacket(onAlpacaDiscovery);    

    Serial.print("# Ascom Alpaca server port (TCP): ");
    Serial.println(tcp_port);
    alpacaTCP.begin(tcp_port);

    alpacaTCP.onNotFound([]() {
        String url = alpacaTCP.uri();
        alpacaTCP.send(200, "text/plain", "Not found: '" + url + "'");
    });
    // alpacaTCP.on(UriRegex("^\\/api\\/v1\\/focuser\\$"), []() {
    //     String user = alpacaTCP.pathArg(0);
    //     String device = alpacaTCP.pathArg(1);
    //     alpacaTCP.send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'");
    // });
}

void update_alpaca()
{
    alpacaTCP.handleClient();
}

void onAlpacaDiscovery(AsyncUDPPacket& udpPacket)
{
    // check for arrived UDP packet at port
    int length = udpPacket.length();
    if (length == 0)
        return;

    Serial.print(F("# Alpaca Discovery - Remote ip "));
    Serial.println(udpPacket.remoteIP());

    // check size
    if (length != sizeof(AlpacaDiscoveryPacket)) {
        Serial.print(F("# Alpaca Discovery - Wrong packet size "));
        Serial.println(length);
        return;
    }
    
    // check package content
    AlpacaDiscoveryPacket* alpaca_packet = (AlpacaDiscoveryPacket*)udpPacket.data();
    if (alpaca_packet->valid()) {
        Serial.print("# Alpaca Discovery - Header v. ");
        Serial.println(alpaca_packet->version());
    } else {
        Serial.println("# Alpaca Discovery - Header mismatch");
        return;
    }

    // reply port to ascom tcp server
    uint8_t resp_buf[24];
    int resp_len = sprintf((char *)resp_buf, "{\"alpacaport\":%d}", ALPACA_TCP_PORT);
    alpacaUDP.writeTo(resp_buf, resp_len, udpPacket.remoteIP(), ALPACA_UDP_PORT);
}

void genUID()
{
    uint8_t mac_adr[6];
    esp_read_mac(mac_adr, ESP_MAC_WIFI_STA);
    sprintf(uid, "{%X%X%X%X%X%X}", mac_adr[0], mac_adr[1], mac_adr[2],mac_adr[3],mac_adr[4],mac_adr[5]);
}