//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_SDS011_H
#define AIRROHR_FIRMWARE_SDS011_H
//extern SoftwareSerial serialSDS(PM_SERIAL_RX, PM_SERIAL_TX, false, 128);
#include "variables.h"
void readSingleSDSPacket(int *pm10_serial, int *pm25_serial) {
    char buffer;
    int value;
    int len = 0;
    int checksum_is = 0;
    int checksum_ok = 0;


    while (serialSDS.available() > 0) {
        buffer = serialSDS.read();
        debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MAX_INFO, 1);
//			"aa" = 170, "ab" = 171, "c0" = 192
        value = int(buffer);
        switch (len) {
            case (0):
                if (value != 170) {
                    len = -1;
                };
                break;
            case (1):
                if (value != 192) {
                    len = -1;
                };
                break;
            case (2):
                *pm25_serial = value;
                checksum_is = value;
                break;
            case (3):
                *pm25_serial += (value << 8);
                break;
            case (4):
                *pm10_serial = value;
                break;
            case (5):
                *pm10_serial += (value << 8);
                break;
            case (8):
                debug_out(FPSTR(DBG_TXT_CHECKSUM_IS), DEBUG_MED_INFO, 0);
                debug_out(String(checksum_is % 256), DEBUG_MED_INFO, 0);
                debug_out(FPSTR(DBG_TXT_CHECKSUM_SHOULD), DEBUG_MED_INFO, 0);
                debug_out(String(value), DEBUG_MED_INFO, 1);
                if (value == (checksum_is % 256)) {
                    checksum_ok = 1;
                } else {
                    len = -1;
                };
                break;
            case (9):
                if (value != 171) {
                    len = -1;
                };
                break;
        }
        if (len > 2) { checksum_is += value; }
        len++;
        if (len == 10 && checksum_ok == 1 && (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS))) {
            if ((! isnan(*pm10_serial)) && (! isnan(*pm25_serial))) {
                sds_pm10_sum += *pm10_serial;
                sds_pm25_sum += *pm25_serial;
                if (sds_pm10_min > *pm10_serial) {
                    sds_pm10_min = *pm10_serial;
                }
                if (sds_pm10_max < *pm10_serial) {
                    sds_pm10_max = *pm10_serial;
                }
                if (sds_pm25_min > *pm25_serial) {
                    sds_pm25_min = *pm25_serial;
                }
                if (sds_pm25_max < *pm25_serial) {
                    sds_pm25_max = *pm25_serial;
                }
                debug_out(F("PM10 (sec.) : "), DEBUG_MED_INFO, 0);
                debug_out(Float2String(double(*pm10_serial) / 10), DEBUG_MED_INFO, 1);
                debug_out(F("PM2.5 (sec.): "), DEBUG_MED_INFO, 0);
                debug_out(Float2String(double(*pm25_serial) / 10), DEBUG_MED_INFO, 1);
                sds_val_count++;
            }
            len = 0;
            checksum_ok = 0;
            *pm10_serial = 0.0;
            *pm25_serial = 0.0;
            checksum_is = 0;
        }
        yield();
    }


}
#endif //AIRROHR_FIRMWARE_SDS011_H
