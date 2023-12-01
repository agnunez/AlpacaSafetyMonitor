#include "observingconditions.h"
#include "meteo.h"


// cannot call member functions directly from interrupt, so need these helpers for up to 1 ObservingConditions
uint8_t ObservingConditions::_n_observingconditionss = 0;
ObservingConditions *ObservingConditions::_observingconditions_array[4] = { nullptr,  nullptr, nullptr,  nullptr };

bool ObservingConditions::begin()
{

    // done
    _observingconditions_array[_observingconditions_index] = this;
    return true;
}

void ObservingConditions::update(Meteo meteo, unsigned long measureDelay) {
  //  update meteo
  temperature = meteo.bme_temperature;
  humidity = meteo.bme_humidity;
  pressure = meteo.bme_pressure;
  dewpoint = meteo.dewpoint;
  tempsky = meteo.tempsky;
  noise_db = meteo.noise_db;
  cloudcover = meteo.cloudcover;
  skyquality = meteo.skyquality;
  skybrightness = meteo.skybrightness;
};



void ObservingConditions::aReadJson(JsonObject &root)
{
    AlpacaObservingConditions::aReadJson(root);
    if(JsonObject obj_config = root[F("Configuration")]) {
        _avgperiod  = obj_config[F("Average Period")] | _avgperiod;
        _refresh    = obj_config[F("Refresh Period")] | _refresh;
    }
}

void ObservingConditions::aWriteJson(JsonObject &root)
{
    AlpacaObservingConditions::aWriteJson(root);
    // read-only values marked with #
    JsonObject obj_config = root.createNestedObject(F("Configuration"));
    obj_config[F("Average Period")]  = _avgperiod;
    obj_config[F("Refresh Period")]  = _refresh;

    JsonObject obj_state  = root.createNestedObject(F("State"));
    obj_state[F("Sensors_Description")]    = sensordescription ;
    obj_state[F("Ambient_Temperature")]    = temperature ;
    obj_state[F("Sky_Temperature ")]       = tempsky;
    obj_state[F("Humidity")]               = humidity;
    obj_state[F("Pressure")]               = pressure;
    obj_state[F("Dewpoint")]               = dewpoint;
    obj_state[F("Turbulence_db")]          = noise_db;   // not exactly seeing (fwhm)
    obj_state[F("Cloud_Cover")]            = cloudcover;  
    obj_state[F("Sky_Quality")]            = skyquality; 
    obj_state[F("Sky_Brightness")]         = skybrightness; 

}

