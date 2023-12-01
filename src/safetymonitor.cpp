#include "safetymonitor.h"
#include "meteo.h"
/*
Adafruit_BME280 bme;  // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
*/
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

void SafetyMonitor::update(Meteo meteo, unsigned long measureDelay) {
  //  update meteo
  temperature = meteo.bme_temperature;
  humidity = meteo.bme_humidity;
  pressure = meteo.bme_pressure;
  dewpoint = meteo.dewpoint;
  tempsky = meteo.tempsky;

  if (temperature > limit_tamb){
    status_tamb = true;
  }else{
    status_tamb = false;    
  }
  if (humidity < limit_humid){
    status_humid = true;
  }else{
    status_humid = false;    
  }
  if (tempsky < limit_tsky){
    status_tsky = true;
  }else{
    status_tsky = false;    
  }
  if ((temperature - dewpoint) > limit_dew){
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
    obj_state[F("Ambient_Temperature")]    = temperature ;
    obj_state[F("Sky_Temperature ")]       = tempsky;
    obj_state[F("Humidity")]               = humidity;
    obj_state[F("Pressure")]               = pressure;
    obj_state[F("Time_to_open")]           = time2open;
    obj_state[F("Time_to_close")]          = time2close;
    obj_state[F("Safety_Monitor_status")]  = status_roof;
}



/*
void setup_i2cmlxbme()
{
  Wire.end();                  // Set I2C pinout
  Wire.setPins(SDApin,SCLpin);  
  Wire.begin();
  mlx.begin();                 // Initialize sensors
  bme.begin(0x76);
}

float cb_avg_calc(){
  int sum = 0;
  for (int i = 0; i < CB_SIZE; i++) sum += cb[i];
  return ((float) sum) / CB_SIZE;
}

float cb_rms_calc(){
  int sum = 0;
  for (int i = 0; i < CB_SIZE; i++) sum += cb[i]*cb[i];
  return sqrt(sum/CB_SIZE);
}

void cb_add(float value){
  cb[cb_index] = value;
  cb_avg = cb_avg_calc();
  cb_rms = cb_rms_calc();
  cb_noise[cb_index] = abs(value)-cb_rms;    
  cb_index++;
  if (cb_index == CB_SIZE) cb_index = 0;
}

float cb_noise_db_calc(){
  float n = 0;
  for (int i = 0; i < CB_SIZE; i++){
    n += cb_noise[i]*cb_noise[i];
  }
  if (n == 0) return 0;
  return (10*log10(n));
}

float cb_snr_calc(){
  float s,n = 0;
  for (int i = 0; i < CB_SIZE; i++){
    s += cb[i]*cb[i];
    n += cb_noise[i]*cb_noise[i];
  }
  if (n == 0) return 0;
  return (10*log10(s/n));
}

*/