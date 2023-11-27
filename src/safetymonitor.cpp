#include "safetymonitor.h"

Adafruit_BME280 bme;  // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

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
    obj_state[F("Ambient_Temperature")]    = bme_temperature;
    obj_state[F("Sky_Temperature ")]       = tempsky;
    obj_state[F("Humidity")]               = bme_humidity;
    obj_state[F("Pressure")]               = bme_pressure;
    obj_state[F("Time_to_open")]           = time2open;
    obj_state[F("Time_to_close")]          = time2close;
    obj_state[F("Safety_Monitor_status")]  = status_roof;
}

/*
float limit_tamb = 0.;     // freezing below this
float limit_tsky = -15.;   // cloudy above this
float limit_humid = 90.;   // risk for electronics above this
float limit_dew = 5.;      // risk for optics with temp - dewpoint below this
float delay2open = 1200.;   // waiting time before open roof after a safety close
float delay2close = 120.;   // waiting time before close roof with continuos overall safety waring for this
*/


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

float tsky_calc(float ts, float ta){
  float t67,td = 0;
  if (abs(k[2]/10.-ta) < 1) {
    t67=sgn(k[6])*sgn(ta-k[2]/10.) * abs((k[2]/10. - ta));
  } else {
    t67=k[6]*sgn(ta-k[2]/10.)*(log(abs((k[2]/10-ta)))/log(10.) + k[7]/100);
  }
  td = (k[1]/100.)*(ta-k[2]/10.)+(k[3]/100.)*pow(exp(k[4]/1000.*ta),(k[5]/100.))+t67;
  return (ts-td);
}

float dewpoint_calc(float temp, float humid){
  return (temp -((100-humid)/5.));
}

void update_i2cmlxbme(unsigned long measureDelay){
  bme_temperature = bme.readTemperature();
  bme_humidity    = bme.readHumidity();
  bme_pressure    = bme.readPressure()/ 100.0F;
  mlx_tempamb     = mlx.readAmbientTempC();
  mlx_tempobj     = mlx.readObjectTempC();
  tempsky     = tsky_calc(mlx_tempobj, mlx_tempamb);
  cb_add(tempsky);   // add tempsky value to circular buffer and calculate  Turbulence (noise dB) / Seeing estimation
  noise_db    = cb_noise_db_calc();
  dewpoint    = dewpoint_calc(bme_temperature, bme_humidity);
  
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