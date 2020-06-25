#include <Arduino.h>
#include "components.h"


namespace SimpleScheduler {

    //collect results as JSON
    void getResults(String &res) {
        SPS30::results(res);
    }

    //did all API collect data?
    void afterSendData(bool status) {
        SPS30::afterSend(status);
    }

    //collect HTML table with current results
    void getResultsAsHTML(String &res) {
        SPS30::resultsAsHTML(res);
    }

    //prepare forms with configuration
    String selectConfigForm(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return SPS30::getConfigHTML();
            default:
                return s;
        }
    }

    String parseHTTPConfig(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return SPS30::parseHTTPRequest();
            default:
                return s;
        }
    }

    String getConfigJSON(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return SPS30::getConfigJSON();
            default:
                return s;
        }
    }

    void readConfigJSON(LoopEntryType sensor, JsonObject &json) {
        switch (sensor) {
            case SimpleScheduler::SPS30:
                SPS30::readConfigJSON(json);
                return;
            default:
                return;
        }
    }

    void readConfigJSON(JsonObject &json) {
        LoopEntryType i = EMPTY;
        i++;
        for (; i < NAMF_LOOP_SIZE; i++) {
            if (json.containsKey(findSlotKey(i))) {
                JsonObject &item = json[findSlotKey(i)];
                readConfigJSON(i, item);
            }
        }
    }

    void getConfigJSON(String &json) {
        String s, s1;
        json += F("\"sensors\":{");
        LoopEntryType i = EMPTY;
        i++;
        for (; i < NAMF_LOOP_SIZE; i++) {
            s = getConfigJSON(i);
            s.remove(s.length() - 1);

            s1 = F("\"{k}\":{{r}},");
            s1.replace(F("{k}"), findSlotKey(i));
            s1.replace(F("{r}"), s);
            json += s1;
        }
        json.remove(json.length() - 1);
        json += F("}");
    }

    const __FlashStringHelper *findSlotKey(LoopEntryType sensor) {
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return FPSTR(SPS30::KEY);
            default:
                return F("");
        }


    }

    const __FlashStringHelper *findSlotDescription(LoopEntryType sensor) {
        switch (sensor) {
            case SimpleScheduler::SPS30:
                return FPSTR(INTL_SPS30_SENSOR_DESC);
            default:
                return F("");
        }

    }


}

