#include "AlpacaServer.h"

AlpacaServer::AlpacaServer(const char* name)
{
    // Get unique ID from wifi macadr.
    uint8_t mac_adr[6];
    esp_read_mac(mac_adr, ESP_MAC_WIFI_STA);
    sprintf(_uid, "{%X%X%X%X%X%X}", mac_adr[0], mac_adr[1], mac_adr[2],mac_adr[3],mac_adr[4],mac_adr[5]);

    // save name
    strcpy(_name, name);
}

void AlpacaServer::begin(uint16_t udp_port, uint16_t tcp_port)
{
    // setup ports
    _portUDP = udp_port;
    _portTCP = tcp_port;

    Serial.print("# Ascom Alpaca discovery port (UDP): ");
    Serial.println(_portUDP);
    _serverUDP.listen(_portUDP);
    _serverUDP.onPacket([this](AsyncUDPPacket& udpPacket) { this->onAlpacaDiscovery(udpPacket);});    

    Serial.print("# Ascom Alpaca server port (TCP): ");
    Serial.println(_portTCP);
    _serverTCP.begin(_portTCP);

    _serverTCP.onNotFound([this]() {
        String url = this->_serverTCP.uri();
        this->_serverTCP.send(200, "text/plain", "Not found: '" + url + "'");
    });
}

void AlpacaServer::update()
{
    _serverTCP.handleClient();
}

void AlpacaServer::onAlpacaDiscovery(AsyncUDPPacket& udpPacket)
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
    _serverUDP.writeTo(resp_buf, resp_len, udpPacket.remoteIP(), udpPacket.remotePort());
}