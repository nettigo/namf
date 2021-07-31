//
// Created by viciu on 10.05.2021.
//
#include "heca.h"
#include "../../helpers.h"

#define HECA_DATAPOINTS 20

namespace HECA {
    const char KEY[]
    PROGMEM = "HECA";
    bool enabled = false;
    bool printOnLCD = false;
    ClosedCube_SHT31D heca;
    unsigned long lastDatapoint = 0;    //when last time we have saved T & RH to average
    unsigned long interval = 0;
    double rh_total, t_total;
    unsigned dataCount, dutyCycleCount, dutyCycleValT, dutyCycleValRH, dutyCycleTotal;



    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::HECA);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::HECA);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        return ret;
    };

    String getConfigJSON(){
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        if (printOnLCD) ret += Var2JsonInt(F("d"), printOnLCD);
        return ret;
    };

    void readConfigJSON( JsonObject &json){
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));
        if (enabled && !scheduler.isRegistered(SimpleScheduler::HECA)) {
            scheduler.registerSensor(SimpleScheduler::HECA, HECA::process, FPSTR(HECA::KEY));
            scheduler.init(SimpleScheduler::HECA);
            debug_out(F("HECA started"), DEBUG_MED_INFO);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::HECA)) {
            scheduler.unregisterSensor(SimpleScheduler::HECA);
            debug_out(F("HECA stopped"), DEBUG_MED_INFO);
        }
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::HECA, 1);
    };

    bool display(byte rows, byte minor, String lines[]) {
        byte row = 0;
        if (getLCDRows() == 4) {
            lines[row] += (FPSTR(SENSORS_HECA));
        }
        row++;
        lines[row] += (F("RH: "));
        lines[row] += (check_display_value(last_value_HECA_H, -1, 1, 0));
        lines[row] += (F(" "));
        lines[row] += (FPSTR(UNIT_PERCENT));
        row++;
        lines[row] += (F("T: "));
        lines[row] += (check_display_value(last_value_HECA_T, -128, 1, 0));
        lines[row] += (F(" "));
        lines[row] += (FPSTR(UNIT_CELCIUS_LCD));
//        lines[row] += (F(" µg/m³"));
        if (getLCDRows() == 4) {
            row++;
            lines[row] += (FPSTR(INTL_HECA_DC));
            lines[row] += (F(" "));
            lines[row] += (String(getDutyCycle(),1));
            lines[row] += (F(" "));
            lines[row] += (FPSTR(UNIT_PERCENT));

        }
        return true;
    }

    void initCycle() {
        t_total = rh_total = 0;
        dutyCycleCount = dutyCycleValT = dutyCycleValRH = dataCount = dutyCycleTotal = 0;
    }

    void getData() {
        SHT31D_RegisterStatus reg;
        SHT31D result = heca.periodicFetchData();
        reg = heca.readStatusRegister();
//        SHT31D alertSetHigh = heca.readAlertHighSet();
//        SHT31D alertSetLow = heca.readAlertLowSet();
//        SHT31D alertClearHigh = heca.readAlertHighClear();
//        SHT31D alertClearLow = heca.readAlertLowClear();
        if (result.error == SHT3XD_NO_ERROR /*&& alertClearLow.error == SHT3XD_NO_ERROR &&
            alertClearHigh.error == SHT3XD_NO_ERROR && alertSetLow.error == SHT3XD_NO_ERROR &&
            alertSetHigh.error == SHT3XD_NO_ERROR*/) {
            if (millis() - lastDatapoint > interval) {
                lastDatapoint = millis();
                t_total += result.t;
                rh_total += result.rh;
                dataCount++;
            }
            if (reg.rawData & 1<<10)    //Temp tracking alert
                dutyCycleValT++;
            if (reg.rawData & 1<<11)    //RH tracking alert
                dutyCycleValRH++;
            if (reg.rawData & 1<<15)
                dutyCycleTotal++;
            dutyCycleCount++;

            /*
            Serial.println("HECA\t\tSetH\tSetL\tClH\tClL\tAlert");
            Serial.print(F("Temp\t"));
            Serial.print(result.t);
            Serial.print(F("\t"));
            Serial.print(alertSetHigh.t);
            Serial.print(F("\t"));
            Serial.print(alertSetLow.t);
            Serial.print(F("\t"));
            Serial.print(alertClearHigh.t);
            Serial.print(F("\t"));
            Serial.print(alertClearLow.t);
            Serial.print(F("\t"));
            Serial.print((reg.rawData & 1<<10) ? 1:0);
            Serial.print((reg.rawData & 1<<15) ? 1:0);


//            Serial.print(F("\t"));
            Serial.println();
            Serial.print(F("RH\t"));
            Serial.print(result.rh);
            Serial.print(F("\t"));
            Serial.print(alertSetHigh.rh);
            Serial.print(F("\t"));
            Serial.print(alertSetLow.rh);
            Serial.print(F("\t"));
            Serial.print(alertClearHigh.rh);
            Serial.print(F("\t"));
            Serial.print(alertClearLow.rh);
            Serial.print(F("\t"));
            Serial.print((reg.rawData & 1<<11) ? 1:0);
            Serial.print((reg.rawData & 1<<15) ? 1:0);
            Serial.print(reg.RH_TrackingAlert);
//            Serial.print(F("\t"));
            Serial.println();
//            Serial.println(reg.rawData,HEX);
//            Serial.println((unsigned long)(reg.rawData+65536), BIN);
//            Serial.println(" EDCBA09876543210");
             */
        } else {
            Serial.println("HECA error");
        }

    }

    void afterSend(bool status) {
        initCycle();
    }

    float getDutyCycle() {
        if (dutyCycleCount == 0) return 0.0;
        return (float)dutyCycleTotal/dutyCycleCount*100;
    }

    void getResults(String &res){
        if (!enabled) return;
        if (dataCount) {
            last_value_HECA_H = rh_total/dataCount;
            last_value_HECA_T = t_total/dataCount;
            res += Value2Json(F("HECA_temperature"), Float2String(last_value_HECA_T));
            res += Value2Json(F("HECA_humidity"), Float2String(last_value_HECA_H));
            if (dutyCycleCount) {
                res += Value2Json(F("HECA_Tdc"), String((float)dutyCycleValT/dutyCycleCount*100));
                res += Value2Json(F("HECA_RHdc"), String((float)dutyCycleValRH/dutyCycleCount*100));
                res += Value2Json(F("HECA_dc"), String(getDutyCycle()));
            }
        } else {
            last_value_HECA_T = -128.0;
            last_value_HECA_H = -1.0;
        }
    }

    void resultsAsHTML(String &page_content) {
        if (!enabled) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_HECA_T, -128, 1, 0), FPSTR(UNIT_CELCIUS)));
        page_content.concat(table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_HUMIDITY), check_display_value(last_value_HECA_H, -1, 1, 0), FPSTR(UNIT_PERCENT)));
    }

    void getStatusReport (String &page_content) {
        if (!enabled) return;
        if (dutyCycleCount) {
            page_content.concat(FPSTR(EMPTY_ROW));
            page_content.concat(table_row_from_value(FPSTR(SENSORS_HECA), "DutyCycle", String(getDutyCycle()), "%"));
            page_content.concat(table_row_from_value(FPSTR(SENSORS_HECA), "DutyCycleTemp", String((float)dutyCycleValT/dutyCycleCount*100), FPSTR(UNIT_PERCENT)));
            page_content.concat(table_row_from_value(FPSTR(SENSORS_HECA), "DutyCycleRH", String((float)dutyCycleValRH/dutyCycleCount*100), FPSTR(UNIT_PERCENT)));

        }

    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::STOP:
                break;
            case SimpleScheduler::INIT:
                if (initHECA())
                    return 1000;
                else
                    return 0;
                break;
            case SimpleScheduler::RUN:
                getData();
                return 1000;
                break;


        }
        return 1000;
    }
/*****************************************************************
 * read HECA (SHT30) sensor values                                     *
 *****************************************************************/
    String sensorHECA() {
        String s;

        debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_HECA), DEBUG_MED_INFO, 1);

        SHT31D result = heca.periodicFetchData();
        if (result.error == SHT3XD_NO_ERROR) {
            last_value_HECA_T = result.t;
            last_value_HECA_H = result.rh;
        } else {
            last_value_HECA_T = -128.0;
            last_value_HECA_H = -1.0;
            return s;
        }
        s += Value2Json(F("HECA_temperature"), Float2String(last_value_HECA_T));
        s += Value2Json(F("HECA_humidity"), Float2String(last_value_HECA_H));
        return s;
    }

    bool getDisplaySetting() {
        return printOnLCD;
    };

/*****************************************************************
 * Init HECA                                                     *
 *****************************************************************/

    bool initHECA() {

        debug_out(F("Trying HECA (SHT30) sensor on 0x44"), DEBUG_MED_INFO, 0);
        heca.begin(0x44);
        //heca.begin(addr);
        if (heca.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_1HZ) != SHT3XD_NO_ERROR) {
            debug_out(F(" ... not found"), DEBUG_MED_INFO, 1);
            debug_out(F(" [HECA ERROR] Cannot start periodic mode"), DEBUG_ERROR, 1);
            return false;
        } else {
            // temperature set, temperature clear, humidity set, humidity clear
            if (heca.writeAlertHigh(120, 119, 63, 60) != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot set Alert HIGH"), DEBUG_ERROR, 1);
            }
            if (heca.writeAlertLow(5, -5, 0, 1) != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot set Alert LOW"), DEBUG_ERROR, 1);
            }
            if (heca.clearAll() != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot clear register"), DEBUG_ERROR, 1);
            }
            interval = cfg::sending_intervall_ms / (HECA_DATAPOINTS + 1);
            initCycle();
            return true;
        }
    }
}

