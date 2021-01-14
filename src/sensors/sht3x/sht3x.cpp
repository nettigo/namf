//
// Created by viciu on 04.09.2020.
//
#include "sht3x.h"
#include <Arduino.h>
#include "ClosedCube_SHT31D.h"


ClosedCube_SHT31D sht30;

namespace SHT3x {
    const char KEY[] PROGMEM = "SHT3x";
    bool enabled = false;
    long temp;
    unsigned long rh;
    unsigned cnt;

    bool initSensor() {
        sht30.begin(0x45);  //on 0x44 is HECA, which uses SHT30 internally
        if (sht30.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) != SHT3XD_NO_ERROR) {
            return false;
        }
        return true;
    }
    float currentTemp(void) {
        if (cnt == 0) { return -128;}
        return ((float)temp/100.0)/(float)cnt;
    }

    float currentRH(void) {
        if (cnt == 0) { return -1;}
        return (float)rh/10.0/(float)cnt;
    }


    String getConfigHTML() {
        String s = F("");

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
        scheduler.enableSubsystem(SimpleScheduler::SHT3x, enabled, SHT3x::process, FPSTR(SHT3x::KEY));
    }

    unsigned long process (SimpleScheduler::LoopEventType event){
        switch(event) {
            case SimpleScheduler::INIT:
                temp = 0;
                rh = 0;
                cnt = 0;
                if (initSensor()) {
                    return 1000;
                } else {
                    return 0;
                }
            case SimpleScheduler::RUN:
                SHT31D result = sht30.periodicFetchData();
                if (result.error == SHT3XD_NO_ERROR) {
//                    Serial.print (F("SHT3x:   "));
//                    Serial.print ((int) (result.t * 100));
//                    Serial.print (F(", "));
//                    Serial.println (result.rh);

                    temp += (int) (result.t * 100);
                    rh += (int) (result.rh * 10);
                    cnt++;
//                    Serial.println(currentTemp());
                }
                return (15 * 1000);
        }
        return 1000;
    };

    void resultsAsHTML(String &page_content) {
        if (!enabled) {return;}
        page_content += FPSTR(EMPTY_ROW);
        if (cnt == 0) {
            page_content += table_row_from_value(FPSTR("SHT3X"), FPSTR(INTL_SPS30_NO_RESULT), F(""), "");
        } else {
            page_content += F("<tr><td colspan='3'>");
            page_content += FPSTR(INTL_SHT3X_RESULTS);
            page_content += F("</td></tr>\n");

            page_content += table_row_from_value(F("SHT30"), FPSTR(INTL_SHT3x_TEMP), String(currentTemp()), F(" °C"));
            page_content += table_row_from_value(F("SHT30"), FPSTR(INTL_SHT3x_HUM), String(currentRH()), F(" %"));

        }
    }

    void results(String &s) {
        if (!enabled || cnt == 0) { return; }
        String tmp;
        tmp.reserve(64);
        tmp = Value2Json(F("SHT30_T"), String(currentTemp()));
        tmp += Value2Json(F("SHT30_RH"), String(currentRH()));
        s += tmp;
    }

    void afterSend(bool success) {
        temp = 0;
        rh = 0;
        cnt = 0;
    }

}
