//
// Created by viciu on 17.02.2020.
//
#include "heca.h"

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
        if (heca.writeAlertHigh(120, 119, 63, 60) != SHT3XD_NO_ERROR) {
            debug_out(F(" [HECA ERROR] Cannot set Alert HIGH"), DEBUG_ERROR, 1);
        }
        if (heca.writeAlertLow(-5, 5, 0, 1) != SHT3XD_NO_ERROR) {
            debug_out(F(" [HECA ERROR] Cannot set Alert LOW"), DEBUG_ERROR, 1);
        }
        if (heca.clearAll() != SHT3XD_NO_ERROR) {
            debug_out(F(" [HECA ERROR] Cannot clear register"), DEBUG_ERROR, 1);
        }
        return true;
    }
}
