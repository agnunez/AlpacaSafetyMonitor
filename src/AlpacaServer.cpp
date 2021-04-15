#include "AlpacaServer.h"
#include "AlpacaDevice.h"

#define DEBUGSTREAM if(debug) debugstream

AlpacaServer::AlpacaServer(const char* name)
{
    // Get unique ID from wifi macadr.
    uint8_t mac_adr[6];
    esp_read_mac(mac_adr, ESP_MAC_WIFI_STA);
    sprintf(_uid, "%02X%02X%02X%02X%02X%02X", mac_adr[0], mac_adr[1], mac_adr[2],mac_adr[3],mac_adr[4],mac_adr[5]);

    // save name
    strcpy(_name, name);
}

// initialize alpaca server
void AlpacaServer::begin(uint16_t udp_port, uint16_t tcp_port)
{
    // setup ports
    _portUDP = udp_port;
    _portTCP = tcp_port;

    DEBUGSTREAM->print("# Ascom Alpaca discovery port (UDP): ");
    DEBUGSTREAM->println(_portUDP);
    _serverUDP.listen(_portUDP);
    _serverUDP.onPacket([this](AsyncUDPPacket& udpPacket) { this->onAlpacaDiscovery(udpPacket);});    

    DEBUGSTREAM->print("# Ascom Alpaca server port (TCP): ");
    DEBUGSTREAM->println(_portTCP);
    _serverTCP.begin(_portTCP);

    _serverTCP.onNotFound([this]() {
        String url = this->_serverTCP.uri();
        this->_serverTCP.send(400, "text/plain", "Not found: '" + url + "'");
    });
    
    _registerCallbacks();
}

// add alpaca device to server
void AlpacaServer::addDevice(AlpacaDevice *device)
{
    if(_n_devices == ALPACA_MAX_DEVICES) {
        DEBUGSTREAM->println("# ERROR - max alpaca devices exceeded");
        return;
    }

    // get device_number for device_type
    int device_number = 0;
    const char* device_type = device->getDeviceType();
    // loop through registered devices and count
    for(int i=0;i<_n_devices;i++){
        if(strcmp(_device[i]->getDeviceType(), device_type) == 0) {
            device_number++;
        }
    }
    // and set device number
    _device[_n_devices++] = device;
    device->setDeviceNumber(device_number);
    device->setAlpacaServer(this);
    device->registerCallbacks();
}

// register callbacks for REST API
void AlpacaServer::_registerCallbacks()
{
    DEBUGSTREAM->println(F("# Register handler for \"/management/apiversions\" to getApiVersions"));
    _serverTCP.on("/management/apiversions", HTTP_GET, LHF(_getApiVersions));
    DEBUGSTREAM->println(F("# Register handler for \"/management/v1/description\" to getDescription"));
    _serverTCP.on("/management/v1/description", HTTP_GET, LHF(_getDescription));
    DEBUGSTREAM->println(F("# Register handler for \"/management/v1/configureddevices\" to getConfiguredDevices"));
    _serverTCP.on("/management/v1/configureddevices", HTTP_GET, LHF(_getConfiguredDevices));
}

void AlpacaServer::_getApiVersions(){
    respond(ALPACA_API_VERSIONS);
}

void AlpacaServer::_getDescription(){
    respond(ALPACA_DESCRIPTION);
    //_serverTCP.send(200,"text/plain", ALPACA_DESCRIPTION);
}

// Return list of dicts describing connected alpaca devices
void AlpacaServer::_getConfiguredDevices(){
    char value[ALPACA_MAX_DEVICES*256] = "";
    char deviceinfo[256];
    strcat(value, "[");
    for(int i=0; i<_n_devices; i++) {
        sprintf(
            deviceinfo,
            ALPACA_DEVICE_LIST,
            _device[i]->getDeviceName(),
            _device[i]->getDeviceType(),
            _device[i]->getDeviceNumber(),
            _device[i]->getDeviceUID()
        );
        strcat(value, deviceinfo);
        if(i<_n_devices-1)
            strcat(value, ","); // add comma to all but last device
    }
    strcat(value, "]");
    respond(value);
}

void AlpacaServer::update()
{
    _serverTCP.handleClient();
}

// Update msg and client IDs
void AlpacaServer::_getTransactionData()
{
    _clientID = 0;
    _clientTransactionID = 0;
    for(int i=0; i< _serverTCP.args(); i++) {
        if (_serverTCP.argName(i).equalsIgnoreCase("clienttransactionid")) {
            _clientTransactionID = _serverTCP.arg(i).toInt();
        }
        else if (_serverTCP.argName(i).equalsIgnoreCase("clientid")) {
            _clientID = _serverTCP.arg(i).toInt();
        }
    }
    _serverTransactionID++;
}

// return index of parameter 'name' in PUT request, return -1 if not found
int AlpacaServer::_paramIndex(const char* name)
{
    for(int i=0; i< _serverTCP.args(); i++) {
        if (_serverTCP.argName(i).equalsIgnoreCase(name)) {
            return i;
        }
    }
    return -1;
}

// get value of parameter 'name' in PUT request and return true, return false if not found
bool AlpacaServer::getParam(const char* name, bool &value)
{
    int index = _paramIndex(name);
    if(index < 0) 
        return false;
    // both "True" and 1 should be interpreted as true.
    value = _serverTCP.arg(index).equalsIgnoreCase("True");
    value |= _serverTCP.arg(index).toInt() == 1;
    return true;
}

// get value of parameter 'name' in PUT request and return true, return false if not found
bool AlpacaServer::getParam(const char* name, float &value)
{
    int index = _paramIndex(name);
    if(index < 0) 
        return false;
    value = _serverTCP.arg(index).toFloat();
    return true;
}

// get value of parameter 'name' in PUT request and return true, return false if not found
bool AlpacaServer::getParam(const char* name, int &value)
{
    int index = _paramIndex(name);
    if(index < 0) 
        return false;
    value = _serverTCP.arg(index).toInt();
    return true;
}

// get value of parameter 'name' in PUT request and return true, return false if not found
bool AlpacaServer::getParam(const char* name, char *buffer, int buffer_size)
{
    int index = _paramIndex(name);
    if(index < 0)
        return false;
    _serverTCP.arg(index).toCharArray(buffer, buffer_size);
    return true;
}

// send response to alpaca client with bool
void AlpacaServer::respond(bool value, int32_t error_number, const char* error_message)
{
    const char* str_val = (value?"1":"0");
    respond(str_val, error_number, error_message);
}

// send response to alpaca client with int
void AlpacaServer::respond(int32_t value, int32_t error_number, const char* error_message)
{
    char str_val[16];
    sprintf(str_val, "%i", value);
    respond(str_val, error_number, error_message);
}

// send response to alpaca client with float
void AlpacaServer::respond(float value, int32_t error_number, const char* error_message)
{
    char str_val[16];
    sprintf(str_val, "%0.5f", value);
    respond(str_val, error_number, error_message);
}

// send response to alpaca client with string
void AlpacaServer::respond(const char* value, int32_t error_number, const char* error_message)
{
    DEBUGSTREAM->print("# Alpaca (");
    DEBUGSTREAM->print(_serverTCP.client().remoteIP());
    DEBUGSTREAM->print(") ");
    DEBUGSTREAM->println(_serverTCP.uri());
 
    _getTransactionData();

    // create msg to be sent, hope that buffer is large enough
    char response[2048];
    if( value == nullptr) {
        sprintf(response,ALPACA_RESPOSE_ERROR, _clientTransactionID, _serverTransactionID, error_number, error_message);
    } else {
        if ((value[0] >= '0' && value[0] <= '9') || value[0] == '[' || value[0] == '{' || value[0] == '"') {
            sprintf(response,ALPACA_RESPOSE_VALUE_ERROR, value, _clientTransactionID, _serverTransactionID, error_number, error_message);
        } else {
            sprintf(response,ALPACA_RESPOSE_VALUE_ERROR_STR, value, _clientTransactionID, _serverTransactionID, error_number, error_message);
        }
    }
    
    _serverTCP.send(200, ALPACA_JSON_TYPE, response);
    DEBUGSTREAM->println(response);
}

// Handler for replying to ascom alpaca discovery UDP packet
void AlpacaServer::onAlpacaDiscovery(AsyncUDPPacket& udpPacket)
{
    // check for arrived UDP packet at port
    int length = udpPacket.length();
    if (length == 0)
        return;

    DEBUGSTREAM->print(F("# Alpaca Discovery - Remote ip "));
    DEBUGSTREAM->println(udpPacket.remoteIP());

    // check size
    if (length < 16) {
        DEBUGSTREAM->print(F("# Alpaca Discovery - Wrong packet size "));
        DEBUGSTREAM->println(length);
        return;
    }
    
    // check package content
    AlpacaDiscoveryPacket* alpaca_packet = (AlpacaDiscoveryPacket*)udpPacket.data();
    if (alpaca_packet->valid()) {
        DEBUGSTREAM->print("# Alpaca Discovery - Header v. ");
        DEBUGSTREAM->println(alpaca_packet->version());
    } else {
        DEBUGSTREAM->println("# Alpaca Discovery - Header mismatch");
        return;
    }

    // reply port to ascom tcp server
    uint8_t resp_buf[24];
    int resp_len = sprintf((char *)resp_buf, "{\"alpacaport\":%d}", ALPACA_TCP_PORT);
    _serverUDP.writeTo(resp_buf, resp_len, udpPacket.remoteIP(), udpPacket.remotePort());
}