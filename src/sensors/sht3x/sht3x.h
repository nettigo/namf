//
// Created by viciu on 04.09.2020.
//

#ifndef NAMF_SHT3X_H
#define NAMF_SHT3X_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "sending.h"
#include "system/scheduler.h"
#include "system/components.h"

namespace SHT3x {
    extern const char KEY[] PROGMEM;


    extern String getConfigHTML();
    extern JsonObject &parseHTTPRequest(void);
    extern String getConfigJSON(void);
    extern void readConfigJSON(JsonObject &json);
    extern unsigned long process (SimpleScheduler::LoopEventType);
    //send data to LD API...
    extern void sendToLD();
    extern void results (String &s);
    extern void resultsAsHTML(String &page_content);
    extern     void afterSend(bool success);

    }
#endif //NAMF_SHT3X_H
