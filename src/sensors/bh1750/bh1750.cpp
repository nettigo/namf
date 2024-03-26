//
// Created by viciu on 3/22/24.
//
#include "bh1750.h"

namespace BH17 {
    const char KEY[] PROGMEM = "BH1750";
    bool enabled = false;
    bool printOnLCD = false;

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

    void initBH1750(void) {

    }
}

    unsigned long process(SimpleScheduler::LoopEventType e){
        switch (e) {
            case SimpleScheduler::INIT:
                initBH1750();
                return 10000;
            default:
                return 0;
    };



}