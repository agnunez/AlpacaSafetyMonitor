// shared_variables.h
#pragma once
#ifndef SHARED_VARIABLE_H
#define SHARED_VARIABLE_H

extern float k[] = {0., 33., 0., 4., 100., 100., 0., 0.};  // sky temperature corrections polynomial coefficients
extern float bme_temperature, bme_humidity, bme_pressure, mlx_tempamb, mlx_tempobj, tempsky, noise_db, dewpoint;
extern float rainrate, skybrightness, winddirection, windgust, windspeed = 0;      // not implemented yet


#endif // SHARED_VARIABLE_H
