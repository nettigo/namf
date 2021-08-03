#include "bmpX80.h"


    Adafruit_BMP280 bmp280;
    Adafruit_BMP085 bmp180;

    typedef enum {
        BMP_180,
        BMP_280,
        NONE
    } SensorType;

    SensorType currentSensor = NONE;


    /*****************************************************************
     * Init BMP280                                                   *
     *****************************************************************/
    bool initBMP280(char addr) {
        debug_out(F("Trying BMP280 sensor on "), DEBUG_MIN_INFO, true);
        debug_out(String(addr, HEX), DEBUG_MIN_INFO, 0);

        if (bmp280.begin(addr)) {
            debug_out(F(" ... found"), DEBUG_MIN_INFO, 1);
            return true;
        } else {
            debug_out(F(" ... not found"), DEBUG_MIN_INFO, 1);
            return false;
        }
    }

    bool initBMP180(char addr = 0x77) {
        debug_out(F("Trying BMP180/085 sensor on "), DEBUG_MIN_INFO, 0);
        debug_out(String(addr, HEX), DEBUG_MIN_INFO, 0);
        if (bmp180.begin(addr)) {
            debug_out(F(" ... found"), DEBUG_MIN_INFO);
            return true;
        }
        debug_out(F(" ... not found"), DEBUG_MIN_INFO);
        return false;

    }

    bool readyBMPx80() {
        return currentSensor != NONE;
    }

    bool initBMPx80() {
        if (initBMP280(0x76) || initBMP280(0x77)) {
            currentSensor = BMP_280;
            return true;
        }
        if (initBMP180()) {
            currentSensor = BMP_180;
            return true;
        }
        return false;
    }


    const char * sensorPrefixBMPx80() {
        static char buff[8];
        switch (currentSensor) {
            case NONE:
                String(F("")).toCharArray(buff,8);
            case BMP_280:
                String(F("BMP280_")).toCharArray(buff,8);
            case BMP_180:
                String(F("BMP_")).toCharArray(buff,8);
        }
        return buff;
    }

    /*****************************************************************
     * read BMP280 sensor values                                     *
     *****************************************************************/
    String sensorBMPx80() {
        String s = F("");

        debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_BMP280), DEBUG_MED_INFO, true);
        float p = -1;
        float t = -128;
        String t_desc, p_desc;
        switch (currentSensor) {
            case NONE:
                return s;
            case BMP_180:
                p = (float)bmp180.readPressure();
                t = bmp180.readTemperature();
                t_desc = String(F("BMP_temperature"));
                p_desc = String(F("BMP_pressure"));
                break;
            case BMP_280:
                p = bmp280.readPressure();
                t = bmp280.readTemperature();
                t_desc = String(F("BMP280_temperature"));
                p_desc = String(F("BMP280_pressure"));
                break;
        }
        if (isnan(p) || isnan(t)) {
            last_value_BMP280_T = -128.0;
            last_value_BMP280_P = -1.0;
            debug_out(String(FPSTR(SENSORS_BMP280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
        } else {
//            debug_out(FPSTR(DBG_TXT_TEMPERATURE), DEBUG_MIN_INFO, 0);
//            debug_out(String(t) + " C", DEBUG_MIN_INFO, 1);
//            debug_out(FPSTR(DBG_TXT_PRESSURE), DEBUG_MIN_INFO, 0);
//            debug_out(Float2String(p / 100) + " hPa", DEBUG_MIN_INFO, 1);
            last_value_BMP280_T = t;
            last_value_BMP280_P = p;
            s += Value2Json(p_desc, Float2String(last_value_BMP280_P));
            s += Value2Json(t_desc, Float2String(last_value_BMP280_T));
        }
        debug_out(s, DEBUG_MIN_INFO, 1);

        debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_BMP280), DEBUG_MED_INFO, 1);

        return s;
    }

