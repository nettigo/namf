//
// Created by viciu on 3/22/24.
//
#include "bh1750.h"

namespace BH17 {
    const char KEY[] PROGMEM = "BH1750";
    BH1750 sensor;
    bool enabled = false;
    bool printOnLCD = false;
    bool running = false;
    float ambientLightMax = 0.0;
    float ambientLightMin = 70000;
#define BH1750_SAMPLE_SIZE   10
    float *samples = nullptr;
    unsigned int samplesCount;

    void resetStats() {
        samplesCount = 0;
        ambientLightMax = 0;
        ambientLightMin = 70000;
    }

    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::BH1750);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::BH1750);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        return ret;
    };

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));

        //register/deregister sensor
        if (enabled && !scheduler.isRegistered(SimpleScheduler::BH1750)) {
            scheduler.registerSensor(SimpleScheduler::BH1750, BH17::process, FPSTR(BH17::KEY));
            scheduler.init(SimpleScheduler::BH1750);
            enabled = true;
            debug_out(F("BH1750: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::BH1750)) {   //de
            debug_out(F("BH1750: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::BH1750);
        }
        //register display - separate check to allow enable/disable display not only when turning BH1750 on/off

        if (enabled && printOnLCD) {
            scheduler.registerDisplay(SimpleScheduler::BH1750, 1);
        }
    }

    bool initBH1750(void) {
        running = false;
        if (sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
            running = true;
            debug_out(F("BH1750 sensor found and started at 0x23!"), DEBUG_MED_INFO);
        }

        if (!running && sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
            debug_out(F("BH1750 sensor found and started at 0x5C!"), DEBUG_MED_INFO);
            running = true;
        }
        if (running){
            samples = new (std::nothrow) float[BH1750_SAMPLE_SIZE];
            if (samples == nullptr) {
                debug_out(F("BH1750 - no array for data"), DEBUG_ERROR);
                running = false;
            }
            samplesCount = 0;
        }
        return running;
    }

    //read and store sensor readings
    void readValues(){
        static unsigned long lastRead = millis();
        static unsigned long interval = cfg::sending_intervall_ms / (BH1750_SAMPLE_SIZE + 2);
        if (
                sensor.measurementReady() &&
                samplesCount < BH1750_SAMPLE_SIZE &&
                millis() - lastRead > interval
                ) {
            samples[samplesCount] = sensor.readLightLevel();
            debug_out(F("BH1750: readLightLevel "),DEBUG_MED_INFO,0);
            debug_out(String(samples[samplesCount]),DEBUG_MED_INFO,1);
            if (samples[samplesCount] > ambientLightMax)
                ambientLightMax = samples[samplesCount];
            if (samples[samplesCount] < ambientLightMin)
                ambientLightMin = samples[samplesCount];

            samplesCount++;
            lastRead = millis();
        }
    }

    float currentReading() {
        float sum = 0;
        if (samplesCount == 0) return 0;

        for (byte i = 0; i < samplesCount; i++)
            sum += samples[i];
        return sum /(float)samplesCount;
    }

    float maxVal(){
        if (samplesCount == 0) {
            return 0;
        }
        return ambientLightMax;
    }

    float minVal(){
        if (samplesCount == 0) {
            return 0;
        }
        return ambientLightMin;
    }

    void afterSend(bool status) {
        resetStats();
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                if (initBH1750())
                    return 10000;
                else {
                    debug_out(F("!!!! BH1750 sensor init failed...."), DEBUG_ERROR, 1);
                }
                return 0;
            case SimpleScheduler::RUN:
                readValues();
                return 10000;
            default:
                return 0;
        };
    }

    void resultsAsHTML(String &page_content) {
        if (!enabled || !running) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_AMBIENT_LIGHT),
                                                 String(currentReading()), F("lx")));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_AMBIENT_LIGHT_MIN),
                                                 String(minVal()), F("lx")));
        page_content.concat(table_row_from_value(FPSTR(KEY), FPSTR(INTL_AMBIENT_LIGHT_MAX),
                                                 String(maxVal()), F("lx")));

    }

    void results(String &s) {
        if (!enabled) return;
        if (!running) return;

        s.concat(Value2Json(F("ambient_light"), String(currentReading())));
    }

    String getConfigJSON() {
        String ret = F("");
        ret.concat(Var2JsonInt(F("e"), enabled));
        if (printOnLCD) ret.concat(Var2JsonInt(F("d"), printOnLCD));
        return ret;
    };


}