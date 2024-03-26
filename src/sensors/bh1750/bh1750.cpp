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
    float ambientLight = 0.0;

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
        if (sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
            running = true;
            debug_out(F("BH1750 sensor found and started on 0x23!"), DEBUG_MED_INFO);
            return true;
        }

        if (sensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x5C, &Wire)) {
            debug_out(F("BH1750 sensor found and started on 0x5C!"), DEBUG_MED_INFO);
            running = true;
            return true;
        }

        running = false;
        return false;
    }

    //read and store sensor readings
    void readValues(){
        if (sensor.measurementReady()) {
            ambientLight = sensor.readLightLevel();
        }
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
                                                 String(ambientLight), FPSTR(INTL_AMBIENT_LIGHT_UNIT)));

    }

}