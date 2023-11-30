#include "meteo.h"

Meteo::Meteo(const std::string& newName) :  Name(newName) { }

const std::string& Meteo::getName() const {
    return Name;
}
/*
void Meteo::setup_i2cmlxbme(int sdapin, int sclpin, int i2caddress) {
    Wire.end();                  // Set I2C pinout
    Wire.setPins(sdapin,sclpin);  
    Wire.begin();
    mlx.begin();                 // Initialize sensors
    bme.begin(i2caddress);
}
*/

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

void Meteo::update_i2cmlxbme(unsigned long measureDelay){
  /*
  bme_temperature = bme.readTemperature();
  bme_humidity    = bme.readHumidity();
  bme_pressure    = bme.readPressure()/ 100.0F;
  mlx_tempamb     = mlx.readAmbientTempC();
  mlx_tempobj     = mlx.readObjectTempC();
  */
  dewpoint        = bme_temperature - (100-bme_humidity)/5.;
  tempsky         = tsky_calc(mlx_tempobj, mlx_tempamb);
  //cb_add(tempsky);   // add tempsky value to circular buffer and calculate  Turbulence (noise dB) / Seeing estimation
  //noise_db    = cb_noise_db_calc();
  // RULES    status true means SAFE!  false means UNSAFE!
}