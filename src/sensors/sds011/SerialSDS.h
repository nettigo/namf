//
// Created by viciu on 10.06.2021.
//

#ifndef NAMF_SERIALSDS_H
#define NAMF_SERIALSDS_H

#include <Arduino.h>
#include "helpers.h"

class SerialSDS {
public:
    typedef enum {
        SER_UNDEF,
        SER_HDR,
        SER_DATA,
        SER_REPLY,
        SER_IDLE
    } SerialState;
    typedef enum {
        SDS_REPORTING,
        SDS_DATA,
        SDS_NEW_DEV_ID,
        SDS_SLEEP,
        SDS_PERIOD,
        SDS_FW_VER,
        SDS_UNKNOWN
    } ResponseType;
    typedef struct {
        bool sent;
        bool received;
        unsigned long lastRequest;
        unsigned long lastReply;
        byte data[5];
    } ReplyInfo;

    SerialSDS(Stream &serial) : _serial(serial) {
        _currState = SER_UNDEF;
        checksumFailed = 0;
        packetCount = 0;
        for (byte i = 0; i < SDS_UNKNOWN; i++) {
            _replies[i].sent = false;
            _replies[i].received = false;
            _replies[i].lastRequest = 0;
            _replies[i].lastReply = 0;

        }
    }

    void process();

    unsigned checksumErrCnt() { return checksumFailed; }

    unsigned long totalPacketCnt() { return packetCount; }

    bool readingAvailable();

    bool fetchReading(int &pm10, int &pm25);

    ReplyInfo _replies[SDS_UNKNOWN];

    //checksum errors rate
    float errorRate();

private:
    Stream &_serial;
    SerialState _currState;
    byte _buff[10];
    unsigned long checksumFailed;
    unsigned long packetCount;

    bool checksumValid();

    void logReply(ResponseType type);

    void clearBuf(void){for (byte i=0; i<10; i++) {_buff[i]=0;}};

    void storeReply();

    ResponseType selectResponse(byte x);
//    const char SRT_0 [];
//    const char SRT_1 [];
//    const char SRT_2 [];
//    const char SRT_3 [];
//    const char SRT_4 [];
//    const char SRT_5 [] PROGMEM = "FW version";
//    const char SRT_6 [] PROGMEM = "Unknown";
//    const char *SRT_NAMES[] PROGMEM = {SRT_0, SRT_1, SRT_2, SRT_3, SRT_4, SRT_5, SRT_6};


};


#endif //NAMF_SERIALSDS_H
