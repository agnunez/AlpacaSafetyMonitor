#pragma once
#include <Arduino.h>
#include "config.h"
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>

#ifndef METEO_H
#define METEO_H

class Meteo {
public:
    // properties
    bool _issafe;
	std::string Name;
	float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint;
    float limit_tamb = 0.;     // freezing below this
    float limit_tsky = 30.; //-15.;   // cloudy above this
    float limit_humid = 90.;   // risk for electronics above this
    float limit_dew = 5.;      // risk for optics with temp - dewpoint below this
    float delay2open = 1200.;   // waiting time before open roof after a safety close
    float delay2close = 120.;   // waiting time before close roof with continuos overall safety waring for this
    float time2open, time2close;
    bool status_tamb, status_tsky, status_humid, status_dew, status_weather, instant_status, status_roof;
    Adafruit_BME280 bme;  // I2C
    Adafruit_MLX90614 mlx = Adafruit_MLX90614();
    // methods
    void update_i2cmlxbme(unsigned long measureDelay);
	Meteo(const std::string& newName); // constructor place
    // setters
    void setIssafe(bool value);
    // getters
    bool getIssafe() const;
	const std::string& getName() const;
    void setup_i2cmlxbme(int sdapin, int sclpin, int i2caddress);
};

#endif