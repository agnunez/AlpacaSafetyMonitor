//#define USE_SPIFFS            true
#define ESP_DRD_USE_EEPROM    true
#define DRD_TIMEOUT             10
#define DRD_ADDRESS             0

#include <Arduino.h>
#include <ESP_WiFiManager.h>
#include <ESP_DoubleResetDetector.h>
#include <SPI.h>
#include <TMCStepper.h>
#include <FastAccelStepper.h>

#include "config.h"
#include "pins.h"

FastAccelStepper *stepper1 = NULL;
FastAccelStepper *stepper2 = NULL;

bool encA, encB;
int32_t target_position = 16000;
int32_t min_position = 0;
int32_t max_position = 32000;

// interrupt routines
void ICACHE_RAM_ATTR homing1_isr();
void ICACHE_RAM_ATTR homing2_isr();
void ICACHE_RAM_ATTR encoderA_isr();
void ICACHE_RAM_ATTR encoderB_isr();