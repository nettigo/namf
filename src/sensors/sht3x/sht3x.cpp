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
    bool printOnLCD = false;
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
        if (cnt == 0) { return -1; }
        return (float) rh / 10.0 / (float) cnt;
    }

    bool getDisplaySetting() {
        return printOnLCD;
    };


    String getConfigHTML() {
        String s = F("");
        return s;

    }

    void display(byte rows, byte minor, String lines[]) {
        byte row = 0;
        if (rows == 4) {
            lines[row++] = FPSTR(SENSOR_SHT3);
        }

        lines[row] += F("T:  ");
        lines[row] += check_display_value(currentTemp(), -128, 1, 0);
        lines[row] += F(" ");
        lines[row] += FPSTR(UNIT_CELCIUS_LCD);

        row++;
        lines[row] += F("RH: ");
        lines[row] += check_display_value(currentRH(), -1, 1, 0);
        lines[row] += F(" ");
        lines[row] += FPSTR(UNIT_PERCENT);
    }

    JsonObject &parseHTTPRequest(void) {
//        String host;
//        parseHTTP(F("host"), host);
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::SHT3x);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::SHT3x);

        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        ret.printTo(Serial);
        return ret;
    }

    String getConfigJSON(void) {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        ret += Var2JsonInt(F("d"), printOnLCD);
        return ret;
    }

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));

        scheduler.enableSubsystem(SimpleScheduler::SHT3x, enabled, SHT3x::process, FPSTR(SHT3x::KEY));
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::SHT3x, 1);

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
            {
                SHT31D result = sht30.periodicFetchData();
                if (result.error == SHT3XD_NO_ERROR) {
                    temp += (int) (result.t * 100);
                    rh += (int) (result.rh * 10);
                    cnt++;
                }
                return (15 * 1000);

            }
            default:
                return 1000;
        }
    };

    void resultsAsHTML(String &page_content) {
        if (!enabled) {return;}
        page_content.concat(FPSTR(EMPTY_ROW));
        if (cnt == 0) {
            page_content.concat(table_row_from_value(FPSTR("SHT3X"), FPSTR(INTL_SPS30_NO_RESULT), F(""), ""));
        } else {
            page_content.concat(F("<tr><td colspan='3'>"));
            page_content.concat(FPSTR(INTL_SHT3X_RESULTS));
            page_content.concat(F("</td></tr>\n"));

            page_content.concat(table_row_from_value(F("SHT30"), FPSTR(INTL_SHT3x_TEMP), String(currentTemp()), F(" °C")));
            page_content.concat(table_row_from_value(F("SHT30"), FPSTR(INTL_SHT3x_HUM), String(currentRH()), F(" %")));

        }
    }

    //send data to LD API...
    void sendToLD(){
        if (!enabled) return;
        const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);
        String data;
        results(data);
        debug_out(F("SHT3x to SensorCommunity"), DEBUG_MIN_INFO, 1);
        sendLuftdaten(data, 7 , HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "SHT3X_");

    };

    void results(String &s) {
        if (!enabled || cnt == 0) { return; }
        String tmp;
        tmp.reserve(64);
        tmp = Value2Json(F("SHT3X_temperature"), String(currentTemp()));
        tmp += Value2Json(F("SHT3X_humidity"), String(currentRH()));
        s += tmp;
    }

    void afterSend(bool success) {
        temp = 0;
        rh = 0;
        cnt = 0;
    }

}
