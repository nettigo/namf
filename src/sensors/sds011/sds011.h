//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_SDS011_H
#define AIRROHR_FIRMWARE_SDS011_H
enum {
    SDS_REPLY_HDR = 10,
    SDS_REPLY_BODY = 8
} SDS_waiting_for;
unsigned long SDS_error_count;

#define UPDATE_MIN(MIN, SAMPLE) if (SAMPLE < MIN) { MIN = SAMPLE; }
#define UPDATE_MAX(MAX, SAMPLE) if (SAMPLE > MAX) { MAX = SAMPLE; }
#define UPDATE_MIN_MAX(MIN, MAX, SAMPLE) { UPDATE_MIN(MIN, SAMPLE); UPDATE_MAX(MAX, SAMPLE); }

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
void SDS_rawcmd(const uint8_t cmd_head1, const uint8_t cmd_head2, const uint8_t cmd_head3) {
    constexpr uint8_t cmd_len = 19;

    uint8_t buf[cmd_len];
    buf[0] = 0xAA;
    buf[1] = 0xB4;
    buf[2] = cmd_head1;
    buf[3] = cmd_head2;
    buf[4] = cmd_head3;
    for (unsigned i = 5; i < 15; ++i) {
        buf[i] = 0x00;
    }
    buf[15] = 0xFF;
    buf[16] = 0xFF;
    buf[17] = cmd_head1 + cmd_head2 + cmd_head3 - 2;
    buf[18] = 0xAB;
    serialSDS.write(buf, cmd_len);
}

bool SDS_cmd(PmSensorCmd cmd) {
    switch (cmd) {
        case PmSensorCmd::Start:
            SDS_rawcmd(0x06, 0x01, 0x01);
            break;
        case PmSensorCmd::Stop:
            SDS_rawcmd(0x06, 0x01, 0x00);
            break;
        case PmSensorCmd::ContinuousMode:
            // TODO: Check mode first before (re-)setting it
            SDS_rawcmd(0x08, 0x01, 0x00);
            SDS_rawcmd(0x02, 0x01, 0x00);
            break;
        case PmSensorCmd::VersionDate:
            SDS_rawcmd(0x07, 0, 0);
            break;
    }

    return cmd != PmSensorCmd::Stop;
}
bool SDS_checksum_valid(const uint8_t (&data)[8]) {
    uint8_t checksum_is = 0;
    for (unsigned i = 0; i < 6; ++i) {
        checksum_is += data[i];
    }
    return (data[7] == 0xAB && checksum_is == data[6]);
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
static void sensorSDS(String& s) {
    if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS) &&
        msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
        if (is_SDS_running) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
        }
    } else {
        if (! is_SDS_running) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Start);
            SDS_waiting_for = SDS_REPLY_HDR;
        }

        while (serialSDS.available() >= SDS_waiting_for) {
            const uint8_t constexpr hdr_measurement[2] = { 0xAA, 0xC0 };
            uint8_t data[8];

            switch (SDS_waiting_for) {
                case SDS_REPLY_HDR:
                    if (serialSDS.find(hdr_measurement, sizeof(hdr_measurement)))
                        SDS_waiting_for = SDS_REPLY_BODY;
                    break;
                case SDS_REPLY_BODY:
                    debug_out(FPSTR(DBG_TXT_START_READING), DEBUG_MAX_INFO, 0);
                    debug_out(FPSTR(SENSORS_SDS011), DEBUG_MAX_INFO, 1);
                    size_t read = serialSDS.readBytes(data, sizeof(data));
//                    for (byte i=0; i< sizeof(data); i++) {
//                        Serial.print(data[i], HEX);
//                        Serial.print(F(","));
//                    }
//                    Serial.println();
//                    Serial.print(F("Readed: "));
//                    Serial.println(read);
                    if (read == sizeof(data) &&  SDS_checksum_valid(data)) {
                        uint32_t pm25_serial = data[0] | (data[1] << 8);
                        uint32_t pm10_serial = data[2] | (data[3] << 8);

                        if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS)) {
                            sds_pm10_sum += pm10_serial;
                            sds_pm25_sum += pm25_serial;
                            UPDATE_MIN_MAX(sds_pm10_min, sds_pm10_max, pm10_serial);
                            UPDATE_MIN_MAX(sds_pm25_min, sds_pm25_max, pm25_serial);
//                            debug_outln_verbose(F("PM10 (sec.) : "), String(pm10_serial / 10.0f));
//                            debug_outln_verbose(F("PM2.5 (sec.): "), String(pm25_serial / 10.0f));
                            sds_val_count++;
                        }
                    }
//                    debug_outln_verbose(FPSTR(DBG_TXT_END_READING), FPSTR(SENSORS_SDS011));
                    SDS_waiting_for = SDS_REPLY_HDR;
                    for (byte i=0; i< sizeof(data); i++) {
                        data[i]=0;
                    }
                    break;
            }
        }
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
            last_value_SDS_P1 = float(sds_pm10_sum) / (sds_val_count * 10.0f);
            last_value_SDS_P2 = float(sds_pm25_sum) / (sds_val_count * 10.0f);
//            debug_outln_info(FPSTR(DBG_TXT_SEP));
            if (sds_val_count < 3) {
                SDS_error_count++;
            }
        } else {
            SDS_error_count++;
        }
        s += Value2Json(F("SDS_P1"), Float2String(last_value_SDS_P1));
        s += Value2Json( F("SDS_P2"), Float2String(last_value_SDS_P2));
        sds_pm10_sum = 0;
        sds_pm25_sum = 0;
        sds_val_count = 0;
        sds_pm10_max = 0;
        sds_pm10_min = 20000;
        sds_pm25_max = 0;
        sds_pm25_min = 20000;
        if ((cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {

            if (is_SDS_running) {
                is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
            }
        }
    }
}


#endif //AIRROHR_FIRMWARE_SDS011_H
