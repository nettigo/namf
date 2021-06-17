//
// Created by viciu on 10.06.2021.
//

#include "SerialSDS.h"

void SerialSDS::process() {
    while (_serial.available()) {
        byte x;
        static byte idx;
        switch (_currState) {
            case SER_UNDEF:
                x = _serial.read();
                if (x == 0xAA) {
                    _currState = SER_HDR;
                    idx = 2;
                    _buff[0] = 0xAA;
                }
                break;
            case SER_HDR:
                x = _serial.read();
                _buff[1] = x;
                if (x == 0xC0) {
                    _currState = SER_DATA;
                } else if (x == 0xC5) {
                    _currState = SER_REPLY;
                } else
                    _currState = SER_UNDEF;
                break;
            case SER_REPLY:
                if (idx < 10) {
                    _buff[idx] = _serial.read();
                    idx++;
                }
                if (idx == 10) {
                    if (!checksumValid()) {
                        _currState = SER_UNDEF;
                        break;
                    }
                    storeReply();
                    _currState = SER_UNDEF;
                }
                break;
            case SER_DATA:
                if (idx < 10)
                    _buff[idx++] = _serial.read();
                if (idx == 10) {
                    if (!checksumValid()) {
                        _currState = SER_UNDEF;
                        break;
                    }
                    _replies[SDS_DATA].received = true;
                    _replies[SDS_DATA].lastReply = millis();
                    for (byte i = 0; i < 5; i++) _replies[SDS_DATA].data[i] = _buff[2 + i];
                    if (cfg::debug > DEBUG_MIN_INFO) logReply(SDS_DATA);
                    _currState = SER_UNDEF;
                }
                break;
            default:
                _currState = SER_UNDEF;
        }
    }
}

bool SerialSDS::readingAvailable() {
    return _replies[SDS_DATA].received;
};

bool SerialSDS::fetchReading(int &pm10, int &pm25) {
    if (!readingAvailable()) return false;
    pm25 = _replies[SDS_DATA].data[0] | (_replies[SDS_DATA].data[1] << 8);
    pm10 = _replies[SDS_DATA].data[2] | (_replies[SDS_DATA].data[3] << 8);
    _replies[SDS_DATA].received = false;
    return true;

}

bool SerialSDS::checksumValid(void) {
    uint8_t checksum_is = 0;
    for (unsigned i = 2; i < 8; ++i) {
        checksum_is += _buff[i];
    }
    bool chk = _buff[9] == 0xAB && checksum_is == _buff[8];
    packetCount++;
    if (!chk) {
        debug_out(F("SDS011 reply checksum failed "), DEBUG_ERROR);
        checksumFailed++;
//        Serial.println(checksum_is,16);
//        for (byte i=0; i<10; i++) {
//            Serial.print(_buff[i],16);
//            Serial.print(" ");
//        }
//        Serial.println();
    }
    return (chk);
}


void SerialSDS::logReply(ResponseType type) {
    ReplyInfo x = _replies[type];
//    for (byte i=0; i<5;i++) {Serial.print(x.data[i],16); Serial.print(" ");}
//    Serial.println();
//    Serial.print(F("**** SDS**** Odpowiedź z SDS: "));
    switch (type) {
        case SDS_REPORTING:
            Serial.print(F("REPORTING MODE "));
            Serial.print(x.data[0] ? F("SET: ") : F("QUERY: "));
            Serial.print(x.data[1] ? F("query ") : F("active"));
            break;
        case SDS_DATA:
//            Serial.print(F("data packet"));
            break;
        case SDS_NEW_DEV_ID:
        case SDS_SLEEP:
            Serial.print(F("SLEEP MODE "));
            Serial.print(x.data[0] ? F("SET: ") : F("QUERY: "));
            Serial.print(x.data[1] ? F("work ") : F("sleep"));
            break;
        case SDS_PERIOD:
            Serial.print(F("WORKING PERIOD "));
            Serial.print(x.data[0] ? F("SET: ") : F("QUERY: "));
            Serial.print(x.data[1] ? String(x.data[1]) : F("continous"));
            break;

        case SDS_FW_VER:
            Serial.println(F("FIRMWARE VERSION response"));
            break;

    }
    Serial.println();
}

//store reply for command
void SerialSDS::storeReply() {
    ResponseType type;
    type = selectResponse(_buff[2]);
    debug_out(F("Response type: "), DEBUG_MED_INFO, 0);
    debug_out(String(type), DEBUG_MED_INFO);
    if (type == SDS_UNKNOWN) { return; }
    _replies[type].received = true;
    _replies[type].lastReply = millis();
    for (byte i = 0; i < 5; i++) _replies[type].data[i] = _buff[3 + i];

    if (cfg::debug > DEBUG_MIN_INFO) logReply(type);
}

SerialSDS::ResponseType SerialSDS::selectResponse(byte x) {
    switch (x) {
        case 7:
            return SDS_FW_VER;
        case 8:
            return SDS_PERIOD;
        case 6:
            return SDS_SLEEP;
        case 5:
            return SDS_NEW_DEV_ID;
        case 2:
            return SDS_REPORTING;
        default:
            return SDS_UNKNOWN;
    }
}

//const char SerialSDS::SRT_0 [] PROGMEM = "Reporting mode";
//const char SerialSDS::SRT_1 [] PROGMEM = "Data packet";
//const char SerialSDS::SRT_2 [] PROGMEM = "New ID";
//const char SerialSDS::SRT_3 [] PROGMEM = "Sleep/work";
//const char SerialSDS::SRT_4 [] PROGMEM = "Working period";
//const char SerialSDS::SRT_5 [] PROGMEM = "FW version";
//const char SerialSDS::SRT_6 [] PROGMEM = "Unknown";
//const char *SerialSDS::SRT_NAMES[] PROGMEM = {SerialSDS::SRT_0, SerialSDS::SRT_1, SerialSDS::SRT_2, SerialSDS::SRT_3, SerialSDS::SRT_4, SerialSDS::SRT_5, SerialSDS::SRT_6};
