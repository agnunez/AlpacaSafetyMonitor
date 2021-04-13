#include "AlpacaServer.h"

AsyncUDP  alpacaUDP;
WebServer alpacaTCP;
int alpacaServerTransactionID = 0;
char uid[12];

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
    alpacaUDP.writeTo(resp_buf, resp_len, udpPacket.remoteIP(), udpPacket.remotePort());
}

void genUID()
{
    uint8_t mac_adr[6];
    esp_read_mac(mac_adr, ESP_MAC_WIFI_STA);
    sprintf(uid, "{%X%X%X%X%X%X}", mac_adr[0], mac_adr[1], mac_adr[2],mac_adr[3],mac_adr[4],mac_adr[5]);
}