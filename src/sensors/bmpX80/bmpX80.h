
#ifndef NAMF_SENSORS_BMPX80_H
#define NAMF_SENSORS_BMPX80_H

#include <Arduino.h>
#include "system/scheduler.h"
#include "helpers.h"
#include "ext_def.h"
#include "../../system/debug.h"
#include "html-content.h"
#include "sending.h"

#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>

namespace BMPx80 {
    extern const char KEY[] PROGMEM;

    JsonObject &parseHTTPRequest();

    void readConfigJSON(JsonObject &json);

    unsigned long process(SimpleScheduler::LoopEventType e);

    String getConfigJSON();

    void results(String &);
    void resultsAsHTML(String &);
    void afterSend(bool status);
    void sendToLD();


}

#endif //NAMF_SENSORS_BMPX80_H
