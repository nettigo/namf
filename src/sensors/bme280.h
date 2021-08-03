//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_BME280_H
#define NAMF_BME280_H
#include "Arduino.h"
#include "defines.h"
#include "variables.h"
#include "html-content.h"
#include "helpers.h"
#include "system/debug.h"

#include <Adafruit_BMP280.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BME280.h>

/*****************************************************************
 * BMP280 declaration                                               *
 *****************************************************************/
extern Adafruit_BMP280 bmp280;
extern Adafruit_BMP085 bmp180;

/*****************************************************************
 * BME280 declaration                                            *
 *****************************************************************/
extern Adafruit_BME280 bme280;

bool initBME280(char addr);
String sensorBME280();

#endif //NAMF_BME280_H
