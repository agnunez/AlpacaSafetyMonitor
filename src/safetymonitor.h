#pragma once
#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "AlpacaSafetyMonitor.h"
#include "meteo.h"

class SafetyMonitor : public AlpacaSafetyMonitor {
    private:
        static uint8_t _n_safetymonitors;
        static SafetyMonitor *_safetymonitor_array[4];
        uint8_t _safetymonitor_index;
        float limit_tamb = 0.;     // freezing below this
        float limit_tsky = -15.;   //-15.;   // cloudy above this
        float limit_humid = 90.;   // risk for electronics above this
        float limit_dew = 5.;      // risk for optics with temp - dewpoint below this
        float delay2open = 1200.;  // waiting time before open roof after a safety close
        float delay2close = 120.;  // waiting time before close roof with continuos overall safety waring for this
        float time2open, time2close = 0;
        bool  status_tamb, status_tsky, status_humid, status_dew, status_weather, instant_status, status_roof = false;
        bool _issafe = false;  // overall meteo safety status
        // acquired parameters
        float temperature, humidity, pressure, dewpoint, tempsky; 
    public:
        SafetyMonitor()  : AlpacaSafetyMonitor() {  _safetymonitor_index = _n_safetymonitors++; }
        bool begin();
        void update(Meteo meteo, unsigned long measureDelay);

        // alpaca getters
        void aGetIsSafe(AsyncWebServerRequest *request)  {  _alpacaServer->respond(request,  _issafe);  }
    
        // alpaca setters

        // alpaca json
        void aReadJson(JsonObject &root);
        void aWriteJson(JsonObject &root);

};