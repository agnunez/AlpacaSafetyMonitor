#pragma once
#include <Arduino.h>
#include "config.h"
// mlx & bme
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_BME280.h>

#ifndef METEO_H
#define METEO_H


class Meteo {
public:
    // attributes
	std::string Name;
	float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint;
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