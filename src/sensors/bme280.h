//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_BME280_H
#define NAMF_BME280_H
#include "Arduino.h"
#include "helpers.h"
extern const char DBG_TXT_START_READING[] PROGMEM;
extern const char DBG_TXT_COULDNT_BE_READ[] PROGMEM;
extern const char DBG_TXT_TEMPERATURE[] PROGMEM;
extern const char DBG_TXT_PRESSURE[] PROGMEM;
extern const char DBG_TXT_HUMIDITY[] PROGMEM;
extern const char DBG_TXT_END_READING[] PROGMEM;
extern const char SENSORS_BME280[] PROGMEM;
extern const char DEBUG_MED_INFO[] PROGMEM;
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
