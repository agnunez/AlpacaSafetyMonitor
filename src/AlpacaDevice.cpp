#include "AlpacaDevice.h"

#define DEBUGSTREAM if(_alpacaServer->debug) _alpacaServer->debugstream

// create url and register callback for REST API
void AlpacaDevice::createCallBack(WebServer::THandlerFunction fn, http_method type, const char command[], bool devicemethod)
{
    char url[64];
    sprintf(url, ALPACA_DEVICE_COMMAND, _device_type, _device_number, command);
    DEBUGSTREAM->print("# Register handler for \"");
    DEBUGSTREAM->print(url);
    DEBUGSTREAM->print("\" to ");
    DEBUGSTREAM->println(command);
    
    // register handler for generated URI
    _alpacaServer->getTCPServer()->on(url, type, fn);

    // add command to supported methods if devicemethod is true
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

// register callbacks for REST API
void AlpacaDevice::registerCallbacks()
{
    this->createCallBack(LHF(aPutAction), HTTP_PUT, "action", false);
    this->createCallBack(LHF(aPutCommandBlind), HTTP_PUT, "commandblind", false);
    this->createCallBack(LHF(aPutCommandBool), HTTP_PUT, "commandbool", false);
    this->createCallBack(LHF(aPutCommandString), HTTP_PUT, "commandstring", false);
    this->createCallBack(LHF(aGetConnected), HTTP_GET, "connected", false);
    this->createCallBack(LHF(aPutConnected), HTTP_PUT, "connected", false);
    this->createCallBack(LHF(aGetDescription), HTTP_GET, "description", false);
    this->createCallBack(LHF(aGetDriverInfo), HTTP_GET, "driverinfo", false);
    this->createCallBack(LHF(aGetDriverVersion), HTTP_GET, "driverversion", false);
    this->createCallBack(LHF(aGetInterfaceVersion), HTTP_GET, "interfaceversion", false);
    this->createCallBack(LHF(aGetName), HTTP_GET, "name", false);
    this->createCallBack(LHF(aGetSupportedActions), HTTP_GET, "supportedactions", false);
}

// alpaca commands
void AlpacaDevice::aPutAction()
{
    _alpacaServer->respond(nullptr, NotImplemented);
};
void AlpacaDevice::aPutCommandBlind()
{
    _alpacaServer->respond(nullptr, NotImplemented);
};
void AlpacaDevice::aPutCommandBool(){
    _alpacaServer->respond(nullptr, NotImplemented);
};
void AlpacaDevice::aPutCommandString(){
    _alpacaServer->respond(nullptr, NotImplemented);
};
void AlpacaDevice::aGetConnected(){
    _alpacaServer->respond("1");
};
void AlpacaDevice::aPutConnected(){
    _alpacaServer->respond(nullptr);
};
void AlpacaDevice::aGetDescription(){
    _alpacaServer->respond(_device_desc);
};
void AlpacaDevice::aGetDriverInfo(){
    _alpacaServer->respond(ALPACA_DRIVER_INFO);
};
void AlpacaDevice::aGetDriverVersion(){
    _alpacaServer->respond(ALPACA_DRIVER_VER);
};
void AlpacaDevice::aGetName(){
    _alpacaServer->respond(getDeviceName());
};
void AlpacaDevice::aGetSupportedActions(){
    _alpacaServer->respond(_supported_actions);
};
