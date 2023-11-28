#include "safetymonitor.h"


// todo: set update() to private and only run when needed

// cannot call member functions directly from interrupt, so need these helpers for up to 1 SafetyMonitor
uint8_t SafetyMonitor::_n_safetymonitors = 0;
SafetyMonitor *SafetyMonitor::_safetymonitor_array[4] = { nullptr,  nullptr, nullptr,  nullptr };

bool SafetyMonitor::begin()
{

    // done
    _safetymonitor_array[_safetymonitor_index] = this;
    return true;
}

void SafetyMonitor::update()
{
    int target;
}


void SafetyMonitor::aReadJson(JsonObject &root)
{
    AlpacaSafetyMonitor::aReadJson(root);
    if(JsonObject obj_config = root[F("Configuration")]) {
        limit_tamb  = obj_config[F("Freezing_Temperature")] | limit_tamb;
        limit_tsky  = obj_config[F("Cloudy_SkyTemperature")] | limit_tsky;
        limit_humid = obj_config[F("Humidity_limit")] | limit_humid;
        limit_dew   = obj_config[F("Dew_delta_Temperature")] | limit_dew;
        delay2open  = obj_config[F("Delay_to_Open")] | delay2open;
        delay2close = obj_config[F("Delay_to_Close")] | delay2close;
    }
    status_roof = root[F("State")][F("Safety Monitor Status")] | status_roof;
}

void SafetyMonitor::aWriteJson(JsonObject &root)
{
    AlpacaSafetyMonitor::aWriteJson(root);
    // read-only values marked with #
    JsonObject obj_config = root.createNestedObject(F("Configuration"));
    obj_config[F("Freezing_Temperature")]    = limit_tamb;
    obj_config[F("Cloudy_SkyTemperature")]  = limit_tsky;
    obj_config[F("Humidity_limit")]          = limit_humid;
    obj_config[F("Dew_delta_Temperature")]   = limit_dew;
    obj_config[F("Delay_to_Open")]           = delay2open;
    obj_config[F("Delay_to_Close")]          = delay2close;

    JsonObject obj_state  = root.createNestedObject(F("State"));
    obj_state[F("Time_to_open")]           = time2open;
    obj_state[F("Time_to_close")]          = time2close;
    obj_state[F("Safety_Monitor_status")]  = status_roof;
}



// RULES    status true means SAFE!  false means UNSAFE!
  if (mlx_tempamb > limit_tamb){
    status_tamb = true;
  }else{
    status_tamb = false;    
  }
  if (bme_humidity < limit_humid){
    status_humid = true;
  }else{
    status_humid = false;    
  }
  if (tempsky < limit_tsky){
    status_tsky = true;
  }else{
    status_tsky = false;    
  }
  if ((mlx_tempamb - dewpoint) > limit_dew){
    status_dew = true;
  }else{
    status_dew = false;    
  }
  if (status_tamb && status_humid && status_tsky && status_dew){
    instant_status = true;
  }else{
    instant_status = false;
  }
  if (instant_status == false){
    if (status_weather == true) Serial.println("Unsafe received");
    time2open = delay2open;
    status_weather = false;
    if (status_roof == true){
      if (time2close == 0.) {
        status_roof = false;
        _issafe = false;
        Serial.println("Close Roofs");
        digitalWrite(ROOFpin, LOW);
      }
    }
  }
  if (instant_status == true){
    if (status_weather == false) Serial.println("Safe received");
    time2close = delay2close;
    status_weather = true;
    if (status_roof == false){
      if (time2open == 0.) {
        status_roof = true;
        _issafe = true;
        Serial.println("Open Roofs");
        digitalWrite(ROOFpin, HIGH);
      }
    }
  }
  time2open -= measureDelay/1000;
  if (time2open < 0.) time2open = 0;
  time2close -= measureDelay/1000;
  if (time2close < 0.) time2close = 0;
};