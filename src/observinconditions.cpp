#include "observingconditions.h"

Adafruit_BME280 bme;  // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// todo: set update() to private and only run when needed

// cannot call member functions directly from interrupt, so need these helpers for up to 1 ObservingConditions
uint8_t ObservingConditions::_n_observingconditionss = 0;
ObservingConditions *ObservingConditions::_observingconditions_array[4] = { nullptr,  nullptr, nullptr,  nullptr };

bool ObservingConditions::begin()
{

    // done
    _observingconditions_array[_observingconditions_index] = this;
    return true;
}

void ObservingConditions::update()
{
    int target;
}


void ObservingConditions::aReadJson(JsonObject &root)
{
    AlpacaObservingConditions::aReadJson(root);
    if(JsonObject obj_config = root[F("Configuration")]) {
        _AveragePeriod = obj_config[F("Average_Period")] | _AveragePeriod;
        _Refresh = obj_config[F("Refresh_Rate")] | _Refresh;
    }
}

void ObservingConditions::aWriteJson(JsonObject &root)
{
    AlpacaObservingConditions::aWriteJson(root);
    // read-only values marked with #
    JsonObject obj_config = root.createNestedObject(F("Configuration"));
    obj_config[F("Average_Period")]          = _AveragePeriod;
    obj_config[F("Refresh_Rate")]            = _Refresh;

    JsonObject obj_state  = root.createNestedObject(F("State"));
    obj_state[F("Ambient_Temperature")]    = bme_temperature;
    obj_state[F("Sky_Temperature ")]       = tempsky;
    obj_state[F("Humidity")]               = bme_humidity;
    obj_state[F("Pressure")]               = bme_pressure;
}



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
  
