#pragma once
#include <Arduino.h>
#include "config.h"
// mlx & bme
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>

// Circular buffer functions
#define CB_SIZE 24
static float cb[CB_SIZE] = {0.};
static float cb_noise[CB_SIZE] = {0.};
static int   cb_index     = 0;
static float cb_avg       = 0.0;
static float cb_rms       = 0.0;

#ifndef METEO_H
#define METEO_H


class Meteo {
public:
    // attributes
	std::string Name;
	float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint, cloudcover, skyquality, skybrightness;
    // sensors
    // methods
    void update_i2cmlxbme(unsigned long measureDelay);
	Meteo(const std::string& newName); // constructor place
    // setters
    void setIssafe(bool value);
    // getters
    bool getIssafe() const;
	const std::string& getName() const;
    void setup_i2cmlxbme();

};

#endif