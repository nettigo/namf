#include <Arduino.h>
#include "components.h"


namespace SimpleScheduler {

    bool sensorWantsDisplay(LoopEntryType sensor){
        switch (sensor) {
            case SimpleScheduler::SHT3x:
                return SHT3x::getDisplaySetting();
            case SimpleScheduler::BME280:
                return BME280::getDisplaySetting();
            case SimpleScheduler::HECA:
                return HECA::getDisplaySetting();
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

    //collect results as JSON. Currently it is called only before sending data, so it can be place where
    //counters are reset and calculations are done
    void getResults(String &res) {
        SDS011::results(res);
        SPS30::results(res);
        SHT3x::results(res);
        MHZ14A::getResults(res);
        HECA::getResults(res);
        BMPx80::results(res);
        BME280::results(res);
    }

    //push results to Luftdaten/SensorCommunity
    void sendToSC(void) {
        if (cfg::send2dusti) {
            SPS30::sendToLD();
            server.handleClient();
            SHT3x::sendToLD();
            server.handleClient();
            MHZ14A::sendToLD();
            server.handleClient();
            SDS011::sendToLD();
            server.handleClient();
            BMPx80::sendToLD();
            server.handleClient();
            BME280::sendToLD();
            server.handleClient();

        }

    }

    //did all API collect data?
    void afterSendData(bool status) {
        HECA::afterSend(status);
        SDS011::afterSend(status);
        SPS30::afterSend(status);
        SHT3x::afterSend(status);
        MHZ14A::afterSend(status);
        BMPx80::afterSend(status);
        BME280::afterSend(status);

    }

    //collect HTML table with current results
    void getResultsAsHTML(String &res) {
        SDS011::resultsAsHTML(res);
        HECA::resultsAsHTML(res);
        SPS30::resultsAsHTML(res);
        SHT3x::resultsAsHTML(res);
        MHZ14A::resultsAsHTML(res);
        BMPx80::resultsAsHTML(res);
        BME280::resultsAsHTML(res);
    }

    //collect sensors status
    void getStatusReport(String &res){
        HECA::getStatusReport(res);
        SPS30::getStatusReport(res);
        SDS011::getStatusReport(res);
        BMPx80::getStatusReport(res);
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
            case SimpleScheduler::BMPx80:
                return BMPx80::getConfigHTML();
            case SimpleScheduler::HECA:
                return HECA::getConfigHTML();
            default:
                return s;
        }
    }

    JsonObject& parseHTTPConfig(LoopEntryType sensor) {

        switch (sensor) {
            case SimpleScheduler::HECA:
                return HECA::parseHTTPRequest();
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
            case SimpleScheduler::BMPx80:
                return BMPx80::parseHTTPRequest();
            case SimpleScheduler::BME280:
                return BME280::parseHTTPRequest();
            default:
                StaticJsonBuffer<16> jsonBuffer;    //empty response
                JsonObject & ret = jsonBuffer.createObject();
                return ret;
        }
    }

    String getConfigJSON(LoopEntryType sensor) {
        String s = F("");
        switch (sensor) {
            case SimpleScheduler::HECA:
                return HECA::getConfigJSON();
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
            case SimpleScheduler::BMPx80:
                return BMPx80::getConfigJSON();
            case SimpleScheduler::BME280:
                return BME280::getConfigJSON();
            default:
                return s;
        }
    }

    void readConfigJSON(LoopEntryType sensor, JsonObject &json) {
        switch (sensor) {
            case SimpleScheduler::HECA:
                HECA::readConfigJSON(json);
                return;
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
            case SimpleScheduler::BMPx80:
                BMPx80::readConfigJSON(json);
                return;
            case SimpleScheduler::BME280:
                BME280::readConfigJSON(json);
                return;
            case SimpleScheduler::BH1750:
                BH17::readConfigJSON(json);
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
            case SimpleScheduler::HECA:
                return FPSTR(HECA::KEY);
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
            case SimpleScheduler::BMPx80:
                return FPSTR(BMPx80::KEY);
            case SimpleScheduler::BME280:
                return FPSTR(BME280::KEY);
            case SimpleScheduler::BH1750:
                return FPSTR(BH17::KEY);
            default:
                debug_out(F("**** MISSING SENSOR SLOT KEY: "), DEBUG_MIN_INFO, false);
                debug_out(String(sensor), DEBUG_MIN_INFO, true);
                return F("");
        }


    }

    //convert sensor/subsytem type to string with code
    const __FlashStringHelper *findSlotDescription(LoopEntryType sensor) {
        switch (sensor) {
            case SimpleScheduler::HECA:
                return FPSTR(INTL_HECA_DESC);
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
            case SimpleScheduler::BMPx80:
                return FPSTR(INTL_BMPx80_DESC);
            case SimpleScheduler::BME280:
                return FPSTR(INTL_BME280_DESC);
            case SimpleScheduler::BH1750:
                return FPSTR(INTL_BH1750_DESC);
            default:
                return F("");
        }

    }

//check if senor has display subroutine. TODO - second parameter with LCD object
    bool displaySensor(SimpleScheduler::LoopEntryType sensor, String (&lines)[4], byte cols, byte rows, byte minor) {
//        Serial.print("displaySensor for: ");
//        Serial.println(LET_NAMES[sensor]);
//        Serial.println(cols);
        switch (sensor) {
            case BME280:
                if (cols == 0) return true;
                BME280::display(rows, minor, lines);
                return true;
            case SHT3x:
                if (cols == 0) return true;
                SHT3x::display(rows, minor, lines);
                return true;
            case HECA:
                if (cols == 0) return true;
                HECA::display(rows, minor, lines);
                return true;
            case SDS011:
                if (cols == 0) return true;   //we are able to do display
                SDS011::display(rows, minor, lines);
                return true;
            case SPS30:
                if (cols == 0) return true;
                SPS30::display(rows, minor, lines);
                return true;
            case MHZ14A:
                if (cols == 0) return true;
                MHZ14A::display(rows, minor, lines);
                return true;
            default:
                return false;
        }

    };
}

