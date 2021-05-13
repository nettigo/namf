//
// Created by viciu on 10.05.2021.
//
#include "heca.h"
#include "../../../.pio/libdeps/nodemcuv2_en/ClosedCube SHT31D/src/ClosedCube_SHT31D.h"

namespace HECA {
    const char KEY[]
    PROGMEM = "HECA";
    bool enabled = false;
    bool printOnLCD = false;
    ClosedCube_SHT31D heca;

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
        }else if (!enabled && scheduler.isRegistered(SimpleScheduler::HECA)) {
            scheduler.unregisterSensor(SimpleScheduler::HECA);
            debug_out(F("HECA stopped"), DEBUG_MED_INFO);

        }
    };

    void getData() {
        SHT31D_RegisterStatus reg;
        SHT31D result = heca.periodicFetchData();
        reg = heca.readStatusRegister();
        SHT31D alertSetHigh = heca.readAlertHighSet();
        SHT31D alertSetLow = heca.readAlertLowSet();
        SHT31D alertClearHigh = heca.readAlertHighClear();
        SHT31D alertClearLow = heca.readAlertLowClear();
        if (result.error == SHT3XD_NO_ERROR && alertClearLow.error == SHT3XD_NO_ERROR &&
            alertClearHigh.error == SHT3XD_NO_ERROR && alertSetLow.error == SHT3XD_NO_ERROR &&
            alertSetHigh.error == SHT3XD_NO_ERROR) {
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
        } else {
            Serial.println("HECA error");
        }

    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::STOP:
                break;
            case SimpleScheduler::INIT:
                initHECA();
                return 1000;
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
            if (heca.writeAlertHigh(120, 119, 41, 38) != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot set Alert HIGH"), DEBUG_ERROR, 1);
            }
            if (heca.writeAlertLow(5, -5, 0, 1) != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot set Alert LOW"), DEBUG_ERROR, 1);
            }
            if (heca.clearAll() != SHT3XD_NO_ERROR) {
                debug_out(F(" [HECA ERROR] Cannot clear register"), DEBUG_ERROR, 1);
            }
            return true;
        }
    }
}

