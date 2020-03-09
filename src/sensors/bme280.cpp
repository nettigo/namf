//
// Created by viciu on 28.01.2020.
//

#include "sensors/bme280.h"

Adafruit_BMP280 bmp280;
Adafruit_BME280 bme280;

/*****************************************************************
 * Init BME280                                                   *
 *****************************************************************/
bool initBME280(char addr) {
    debug_out(F("Trying BME280 sensor on "), DEBUG_MIN_INFO, 0);
    debug_out(String(addr, HEX), DEBUG_MIN_INFO, 0);

    if (bme280.begin(addr)) {
        debug_out(F(" ... found"), DEBUG_MIN_INFO, 1);
        bme280.setSampling(
                Adafruit_BME280::MODE_FORCED,
                Adafruit_BME280::SAMPLING_X1,
                Adafruit_BME280::SAMPLING_X1,
                Adafruit_BME280::SAMPLING_X1,
                Adafruit_BME280::FILTER_OFF);
        return true;
    } else {
        debug_out(F(" ... not found"), DEBUG_MIN_INFO, 1);
        return false;
    }
};



/*****************************************************************
 * read BME280 sensor values                                     *
 *****************************************************************/
String sensorBME280() {
    String s;

    debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_BME280), DEBUG_MED_INFO, 1);

    if (bme280.takeForcedMeasurement()) {
        const auto t = bme280.readTemperature();
        const auto h = bme280.readHumidity();
        const auto p = bme280.readPressure();
        if (isnan(t) || isnan(h) || isnan(p)) {
            last_value_BME280_T = -128.0;
            last_value_BME280_H = -1.0;
            last_value_BME280_P = -1.0;
            debug_out(String(FPSTR(SENSORS_BME280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
            return s;
        } else {
            debug_out(FPSTR(DBG_TXT_TEMPERATURE), DEBUG_MIN_INFO, 0);
            debug_out(Float2String(t) + " C", DEBUG_MIN_INFO, 1);
            debug_out(FPSTR(DBG_TXT_HUMIDITY), DEBUG_MIN_INFO, 0);
            debug_out(Float2String(h) + " %", DEBUG_MIN_INFO, 1);
            debug_out(FPSTR(DBG_TXT_PRESSURE), DEBUG_MIN_INFO, 0);
            debug_out(Float2String(p / 100) + " hPa", DEBUG_MIN_INFO, 1);
            last_value_BME280_T = t;
            last_value_BME280_H = h;
            last_value_BME280_P = p;
            s += Value2Json(F("BME280_temperature"), Float2String(last_value_BME280_T));
            s += Value2Json(F("BME280_humidity"), Float2String(last_value_BME280_H));
            s += Value2Json(F("BME280_pressure"), Float2String(last_value_BME280_P));
        }
        debug_out("----", DEBUG_MIN_INFO, 1);
    } else {
        debug_out(String(FPSTR(SENSORS_BME280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
        return s;
    }
    debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_BME280), DEBUG_MED_INFO, 1);

    return s;
};
