#include "bmpX80.h"


namespace BMPx80 {
    const char KEY[] PROGMEM = "BMPx80";
    bool enabled = false;
    bool printOnLCD = false;
    bool sensorInsideCase = false;
    const unsigned int SAMPLE_SIZE = 5;
    byte sampleCount = 0;

    float *samplesT = nullptr;
    float *samplesP = nullptr;


    Adafruit_BMP280 bmp280;
    Adafruit_BMP085 bmp180;

    typedef enum {
        BMP_180,
        BMP_280,
        NONE
    } SensorType;

    SensorType currentSensor = NONE;


    //ser defaults
    void setDefaults(void) {
        enabled = true;
    }

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


    const char *sensorPrefixBMPx80() {
        static char buff[8];
        switch (currentSensor) {
            case NONE:
                String(F("")).toCharArray(buff, 8);
                break;
            case BMP_280:
                String(F("BMP280_")).toCharArray(buff, 8);
                break;
            case BMP_180:
                String(F("BMP_")).toCharArray(buff, 8);
                break;
        }
        return buff;
    }

    void readFromSensor() {
        if (sampleCount >= SAMPLE_SIZE)
            return;

        float p = -1;
        float t = -128;
        switch (currentSensor) {
            case NONE:
                return;
            case BMP_180:
                p = (float) bmp180.readPressure();
                t = bmp180.readTemperature();
                break;
            case BMP_280:
                p = bmp280.readPressure();
                t = bmp280.readTemperature();
                break;
        }
        if (isnan(p) || isnan(t)) {
            t = -128.0;
            p = -1.0;
            debug_out(String(FPSTR(SENSORS_BMP280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
        } else {
            samplesP[sampleCount] = p;
            samplesT[sampleCount] = t;
            sampleCount++;
        }
    }

    float currentPressure() {
        if (sampleCount == 0)
            return -1;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesP[i];
        return sum/sampleCount;
    }

    float currentTemp() {
        if (sampleCount == 0)
            return -128;
        float sum = 0;
        for (byte i = 0; i < sampleCount; i++)
            sum += samplesT[i];
        return sum/sampleCount;
    }

    void sendToLD(){
        const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);

        String result;
        results(result);
        sendLuftdaten(result, BMP280_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, sensorPrefixBMPx80());
    }

    String getConfigJSON() {
        String ret = F("");
        ret.concat(Var2JsonInt(F("e"), enabled));
        if (printOnLCD) ret.concat(Var2JsonInt(F("d"), printOnLCD));
        if (sensorInsideCase) ret.concat(Var2JsonInt(F("i"), sensorInsideCase));
        return ret;
    };

    bool dataAvailable() {
        return sampleCount > 0;
    }

    void results(String &s) {
        if (!enabled) return;
        if (!dataAvailable()) return;

        String key2 = sensorPrefixBMPx80();
        key2.concat(F("pressure"));
        s.concat(Value2Json(key2, String(currentPressure())));

        if (!sensorInsideCase) {
            String key1 = sensorPrefixBMPx80();
            key1.concat(F("temperature"));
            s.concat(Value2Json(key1, String(currentTemp())));
        }
    }

    void resultsAsHTML(String &page_content){
        if (!enabled) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_TEMPERATURE),
                                                 check_display_value(currentTemp(), -128, 1, 0), F("°C")));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_PRESSURE),
                                                 check_display_value(currentPressure()/100, -1, 1, 0), F("hPa")));
    }

    String getConfigHTML(void) {
        String ret = F("");
        String name;
        setHTTPVarName(name, F("in"), SimpleScheduler::BMPx80);
        ret.concat(formCheckboxGrid(name, FPSTR(INTL_BMPx80_INSIDE), sensorInsideCase));
        return ret;
    }

    void getStatusReport(String &res) {
        if (!enabled) return;
        res.concat(FPSTR(EMPTY_ROW));
        res.concat(table_row_from_value(F("BMx80"), F("Sensor type"), String(currentSensor), ""));

    }

    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::BMPx80);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::BMPx80);
        setBoolVariableFromHTTP(String(F("in")), sensorInsideCase, SimpleScheduler::BMPx80);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        ret[F("i")] = sensorInsideCase;
        return ret;
    };

    void afterSend(bool status) {
        if (!enabled) return;
        debug_out(F("BMPx80 - reset stats"),DEBUG_MED_INFO);
        last_value_BMP280_P = currentPressure();
        last_value_BMP280_T = currentTemp();
        sampleCount = 0;
    }


    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                initBMPx80();
                if (readyBMPx80()) {
                    samplesT = new float[SAMPLE_SIZE];
                    samplesP = new float[SAMPLE_SIZE];
                    readFromSensor();
                }
                return time2Measure() - 5000 -
                       SAMPLE_SIZE * 5000;    //do not deregister - on status page we can write about failure
            case SimpleScheduler::RUN:
                if (!readyBMPx80()) {
                    return 10000;
                }
                readFromSensor();
                return 5000;
        }
        return 10000;

    }

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));
        sensorInsideCase = json.get<bool>(F("i"));

        if (cfg::bmp280_read) { //old setting takes over
            enabled = true;
        }

        //register/deregister sensor
        if (enabled && !scheduler.isRegistered(SimpleScheduler::BMPx80)) {
            scheduler.registerSensor(SimpleScheduler::BMPx80, BMPx80::process, FPSTR(BMPx80::KEY));
            scheduler.init(SimpleScheduler::BMPx80);
            enabled = true;
            debug_out(F("BMPx80: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::BMPx80)) {   //de
            debug_out(F("BMPx80: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::BMPx80);
        }
        //register display - separate check to allow enable/disable display not only when turning BMPx80 on/off
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::BMPx80, 1);
    }
}
