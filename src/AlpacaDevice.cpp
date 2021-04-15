#include "AlpacaDevice.h"

void AlpacaDevice::createCallBack(WebServer::THandlerFunction fn, http_method type, const char command[], bool devicemethod)
{
    char url[64];
    sprintf(url, ALPACA_DEVICE_COMMAND, _device_type, _device_number, command);
    Serial.print("# Register handler for \"");
    Serial.print(url);
    Serial.print("\" to ");
    Serial.println(command);
    
    // register handler for generated URI
    _alpacaServer->getTCPServer()->on(url, type, fn);

    // add to supported methods if devicemethod is true
    if(devicemethod) {
        int len = strlen(_supported_actions);
        _supported_actions[len-1] = '\0';
        if (len >2)
            strcat(_supported_actions, ", \"");
        else
            strcat(_supported_actions, "\"");
        strcat(_supported_actions, command);
        strcat(_supported_actions, "\"]");
    }
}

void AlpacaDevice::registerCallbacks()
{
    this->createCallBack(LHF(putAction), HTTP_PUT, "action", false);
    this->createCallBack(LHF(putCommandBlind), HTTP_PUT, "commandblind", false);
    this->createCallBack(LHF(putCommandBool), HTTP_PUT, "commandbool", false);
    this->createCallBack(LHF(putCommandString), HTTP_PUT, "commandstring", false);
    this->createCallBack(LHF(getConnected), HTTP_GET, "connected", false);
    this->createCallBack(LHF(putConnected), HTTP_PUT, "connected", false);
    this->createCallBack(LHF(getDescription), HTTP_GET, "description", false);
    this->createCallBack(LHF(getDriverInfo), HTTP_GET, "driverinfo", false);
    this->createCallBack(LHF(getDriverVersion), HTTP_GET, "driverversion", false);
    this->createCallBack(LHF(getInterfaceVersion), HTTP_GET, "interfaceversion", false);
    this->createCallBack(LHF(getName), HTTP_GET, "name", false);
    this->createCallBack(LHF(getSupportedActions), HTTP_GET, "supportedactions", false);
}

const char*  AlpacaDevice::getDeviceName() {
    if(strcmp(_device_name, "") == 0) {
        sprintf(_device_name, ALPACA_DEFAULT_NAME, _device_type, _device_number);
    }
    return _device_name;
}
const char*  AlpacaDevice::getDeviceUID() {
    if(strcmp(_device_uid, "") == 0) {
        sprintf(_device_uid, ALPACA_UNIQUE_NAME, _device_type, _alpacaServer->getUID(), _device_number);
    }
    return _device_uid;
}

// alpaca commands
void AlpacaDevice::putAction()
{
    _alpacaServer->respond(nullptr, "", NotImplemented);
};
void AlpacaDevice::putCommandBlind()
{
    _alpacaServer->respond(nullptr, "", NotImplemented);
};
void AlpacaDevice::putCommandBool(){
    _alpacaServer->respond(nullptr, "", NotImplemented);
};
void AlpacaDevice::putCommandString(){
    _alpacaServer->respond(nullptr, "", NotImplemented);
};
void AlpacaDevice::getConnected(){
    _alpacaServer->respond("1", nullptr);
};
void AlpacaDevice::putConnected(){
    _alpacaServer->respond(nullptr, nullptr);
};
void AlpacaDevice::getDescription(){
    _alpacaServer->respond(ALPACA_DRIVER_DESC, nullptr);
};
void AlpacaDevice::getDriverInfo(){
    _alpacaServer->respond(ALPACA_DRIVER_INFO, nullptr);
};
void AlpacaDevice::getDriverVersion(){
    _alpacaServer->respond(ALPACA_DRIVER_VER, nullptr);
};
void AlpacaDevice::getInterfaceVersion(){
    _alpacaServer->respond(ALPACA_INTERFACE_VERSION, nullptr);
};
void AlpacaDevice::getName(){
    _alpacaServer->respond(getDeviceName(), nullptr);
};
void AlpacaDevice::getSupportedActions(){
    _alpacaServer->respond(_supported_actions, nullptr);
};
