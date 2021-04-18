#pragma once
// General
#define DEVICENAME          "ESP AstroFocuser"
#define VERSION             "v1.0.0"
#define COPYRIGHT           "2021 Njål Brekke"
#define FOCUSER2_ENABLE     true
#define COEFF_RANGE         1000 // temp coeff in um/C
#define HOMING_SPEED        1.0f
#define DEBUG

// TMC2208
#define TMC_RS              0.11f   // Rs of Fysetc TMC2208 is 0.11 Ohm

// Stepper settings
#define STP_MICROSTEP      32   // Microsteps internally
#define STP_CURRENT        300  // Stepper current per phase (mAmps)
#define STP_MAX_SPEED      5.0f // mm/s
#define STP_ACCELERATION   0.5f // mm/s^2

// Rotary encoder
#define ENCODER_STEPS      1600
#define ENCODER_RATIO      1/2

// Mechanics
#define STP_MM_PER_REV     (3.0 * 2 * 3.14)
#define STP_GEAR_RATIO     (30/1)
#define STP_STEPS_PER_MM   (200 * STP_MICROSTEP * STP_GEAR_RATIO / STP_MM_PER_REV)

// WiFi config
#define HOSTNAME             "astrofocuser"
#define TCP_PORT             3117
#define ESP_DRD_USE_EEPROM   true
#define DRD_TIMEOUT          3
#define DRD_ADDRESS          0
#define ESP_DRD_USE_SPIFFS   true

// ASCOM Alpaca
#define ALPACA_UDP_PORT     32227
#define ALPACA_TCP_PORT     80