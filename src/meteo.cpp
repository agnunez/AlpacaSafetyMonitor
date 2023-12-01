#include "meteo.h"

Adafruit_BME280 bme;  // I2C
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

Meteo::Meteo(const std::string& newName) :  Name(newName) { }

const std::string& Meteo::getName() const {
    return Name;
}

void Meteo::setup_i2cmlxbme() {
    Wire.end();                  // Set I2C pinout
    Wire.setPins(SDApin,SCLpin);  
    Wire.begin();
    mlx.begin();                 // Initialize sensors
    bme.begin(0x76);
}  

#define sgn(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))  

float tsky_calc(float ts, float ta){
  float t67,td = 0;
  float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};
  if (abs(k[2]/10.-ta) < 1) {
    t67=sgn(k[6])*sgn(ta-k[2]/10.) * abs((k[2]/10. - ta));
  } else {
    t67=k[6]*sgn(ta-k[2]/10.)*(log(abs((k[2]/10-ta)))/log(10.) + k[7]/100);
  }
  td = (k[1]/100.)*(ta-k[2]/10.)+(k[3]/100.)*pow(exp(k[4]/1000.*ta),(k[5]/100.))+t67;
  return (ts-td);
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

// Read through i2c bus mlx90614 and bme280 sensors

void Meteo::update_i2cmlxbme(unsigned long measureDelay){
  Serial.print(F("# update meteo1 & safetymonitors\n"));

  bme_temperature = bme.readTemperature();
  bme_humidity    = bme.readHumidity();
  bme_pressure    = bme.readPressure()/ 100.0F;
  mlx_tempamb     = mlx.readAmbientTempC();
  mlx_tempobj     = mlx.readObjectTempC();

  dewpoint        = bme_temperature - (100-bme_humidity)/5.;
  tempsky         = tsky_calc(mlx_tempobj, mlx_tempamb);
  cb_add(tempsky);   // add tempsky value to circular buffer and calculate  Turbulence (noise dB) / Seeing estimation
  noise_db        = cb_noise_db_calc();
  cloudcover      = 100.+(tempsky*6.);
  if (cloudcover>100.) cloudcover = 100.;
  if (cloudcover<0.) cloudcover = 0.;
}