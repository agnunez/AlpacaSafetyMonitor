#include "AlpacaDevice.h"

extern WebServer alpacaTCP;
extern int alpacaServerTransactionID;

uint8_t AlpacaDevice::_n_devices = 0;
void AlpacaDevice::createCallBack(WebServer::THandlerFunction fn, http_method type, const char command[])
{
    char url[64];
    sprintf(url, ALPACA_DEVICE_COMMAND, _device_type, _index, command);
    Serial.print("# Register handler for \"");
    Serial.print(url);
    Serial.print("\" to ");
    Serial.println(command);
    // register handler for generated URI
    alpacaTCP.on(url, type, fn);
}

void AlpacaDevice::registerCallbacks()
{
    this->createCallBack(LHF(putAction), HTTP_PUT, "action");
    this->createCallBack(LHF(putCommandBlind), HTTP_PUT, "commandblind");
    this->createCallBack(LHF(putCommandBool), HTTP_PUT, "commandbool");
    this->createCallBack(LHF(putCommandString), HTTP_PUT, "commandstring");
    this->createCallBack(LHF(getConnected), HTTP_GET, "connected");
    this->createCallBack(LHF(putConnected), HTTP_PUT, "connected");
    this->createCallBack(LHF(getDescription), HTTP_GET, "description");
    this->createCallBack(LHF(getDriverInfo), HTTP_GET, "driverinfo");
    this->createCallBack(LHF(getDriverVersion), HTTP_GET, "driverversion");
    this->createCallBack(LHF(getInterfaceVersion), HTTP_GET, "interfaceversion");
    this->createCallBack(LHF(getName), HTTP_GET, "name");
    this->createCallBack(LHF(getSupportedActions), HTTP_GET, "supportedactions");
}

void AlpacaDevice::getClientArgs() {
    String str;
    str = alpacaTCP.arg("clientid");
    if (str.length() > 0) {
        int clientid = str.toInt();
    }
    str = alpacaTCP.arg("clienttransactionid");
    if (str.length() > 0) {
        int transactionid = str.toInt();
    }
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
