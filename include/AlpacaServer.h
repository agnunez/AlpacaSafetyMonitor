#pragma once
#include <Arduino.h>
#include <AsyncUDP.h>
#include <WebServer.h>
#include <esp_system.h>
#include "alpaca_defines.h"
#include "config.h"

void setup_alpaca(uint16_t udp_port, uint16_t tcp_port);
void update_alpaca();
void genUID();
void onAlpacaDiscovery(AsyncUDPPacket& udpPacket);