//
// Created by viciu on 04.09.2020.
//
#include "sht3x.h"




namespace SHT3x {
    const char KEY[] PROGMEM = "SHT3x" ;
    bool enabled = false;
    unsigned long temp;
    unsigned long rh;

    String getConfigHTML() {
        String s = F("<h1>SHT3x - czujnik temperatury i wilgotności</h1>Nic nie konfigurujemy. Just works :)");

    }

    JsonObject &parseHTTPRequest(void) {
//        String host;
//        parseHTTP(F("host"), host);
        String sensorID = F("enabled-{s}");
        sensorID.replace(F("{s}"),String(SimpleScheduler::SHT3x));
        parseHTTP(sensorID, enabled);
        StaticJsonBuffer<16> jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        return ret;
    }

    String getConfigJSON(void) {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        return ret;
    }

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));

        if (enabled && !scheduler.isRegistered(SimpleScheduler::SHT3x)) {
            scheduler.registerSensor(SimpleScheduler::SHT3x, SHT3x::process, FPSTR(KEY));
            scheduler.init(SimpleScheduler::SHT3x);

        } else if (scheduler.isRegistered(SimpleScheduler::SHT3x)) {
            scheduler.unregisterSensor(SimpleScheduler::SHT3x);
        }
    }

    unsigned long process (SimpleScheduler::LoopEventType event){
        switch(event) {
            case SimpleScheduler::INIT:
                return 200;
        }
        return 1000;
    };


}
