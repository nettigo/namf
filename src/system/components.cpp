#include <Arduino.h>
#include "components.h"


namespace SimpleScheduler {

    bool sensorWantsDisplay(LoopEntryType sensor){
        switch (sensor) {
            case SimpleScheduler::SDS011:
                return SDS011::getDisplaySetting();
            case SimpleScheduler::SPS30:
                return SPS30::getDisplaySetting();
            case SimpleScheduler::MHZ14A:
                return MHZ14A::getDisplaySetting();
            default:
                return false;
        }
    };

    //collect results as JSON
    void getResults(String &res) {
        SDS011::results(res);
        SPS30::results(res);
        SHT3x::results(res);
        MHZ14A::getResults(res);

    }

    //push results to Luftdaten/SensorCommunity
    void sendToSC(void) {
        if (cfg::send2dusti) {
            SPS30::sendToLD();
            SHT3x::sendToLD();
            MHZ14A::sendToLD();
            SDS011::sendToLD();
        }

    }

    //did all API collect data?
    void afterSendData(bool status) {
        SPS30::afterSend(status);
        SHT3x::afterSend(status);

    }

    //collect HTML table with current results
    void getResultsAsHTML(String &res) {
        SDS011::resultsAsHTML(res);
        SPS30::resultsAsHTML(res);
        SHT3x::resultsAsHTML(res);
        MHZ14A::resultsAsHTML(res);
    }

    //collect sensors status
    void getStatusReport(String &res){
        SDS011::getStatusReport(res);
        NetworkWatchdog::resultsAsHTML(res);

    }

    //prepare forms with configuration
    String selectConfigForm(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SDS011:
                return SDS011::getConfigHTML();
            case SimpleScheduler::SPS30:
                return SPS30::getConfigHTML();
            case SimpleScheduler::NTW_WTD:
                return NetworkWatchdog::getConfigHTML();
            case SimpleScheduler::SHT3x:
                return SHT3x::getConfigHTML();
            default:
                return s;
        }
    }

    JsonObject& parseHTTPConfig(LoopEntryType sensor) {

        switch (sensor) {
            case SimpleScheduler::SDS011:
                return SDS011::parseHTTPRequest();
            case SimpleScheduler::SPS30:
                return SPS30::parseHTTPRequest();
            case SimpleScheduler::NTW_WTD:
                return NetworkWatchdog::parseHTTPRequest();
            case SimpleScheduler::SHT3x:
                return SHT3x::parseHTTPRequest();
            case SimpleScheduler::MHZ14A:
                return MHZ14A::parseHTTPRequest();
            default:
                StaticJsonBuffer<16> jsonBuffer;    //empty response
                JsonObject & ret = jsonBuffer.createObject();
                return ret;
        }
    }

    String getConfigJSON(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::SDS011:
                return SDS011::getConfigJSON();
            case SimpleScheduler::SPS30:
                return SPS30::getConfigJSON();
            case SimpleScheduler::NTW_WTD:
                return NetworkWatchdog::getConfigJSON();
            case SimpleScheduler::SHT3x:
                return SHT3x::getConfigJSON();
            case SimpleScheduler::MHZ14A:
                return MHZ14A::getConfigJSON();
            default:
                return s;
        }
    }

    void readConfigJSON(LoopEntryType sensor, JsonObject &json) {
        switch (sensor) {
            case SimpleScheduler::SDS011:
                SDS011::readConfigJSON(json);
                return;
            case SimpleScheduler::SPS30:
                SPS30::readConfigJSON(json);
                return;
            case SimpleScheduler::NTW_WTD:
                NetworkWatchdog::readConfigJSON(json);
                return;
            case SimpleScheduler::SHT3x:
                SHT3x::readConfigJSON(json);
                return;
            case SimpleScheduler::MHZ14A:
                MHZ14A::readConfigJSON(json);
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
            if (s.length() < 1) continue;    //
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
            case SimpleScheduler::SDS011:
                return FPSTR(SDS011::KEY);
            case SimpleScheduler::SPS30:
                return FPSTR(SPS30::KEY);
            case SimpleScheduler::NTW_WTD:
                return FPSTR(NetworkWatchdog::KEY);
            case SimpleScheduler::SHT3x:
                return FPSTR(SHT3x::KEY);
            case SimpleScheduler::MHZ14A:
                return FPSTR(MHZ14A::KEY);
            default:
                debug_out(F("**** MISSING SENSOR SLOT KEY: "), DEBUG_MIN_INFO, false);
                debug_out(String(sensor), DEBUG_MIN_INFO, true);
                return F("");
        }


    }

    //convert sensor/subsytem type to string with code
    const __FlashStringHelper *findSlotDescription(LoopEntryType sensor) {
        switch (sensor) {
            case SimpleScheduler::SDS011:
                return FPSTR(INTL_SDS011_DESC);
            case SimpleScheduler::SPS30:
                return FPSTR(INTL_SPS30_SENSOR_DESC);
            case SimpleScheduler::NTW_WTD:
                return FPSTR(INTL_NTW_WTD_DESC);
            case SimpleScheduler::SHT3x:
                return FPSTR(INTL_SHT3X_DESC);
            case SimpleScheduler::MHZ14A:
                return FPSTR(INTL_MHZ14A_DESC);
            default:
                return F("");
        }

    }

//check if senor has display subroutine. TODO - second parameter with LCD object
    bool displaySensor(SimpleScheduler::LoopEntryType sensor, LiquidCrystal_I2C *lcd, byte minor){
        switch (sensor) {
            case SDS011:
                return SDS011::display(lcd, minor);
            case SPS30:
                return SPS30::display(lcd, minor);
            case MHZ14A:
                return MHZ14A::display(lcd, minor);
            default:
                return false;
        }
    };
}

