#pragma once
#include <Arduino.h>
#include "config.h"
#include "AlpacaSafetyMonitor.h"
/* 
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>
*/


// mlx static and float variables
#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))          // missing math function sign of number
static float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};  // sky temperature corrections polynomial coefficients
static float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint;
static float limit_tamb = 0.;     // freezing below this
static float limit_tsky = 30.; //-15.;   // cloudy above this
static float limit_humid = 90.;   // risk for electronics above this
static float limit_dew = 5.;      // risk for optics with temp - dewpoint below this
static float delay2open = 1200.;   // waiting time before open roof after a safety close
static float delay2close = 120.;   // waiting time before close roof with continuos overall safety waring for this
static float time2open, time2close;
static bool  status_tamb, status_tsky, status_humid, status_dew, status_weather, instant_status, status_roof;
static bool _issafe = false;

// Circular buffer functions
#define CB_SIZE 24
static float cb[CB_SIZE] = {0.};
static float cb_noise[CB_SIZE] = {0.};
static int   cb_index     = 0;
static float cb_avg       = 0.0;
static float cb_rms       = 0.0;


class SafetyMonitor : public AlpacaSafetyMonitor {
    private:
        static uint8_t _n_safetymonitors;
        static SafetyMonitor *_safetymonitor_array[4];
        uint8_t _safetymonitor_index;
        

    public:
        SafetyMonitor()  : AlpacaSafetyMonitor()
        { _safetymonitor_index = _n_safetymonitors++; } 
        bool begin();
        void update();

        // alpaca getters
        void aGetIsSafe(AsyncWebServerRequest *request)  { _alpacaServer->respond(request,  (_issafe ? "true" : "false" )); }
    
        // alpaca setters

        // alpaca json
        void aReadJson(JsonObject &root);
        void aWriteJson(JsonObject &root);
};