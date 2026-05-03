#include "AlpacaDevice.h"

#define DEBUGSTREAM if(_alpacaServer->debug) _alpacaServer->debugstream

void AlpacaDevice::createCallBack(ArRequestHandlerFunction fn, WebRequestMethodComposite type, const char command[], bool devicemethod)
{
    char url[64];
    snprintf(url, sizeof(url), ALPACA_DEVICE_COMMAND, _device_type, _device_number, command);
    DEBUGSTREAM->print("# Register handler for \"");
    DEBUGSTREAM->print(url);
    DEBUGSTREAM->print("\" to ");
    DEBUGSTREAM->println(command);

    _alpacaServer->getServerTCP()->on(url, type, fn);

    if(devicemethod) {
        int len = strlen(_supported_actions);
        _supported_actions[len-1] = '\0';
        if (len > 2)
            strcat(_supported_actions, ", \"");
        else
            strcat(_supported_actions, "\"");
        strcat(_supported_actions, command);
        strcat(_supported_actions, "\"]");
    }
}

void AlpacaDevice::_setSetupPage()
{
    char url[64];
    snprintf(url, sizeof(url), ALPACA_DEVICE_COMMAND, _device_type, _device_number, "jsondata");
    this->createCallBack(LHF(_getJsondata), HTTP_GET, "jsondata", false);
    AsyncCallbackJsonWebHandler* jsonhandler = new AsyncCallbackJsonWebHandler(url, [this](AsyncWebServerRequest *request, JsonVariant& json) {
       JsonObject jsonObj = json.as<JsonObject>();
       this->aReadJson(jsonObj);
       request->send(200, F("application/json"), F("{\"received\":\"true\"}"));
    });
    _alpacaServer->getServerTCP()->addHandler(jsonhandler);

    DEBUGSTREAM->print("# Register handler for \"");
    DEBUGSTREAM->print(_device_url);
    DEBUGSTREAM->print("\" to ");
    DEBUGSTREAM->println(F("/www/setup.html"));
    _alpacaServer->getServerTCP()->serveStatic(_device_url, SPIFFS, "/www/setup.html");
}

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
    _setSetupPage();
}

void AlpacaDevice::setDeviceNumber(int8_t device_number) {
    _device_number = device_number;
    snprintf(_device_url, sizeof(_device_url), ALPACA_DEVICE_COMMAND, _device_type, _device_number, "setup");
    snprintf(_device_name, sizeof(_device_name), ALPACA_DEFAULT_NAME, _device_type, _device_number);
    snprintf(_device_uid, sizeof(_device_uid), ALPACA_UNIQUE_NAME, _device_type, _alpacaServer->getUID(), _device_number);
}

void AlpacaDevice::aPutAction(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, nullptr, NotImplemented);
}
void AlpacaDevice::aPutCommandBlind(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, nullptr, NotImplemented);
}
void AlpacaDevice::aPutCommandBool(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, nullptr, NotImplemented);
}
void AlpacaDevice::aPutCommandString(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, nullptr, NotImplemented);
}
void AlpacaDevice::aGetConnected(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, (_isconnected ? "true" : "false"));
}
void AlpacaDevice::aPutConnected(AsyncWebServerRequest *request) {
    _isconnected = !_isconnected;
    _alpacaServer->respond(request, (_isconnected ? "true" : "false"));
}
void AlpacaDevice::aGetDescription(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, _device_desc);
}
void AlpacaDevice::aGetDriverInfo(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, ALPACA_DRIVER_INFO);
}
void AlpacaDevice::aGetDriverVersion(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, ALPACA_DRIVER_VER);
}
void AlpacaDevice::aGetName(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, getDeviceName());
}
void AlpacaDevice::aGetSupportedActions(AsyncWebServerRequest *request) {
    _alpacaServer->respond(request, _supported_actions);
}

void AlpacaDevice::aReadJson(JsonObject &root)
{
    const char* name = root[F("General")][F("Name")];
    if(name)
        strlcpy(_device_name, name, sizeof(_device_name));
    const char* desc = root[F("General")][F("Description")];
    if(desc)
        strlcpy(_device_desc, desc, sizeof(_device_desc));
}

void AlpacaDevice::aWriteJson(JsonObject &root)
{
    JsonObject obj_general = root.createNestedObject(F("General"));
    obj_general[F("Name")] = getDeviceName();
    obj_general[F("Description")] = _device_desc;
    obj_general[F("UID")] = _device_uid;
}

void AlpacaDevice::_getJsondata(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    aWriteJson(root);
    String ser_json;
    serializeJson(root, ser_json);
    request->send(200, ALPACA_JSON_TYPE, ser_json);
}
