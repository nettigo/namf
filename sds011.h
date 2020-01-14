//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_SDS011_H
#define AIRROHR_FIRMWARE_SDS011_H

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
                debug_out(FPSTR(DBG_TXT_CHECKSUM_IS), DEBUG_MAX_INFO, 0);
                debug_out(String(checksum_is % 256), DEBUG_MAX_INFO, 0);
                debug_out(FPSTR(DBG_TXT_CHECKSUM_SHOULD), DEBUG_MAX_INFO, 0);
                debug_out(String(value), DEBUG_MAX_INFO, 1);
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


/*****************************************************************
 * send SDS011 command (start, stop, continuous mode, version    *
 *****************************************************************/
static bool SDS_cmd(PmSensorCmd cmd) {
    static constexpr uint8_t start_cmd[] PROGMEM = {
            0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0xAB
    };
    static constexpr uint8_t stop_cmd[] PROGMEM = {
            0xAA, 0xB4, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB
    };
    static constexpr uint8_t continuous_mode_cmd[] PROGMEM = {
            0xAA, 0xB4, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0xAB
    };
    static constexpr uint8_t version_cmd[] PROGMEM = {
            0xAA, 0xB4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB
    };
    constexpr uint8_t cmd_len = array_num_elements(start_cmd);

    uint8_t buf[cmd_len];
    switch (cmd) {
        case PmSensorCmd::Start:
            memcpy_P(buf, start_cmd, cmd_len);
            break;
        case PmSensorCmd::Stop:
            memcpy_P(buf, stop_cmd, cmd_len);
            break;
        case PmSensorCmd::ContinuousMode:
            memcpy_P(buf, continuous_mode_cmd, cmd_len);
            break;
        case PmSensorCmd::VersionDate:
            memcpy_P(buf, version_cmd, cmd_len);
            break;
    }
    serialSDS.write(buf, cmd_len);
    return cmd != PmSensorCmd::Stop;
}


/*****************************************************************
 * read SDS011 sensor serial and firmware date                   *
 *****************************************************************/
String SDS_version_date() {
    String s = "";
    char buffer;
    int value;
    int len = 0;
    String version_date = "";
    String device_id = "";
    int checksum_is = 0;
    int checksum_ok = 0;

    debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(DBG_TXT_SDS011_VERSION_DATE), DEBUG_MED_INFO, 1);

    is_SDS_running = SDS_cmd(PmSensorCmd::Start);

    delay(100);

    is_SDS_running = SDS_cmd(PmSensorCmd::VersionDate);

    delay(500);

    while (serialSDS.available() > 0) {
        buffer = serialSDS.read();
        debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MED_INFO, 1);
//		"aa" = 170, "ab" = 171, "c0" = 192
        value = int(buffer);
        switch (len) {
            case (0):
                if (value != 170) {
                    len = -1;
                };
                break;
            case (1):
                if (value != 197) {
                    len = -1;
                };
                break;
            case (2):
                if (value != 7) {
                    len = -1;
                };
                checksum_is = 7;
                break;
            case (3):
                version_date  = String(value);
                break;
            case (4):
                version_date += "-" + String(value);
                break;
            case (5):
                version_date += "-" + String(value);
                break;
            case (6):
                if (value < 0x10) {
                    device_id  = "0" + String(value, HEX);
                } else {
                    device_id  = String(value, HEX);
                };
                break;
            case (7):
                if (value < 0x10) {
                    device_id += "0";
                };
                device_id += String(value, HEX);
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
        if (len == 10 && checksum_ok == 1) {
            s = version_date + "(" + device_id + ")";
            debug_out(F("SDS version date : "), DEBUG_MIN_INFO, 0);
            debug_out(version_date, DEBUG_MIN_INFO, 1);
            debug_out(F("SDS device ID: "), DEBUG_MIN_INFO, 0);
            debug_out(device_id, DEBUG_MIN_INFO, 1);
            len = 0;
            checksum_ok = 0;
            version_date = "";
            device_id = "";
            checksum_is = 0;
        }
        yield();
    }

    debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(DBG_TXT_SDS011_VERSION_DATE), DEBUG_MED_INFO, 1);

    return s;
}


/*****************************************************************
 * read SDS011 sensor values                                     *
 *****************************************************************/
String sensorSDS() {
    String s = "";

    int pm10_serial = 0;
    int pm25_serial = 0;


    debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_SDS011), DEBUG_MED_INFO, 1);
    if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
        if (is_SDS_running) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
        }
    } else {
        if (! is_SDS_running) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Start);
        }

        readSingleSDSPacket(&pm10_serial, &pm25_serial);
    }
    if (send_now) {
        last_value_SDS_P1 = -1;
        last_value_SDS_P2 = -1;
        if (sds_val_count > 2) {
            sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
            sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
            sds_val_count = sds_val_count - 2;
        }
        if (sds_val_count > 0) {
            last_value_SDS_P1 = double(sds_pm10_sum) / (sds_val_count * 10.0);
            last_value_SDS_P2 = double(sds_pm25_sum) / (sds_val_count * 10.0);
            debug_out("PM10:  " + Float2String(last_value_SDS_P1), DEBUG_MIN_INFO, 1);
            debug_out("PM2.5: " + Float2String(last_value_SDS_P2), DEBUG_MIN_INFO, 1);
            debug_out("----", DEBUG_MIN_INFO, 1);
            s += Value2Json("SDS_P1", Float2String(last_value_SDS_P1));
            s += Value2Json("SDS_P2", Float2String(last_value_SDS_P2));
        }
        sds_pm10_sum = 0;
        sds_pm25_sum = 0;
        sds_val_count = 0;
        sds_pm10_max = 0;
        sds_pm10_min = 20000;
        sds_pm25_max = 0;
        sds_pm25_min = 20000;
        if ((cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
        }
    }

    debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_SDS011), DEBUG_MED_INFO, 1);

    return s;
}


#endif //AIRROHR_FIRMWARE_SDS011_H
