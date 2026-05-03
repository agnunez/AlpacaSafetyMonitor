#include "AlpacaServer.h"
#include "AlpacaDevice.h"

#define DEBUGSTREAM if(debug) debugstream

#define SETTINGS_FILE "/settings.json"

AlpacaServer::AlpacaServer(const char* name)
{
    uint8_t mac_adr[6];
    esp_read_mac(mac_adr, ESP_MAC_WIFI_STA);
    sprintf(_uid, "%02X%02X%02X%02X%02X%02X", mac_adr[0], mac_adr[1], mac_adr[2],mac_adr[3],mac_adr[4],mac_adr[5]);
    strlcpy(_name, name, sizeof(_name));
}

void AlpacaServer::begin(uint16_t udp_port, uint16_t tcp_port)
{
    if(!SPIFFS.begin(true)) {
        Serial.println(F("# Error mounting SPIFFS!"));
    }

    _portUDP = udp_port;
    _portTCP = tcp_port;

    DEBUGSTREAM->print("# Ascom Alpaca discovery port (UDP): ");
    DEBUGSTREAM->println(_portUDP);
    _serverUDP.listen(_portUDP);
    _serverUDP.onPacket([this](AsyncUDPPacket& udpPacket) { this->onAlpacaDiscovery(udpPacket);});

    DEBUGSTREAM->print("# Ascom Alpaca server port (TCP): ");
    DEBUGSTREAM->println(_portTCP);
    _serverTCP = new AsyncWebServer(_portTCP);
    _serverTCP->begin();

    _serverTCP->onNotFound([this](AsyncWebServerRequest *request) {
        String url = request->url();
        request->send(400, "text/plain", "Not found: '" + url + "'");
    });

    _registerCallbacks();
}

void AlpacaServer::addDevice(AlpacaDevice *device)
{
    if(_n_devices == ALPACA_MAX_DEVICES) {
        DEBUGSTREAM->println("# ERROR - max alpaca devices exceeded");
        return;
    }

    int device_number = 0;
    const char* device_type = device->getDeviceType();
    for(int i=0;i<_n_devices;i++){
        if(strcmp(_device[i]->getDeviceType(), device_type) == 0) {
            device_number++;
        }
    }
    _device[_n_devices++] = device;
    device->setAlpacaServer(this);
    device->setDeviceNumber(device_number);
    device->registerCallbacks();
}

void AlpacaServer::_registerCallbacks()
{
    DEBUGSTREAM->println(F("# Register handler for \"/management/apiversions\" to getApiVersions"));
    _serverTCP->on("/management/apiversions", HTTP_GET, LHF(_getApiVersions));
    DEBUGSTREAM->println(F("# Register handler for \"/management/v1/description\" to getDescription"));
    _serverTCP->on("/management/v1/description", HTTP_GET, LHF(_getDescription));
    DEBUGSTREAM->println(F("# Register handler for \"/management/v1/configureddevices\" to getConfiguredDevices"));
    _serverTCP->on("/management/v1/configureddevices", HTTP_GET, LHF(_getConfiguredDevices));

    _serverTCP->serveStatic("/setup", SPIFFS, "/www/setup.html");
    _serverTCP->serveStatic(SETTINGS_FILE, SPIFFS, SETTINGS_FILE);
    _serverTCP->serveStatic("/js", SPIFFS, "/www/js/").setCacheControl("max-age=3600");
    _serverTCP->serveStatic("/css", SPIFFS, "/www/css/").setCacheControl("max-age=3600");

    DEBUGSTREAM->println(F("# Register handler for \"/jsondata\" to readJson"));
    _serverTCP->on("/jsondata", HTTP_GET, LHF(_getJsondata));
    _serverTCP->on("/links", HTTP_GET, LHF(_getLinks));
    AsyncCallbackJsonWebHandler* jsonhandler = new AsyncCallbackJsonWebHandler("/jsondata", [this](AsyncWebServerRequest *request, JsonVariant& json){
       JsonObject jsonObj = json.as<JsonObject>();
       this->_readJson(jsonObj);
       request->send(200, F("application/json"), F("{\"received\":\"true\"}"));
    });
    _serverTCP->addHandler(jsonhandler);
    _serverTCP->on("/save_settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (this->saveSettings())
            request->send(200,"application/json",F("{\"saved\":true}"));
        else
            request->send(400,"application/json",F("{\"saved\":false}"));
    });
}

void AlpacaServer::_getApiVersions(AsyncWebServerRequest *request){
    respond(request, ALPACA_API_VERSIONS);
}

void AlpacaServer::_getDescription(AsyncWebServerRequest *request){
    respond(request, ALPACA_DESCRIPTION);
}

void AlpacaServer::_getConfiguredDevices(AsyncWebServerRequest *request){
    String value = "[";
    char deviceinfo[256];
    for(int i=0; i<_n_devices; i++) {
        snprintf(
            deviceinfo, sizeof(deviceinfo),
            ALPACA_DEVICE_LIST,
            _device[i]->getDeviceName(),
            _device[i]->getDeviceType(),
            _device[i]->getDeviceNumber(),
            _device[i]->getDeviceUID()
        );
        value += deviceinfo;
        if(i<_n_devices-1)
            value += ",";
    }
    value += "]";
    respond(request, value.c_str());
}

int AlpacaServer::_paramIndex(AsyncWebServerRequest *request, const char* name)
{
    for(int i=0; i< request->args(); i++) {
        if (request->argName(i).equalsIgnoreCase(name)) {
            return i;
        }
    }
    return -1;
}

bool AlpacaServer::getParam(AsyncWebServerRequest *request, const char* name, bool &value)
{
    int index = _paramIndex(request, name);
    if(index < 0)
        return false;
    value = request->arg(index).equalsIgnoreCase("True");
    value |= request->arg(index).toInt() == 1;
    return true;
}

bool AlpacaServer::getParam(AsyncWebServerRequest *request, const char* name, float &value)
{
    int index = _paramIndex(request, name);
    if(index < 0)
        return false;
    value = request->arg(index).toFloat();
    return true;
}

bool AlpacaServer::getParam(AsyncWebServerRequest *request, const char* name, int &value)
{
    int index = _paramIndex(request, name);
    if(index < 0)
        return false;
    value = request->arg(index).toInt();
    return true;
}

bool AlpacaServer::getParam(AsyncWebServerRequest *request, const char* name, char *buffer, int buffer_size)
{
    int index = _paramIndex(request, name);
    if(index < 0)
        return false;
    request->arg(index).toCharArray(buffer, buffer_size);
    return true;
}

void AlpacaServer::respond(AsyncWebServerRequest *request, bool value, int32_t error_number, const char* error_message)
{
    const char* str_val = (value ? "true" : "false");
    respond(request, str_val, error_number, error_message);
}

void AlpacaServer::respond(AsyncWebServerRequest *request, int32_t value, int32_t error_number, const char* error_message)
{
    char str_val[16];
    sprintf(str_val, "%i", value);
    respond(request, str_val, error_number, error_message);
}

void AlpacaServer::respond(AsyncWebServerRequest *request, float value, int32_t error_number, const char* error_message)
{
    char str_val[16];
    sprintf(str_val, "%0.5f", value);
    respond(request, str_val, error_number, error_message);
}

void AlpacaServer::respond(AsyncWebServerRequest *request, const char* value, int32_t error_number, const char* error_message)
{
    DEBUGSTREAM->print("# Alpaca (");
    DEBUGSTREAM->print(request->client()->remoteIP());
    DEBUGSTREAM->print(") ");
    DEBUGSTREAM->println(request->url());

    int clientTransactionID = 0;
    for(int i=0; i< request->args(); i++) {
        if (request->argName(i).equalsIgnoreCase("clienttransactionid")) {
            clientTransactionID = request->arg(i).toInt();
            break;
        }
    }
    _serverTransactionID++;

    // Use heap-allocated String instead of stack char[2048]
    String response;
    response.reserve(256);

    if(value == nullptr) {
        char buf[256];
        snprintf(buf, sizeof(buf), ALPACA_RESPOSE_ERROR, clientTransactionID, _serverTransactionID, error_number, error_message);
        response = buf;
    } else {
        char buf[512];
        if ((value[0] >= '0' && value[0] <= '9') || value[0] == '-' || value[0] == '[' || value[0] == '{' || value[0] == '"' || strcmp(value, "true") == 0 || strcmp(value, "false") == 0) {
            snprintf(buf, sizeof(buf), ALPACA_RESPOSE_VALUE_ERROR, value, clientTransactionID, _serverTransactionID, error_number, error_message);
        } else {
            snprintf(buf, sizeof(buf), ALPACA_RESPOSE_VALUE_ERROR_STR, value, clientTransactionID, _serverTransactionID, error_number, error_message);
        }
        response = buf;
    }
    request->send(200, ALPACA_JSON_TYPE, response);
    DEBUGSTREAM->println(response);
}

void AlpacaServer::onAlpacaDiscovery(AsyncUDPPacket& udpPacket)
{
    int length = udpPacket.length();
    if (length == 0)
        return;

    DEBUGSTREAM->print(F("# Alpaca Discovery - Remote ip "));
    DEBUGSTREAM->println(udpPacket.remoteIP());

    if (length < 16) {
        DEBUGSTREAM->print(F("# Alpaca Discovery - Wrong packet size "));
        DEBUGSTREAM->println(length);
        return;
    }

    AlpacaDiscoveryPacket* alpaca_packet = (AlpacaDiscoveryPacket*)udpPacket.data();
    if (alpaca_packet->valid()) {
        DEBUGSTREAM->print("# Alpaca Discovery - Header v. ");
        DEBUGSTREAM->println(alpaca_packet->version());
    } else {
        DEBUGSTREAM->println("# Alpaca Discovery - Header mismatch");
        return;
    }

    uint8_t resp_buf[24];
    int resp_len = sprintf((char *)resp_buf, "{\"AlpacaPort\":%d}", _portTCP);
    _serverUDP.writeTo(resp_buf, resp_len, udpPacket.remoteIP(), udpPacket.remotePort());
}

void AlpacaServer::_getJsondata(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(1024);
    JsonObject root = doc.to<JsonObject>();
    _writeJson(root);
    String ser_json;
    serializeJson(root, ser_json);
    request->send(200, ALPACA_JSON_TYPE, ser_json);
}

void AlpacaServer::_getLinks(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(512);
    JsonObject root = doc.to<JsonObject>();
    root[F("Server")] = F("/setup");
    for(int i=0; i<_n_devices; i++) {
        root[_device[i]->getDeviceName()] = _device[i]->getDeviceURL();
    }
    String ser_json;
    serializeJson(root, ser_json);
    request->send(200, ALPACA_JSON_TYPE, ser_json);
}

void AlpacaServer::_readJson(JsonObject &root)
{
    const char* name = root[F("name")];
    if (name)
        strlcpy(_name, name, sizeof(_name));
    _portTCP = root[F("TCP_port")] | _portTCP;
    _portUDP = root[F("UDP_port")] | _portUDP;
}

void AlpacaServer::_writeJson(JsonObject &root)
{
    root[F("Name")] = _name;
    root[F("UID")] = _uid;
    root[F("TCP_port")] = _portTCP;
    root[F("UDP_port")] = _portUDP;
}

bool AlpacaServer::saveSettings()
{
    DynamicJsonDocument doc(4096);
    JsonObject root = doc.to<JsonObject>();
    _writeJson(root);
    for(int i=0; i<_n_devices; i++) {
        JsonObject json_obj = root.createNestedObject(_device[i]->getDeviceUID());
        _device[i]->aWriteJson(json_obj);
    }
    SPIFFS.remove(SETTINGS_FILE);
    File file = SPIFFS.open(SETTINGS_FILE, FILE_WRITE);
    if(!file) {
        DEBUGSTREAM->println(F("# SPIFFS could not create settings.json"));
        return false;
    }
    if(serializeJson(doc, file) == 0) {
        DEBUGSTREAM->println(F("# ArduinoJson failed to write settings.json"));
        file.close();
        return false;
    } else {
        DEBUGSTREAM->println(F("# ArduinoJson wrote to settings.json successfully"));
    }
    file.close();
    return true;
}

bool AlpacaServer::loadSettings()
{
    DynamicJsonDocument doc(4096);

    File file = SPIFFS.open(SETTINGS_FILE, FILE_READ);
    if(!file) {
        DEBUGSTREAM->println(F("# SPIFFS could not open settings.json"));
        return false;
    }
    DeserializationError error = deserializeJson(doc, file);
    JsonObject root = doc.as<JsonObject>();
    file.close();
    if(error) {
        DEBUGSTREAM->println(F("# ArduinoJson failed to parse settings.json"));
        return false;
    } else {
        DEBUGSTREAM->println(F("# ArduinoJson opened settings.json successfully"));
    }
    _readJson(root);
    for(int i=0; i<_n_devices; i++) {
        JsonObject json_obj = root[_device[i]->getDeviceUID()];
        if(json_obj)
            _device[i]->aReadJson(json_obj);
    }
    return true;
}
