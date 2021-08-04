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

    bool initBME280() {
        if (initBME280(0x76) || initBME280(0x77)) {
            bme280_init_failed = false;
            return true;
        }
        bme280_init_failed = true;
        return false;
    }


    void readFromSensor() {
        Serial.println(F("BME - read from sensor"));
        Serial.println(sampleCount);
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

    float currentTemp() {
        if (sampleCount == 0)
            return -128;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesT[i];
        Serial.print(F("BME curr tem sum: "));
        Serial.println(sum);
        return sum / sampleCount;
    }
   float currentHumidity() {
        if (sampleCount == 0)
            return -1;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesH[i];
        return sum / sampleCount;
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

    void results(String &s) {
        if (!enabled) return;

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



    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                initBME280();
                Serial.print(F("BME init: "));
                Serial.println(bme280_init_failed);
                Serial.println(readyBME280());
                if (readyBME280()) {
                    Serial.println(F("BME - tworzymy tablice"));
                    samplesT = new float[SAMPLE_SIZE];
                    samplesP = new float[SAMPLE_SIZE];
                    samplesH = new float[SAMPLE_SIZE];
                    sampleCount = 0;
                    readFromSensor();
                }
                Serial.print(F("BME - czas do pomiaru"));
                Serial.println(time2Measure());
                Serial.println(time2Measure()- 5000 -
                SAMPLE_SIZE * 5000);


                return time2Measure() - 5000 -
                       SAMPLE_SIZE * 5000;    //do not deregister - on status page we can write about failure
            case SimpleScheduler::RUN:
                if (!readyBME280()) {
                    return 10000;
                }
                readFromSensor();

                return 5000;
        }

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
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::BME280, 1);
    }
}
