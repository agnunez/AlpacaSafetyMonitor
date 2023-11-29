#pragma once
#include <Arduino.h>
#include "config.h"
#include "pins.h"

#include <ESP_WiFiManager.h>
//#include <ESP_DoubleResetDetector.h>
#include <ESPmDNS.h>
#include <SPI.h>
#include <TMCStepper.h>
#include <FastAccelStepper.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "focuser.h"
#include "safetymonitor.h"
#include "AlpacaServer.h"
#include "meteo.h"

// encoder global variables
bool encA, encB;
volatile int32_t enc_counter = 0;
DeviceAddress temp_address;
float temperature;

// weather sensors loop delay
static unsigned long measureDelay = 5000;   // Sensors read cycle in ms. Always greater than 3000
static unsigned long lastTimeRan;           // a

// module setup
void setup_wifi();
void setup_encoder();
void setup_sensors();
//void setup_i2cmlxbme();

// module housekeeping
void update_encoder();
void update_sensors();
void update_focus();
void update_i2cmlxbme(unsigned long measureDelay);

// interrupt routines
void ICACHE_RAM_ATTR encoderA_isr();
void ICACHE_RAM_ATTR encoderB_isr();

Meteo meteo1("AlpacaESP32");

