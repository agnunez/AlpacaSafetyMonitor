#pragma once
// Stepper 1 pins
#define STP1_EN      14
#define STP1_STEP    12
#define STP1_DIR     13
#define STP1_RX      27
#define STP1_TX      26
#define STP1_SERIAL  Serial1

// Stepper 2 pins
#define STP2_EN      32
#define STP2_STEP    33
#define STP2_DIR     25
#define STP2_RX      35
#define STP2_TX      34
#define STP2_SERIAL  Serial2

// INPUTS
#define PIN_HOME1    16
#define PIN_HOME2    17
#define PIN_ENC_A    15
#define PIN_ENC_B    4
#define PIN_ENC_SEL  5

// Temperature
#define PIN_DS1820   18

// LEDS
#define PIN_WIFI_LED LED_BUILTIN