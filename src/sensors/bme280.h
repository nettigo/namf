//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_BME280_H
#define NAMF_BME280_H
#include "Arduino.h"
#include "variables.h"
#include "helpers.h"
#include "html-content.h"

#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>

/*****************************************************************
 * BMP280 declaration                                               *
 *****************************************************************/
Adafruit_BMP280 bmp280;

/*****************************************************************
 * BME280 declaration                                            *
 *****************************************************************/
Adafruit_BME280 bme280;


static String sensorBME280();
#endif //NAMF_BME280_H
