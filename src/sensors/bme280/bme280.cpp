#include "bme280.h"


namespace BME280 {
    const char KEY[] PROGMEM = "BME280";
    bool enabled = false;
    bool printOnLCD = false;
    const unsigned int SAMPLE_SIZE = 5;
    byte sampleCount = 0;

    float *samplesT = nullptr;
    float *samplesP = nullptr;
    float *samplesH = nullptr;


    Adafruit_BME280 bme280;


    bool readyBME280() {
        return !bme280_init_failed;
    }

    bool isEnabled() {
        return enabled;
    }
    /*****************************************************************
     * Init BMP280                                                   *
     *****************************************************************/
    bool initBME280(char addr) {
        debug_out(F("Trying BME280 sensor on "), DEBUG_MIN_INFO, true);
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
    }

    //set defaults if no config file
    void setDefaults(void) {
        enabled = true;
        printOnLCD = true;
    }

    bool getDisplaySetting() {
        return printOnLCD;
    };

    float currentPressure() {
        if (sampleCount == 0)
            return -1;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesP[i];
        return sum / sampleCount;
    }

    float currentPressureHPa() {
        float ret = currentPressure();
        if (ret == -1) return -1;
        return ret/100.0;
    }

    float currentHumidity() {
        if (sampleCount == 0)
            return -1;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesH[i];
        return sum / sampleCount;
    }

    float currentTemp() {
        if (sampleCount == 0)
            return -128;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesT[i];
        return sum / sampleCount;
    }

    //display on 4 row LCD, no need to split display
    void lcd4rows(String lines[]) {
        byte row = 0;
        lines[row++].concat(FPSTR(SENSORS_BME280));

        lines[row].concat(F("Temp: "));
        lines[row].concat(String(currentTemp()));
        lines[row++].concat(FPSTR(UNIT_CELCIUS_LCD));
        lines[row].concat(F("Hum.: "));
        lines[row].concat(String(currentHumidity()));
        lines[row++].concat(F(" %"));
        lines[row] = F("Press.:");
        lines[row].concat(currentPressureHPa());
        lines[row].concat(F(" hPa"));
        return;
    }

    void lcd2rows(byte minor, String lines[]) {
        byte row = 0;
//        lines[row++].concat(FPSTR(SENSORS_BME280));
        if (minor == 0) {
            lines[row].concat(F("T: "));
            lines[row].concat(String(currentTemp()));
            lines[row++].concat(FPSTR(UNIT_CELCIUS_LCD));
            lines[row].concat(F("H: "));
            lines[row].concat(String(currentHumidity()));
            lines[row].concat(F(" %"));

        } else {
            lines[row++] = F("Pressure:");
            lines[row].concat(currentPressureHPa());
            lines[row].concat(F(" hPa"));
        }
        return;

    }

        void display(byte cols, byte minor, String lines[]) {
        if (getLCDRows() > 2) {
            lcd4rows(lines);
        } else {
            lcd2rows(minor, lines);
        }
        return;
    };


    bool initBME280() {
        if (initBME280(0x76) || initBME280(0x77)) {
            bme280_init_failed = false;
            return true;
        }
        bme280_init_failed = true;
        return false;
    }


    void readFromSensor() {
        if (sampleCount >= SAMPLE_SIZE)
            return;

        float p = -1;
        float t = -128;
        float h = -1;
        if (bme280.takeForcedMeasurement()) {
            p = bme280.readPressure();
            t = bme280.readTemperature();
            h = bme280.readHumidity();
        }
        if (isnan(p) || isnan(t) || isnan(h)) {
            t = -128.0;
            p = -1.0;
            h = -1;
            debug_out(String(FPSTR(SENSORS_BME280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
        } else {
            samplesP[sampleCount] = p;
            samplesT[sampleCount] = t;
            samplesH[sampleCount] = h;
            sampleCount++;
        }
    }



    /*****************************************************************
     * read BMP280 sensor values                                     *
     *****************************************************************/

    void sendToLD() {
        const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);

        String result;
        results(result);
        sendLuftdaten(result, BME280_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "BME280_");
    }

    String getConfigJSON() {
        String ret = F("");
        ret.concat(Var2JsonInt(F("e"), enabled));
        if (printOnLCD) ret.concat(Var2JsonInt(F("d"), printOnLCD));
        return ret;
    };

    bool dataAvailable() {
        return sampleCount > 0;
    }

    void results(String &s) {
        if (!enabled) return;
        if (!dataAvailable()) return;

        s.concat(Value2Json(F("BME280_temperature"), String(currentTemp())));
        s.concat(Value2Json(F("BME280_pressure"), String(currentPressure())));
        s.concat(Value2Json(F("BME280_humidity"), String(currentHumidity())));
    }

    void resultsAsHTML(String &page_content) {
        if (!enabled) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_TEMPERATURE),
                                                 check_display_value(currentTemp(), -128, 1, 0), F("°C")));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_PRESSURE),
                                                 check_display_value(currentPressureHPa(), -1, 1, 0), F("hPa")));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_HUMIDITY),
                                                 check_display_value(currentHumidity(), -1, 1, 0), F("%")));


    }

    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::BME280);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::BME280);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        return ret;
    };

    void afterSend(bool status) {
        if (!enabled) return;
        debug_out(F("BME280 - reset stats"), DEBUG_MED_INFO);
        last_value_BME280_P = currentPressure();
        last_value_BME280_T = currentTemp();
        last_value_BME280_H = currentHumidity();
        sampleCount = 0;
    }

    unsigned long interval() {
        unsigned long ret = cfg::sending_intervall_ms;
        if (ret > 10000) ret -= 10000;
        return ret / SAMPLE_SIZE;
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                initBME280();
                if (readyBME280()) {
                    samplesT = new float[SAMPLE_SIZE];
                    samplesP = new float[SAMPLE_SIZE];
                    samplesH = new float[SAMPLE_SIZE];
                    sampleCount = 0;
                    readFromSensor();
                }

                return interval();
            case SimpleScheduler::RUN:
                if (!readyBME280()) {
                    return 10000;
                }
                readFromSensor();
                auto left = time2Measure();
                if (left == 0) left = 100;
                if (sampleCount >= SAMPLE_SIZE) return left;  // we are full wait till period end
                return interval();
        }
        return 10000;

    }

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));

        //register/deregister sensor
        if (enabled && !scheduler.isRegistered(SimpleScheduler::BME280)) {
            scheduler.registerSensor(SimpleScheduler::BME280, BME280::process, FPSTR(BME280::KEY));
            scheduler.init(SimpleScheduler::BME280);
            enabled = true;
            debug_out(F("BME280: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::BME280)) {   //de
            debug_out(F("BME280: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::BME280);
        }
        //register display - separate check to allow enable/disable display not only when turning BME280 on/off

        if (enabled && printOnLCD) {
            byte screens = getLCDRows() > 2 ? 1 : 2;
            scheduler.registerDisplay(SimpleScheduler::BME280, screens);
        }
    }
}
