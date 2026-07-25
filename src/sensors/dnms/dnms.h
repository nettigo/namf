//
// Created by viciu on 2/8/25.
//

#ifndef NAMF_DNMS_H
#define NAMF_DNMS_H

#include <Arduino.h>
#include "system/scheduler.h"
#include "helpers.h"
#include "html-content.h"
#include "sending.h"


namespace DNMS {
    extern const char KEY[] PROGMEM;
    extern bool enabled;

    JsonDocument parseHTTPRequest();
    String getConfigJSON();
    void getStatusReport(String &res);
    void readConfigJSON(JsonDocument json);
    unsigned long process(SimpleScheduler::LoopEventType);
    void resultsAsHTML(String &);
    void afterSend(bool status);
    void results(String &s);
    void display(byte, byte, String []);
    int16_t readVersion(char *);
}

#endif //NAMF_DNMS_H
