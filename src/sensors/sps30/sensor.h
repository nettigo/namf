//
// Created by viciu on 08.06.2020.
//

#ifndef NAMF_SENSOR_H
#define NAMF_SENSOR_H

#include "variables.h"
#include "system/scheduler.h"
#include "helpers.h"
#include "html-content.h"
#include <SensirionI2cSps30.h>

#define SPS_MAX_SERIAL_LEN 32

namespace SPS30 {
    extern bool started;
    extern const char KEY[] PROGMEM;
    extern unsigned long refresh;
    extern int16_t ret;
    extern uint8_t auto_clean_days;
    extern uint32_t auto_clean;
    extern unsigned int measurement_count;
    extern char serial[SPS_MAX_SERIAL_LEN];

    typedef struct  {
        float mc1p0 = 0;
        float mc2p5 = 0;
        float mc4p0 = 0;
        float mc10p0 = 0;
        float nc0p5 = 0;
        float nc1p0 = 0;
        float nc2p5 = 0;
        float nc4p0 = 0;
        float nc10p0 = 0;
        float typicalParticleSize = 0;
    }  sps30_measurement;

    extern sps30_measurement sum;


    extern void zeroMeasurementStruct(sps30_measurement &str);

    extern void addMeasurementStruct(sps30_measurement &storage, sps30_measurement reading);

    extern unsigned long init();

    extern unsigned long process(SimpleScheduler::LoopEventType e);

    //we will reset average even on API failure
    extern void afterSend(bool success);
    extern String getConfigHTML(void);
    extern JsonDocument parseHTTPRequest(void);

    void getStatusReport (String &page_content);

    extern String getConfigJSON(void);
    extern void readConfigJSON( JsonDocument);

    //return JSON with results
    extern void results(String &s);
    //send data to LD API...
    extern void sendToLD();

    extern void resultsAsHTML(String &page_content);
    //display on LCD
    extern void display(byte rows, byte minor, String lines[]);
    bool getDisplaySetting();

    }

#endif //NAMF_SENSOR_H
