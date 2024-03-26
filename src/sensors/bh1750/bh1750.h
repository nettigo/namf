//
// Created by viciu on 3/22/24.
//

#ifndef NAMF_BH1750_H
#define NAMF_BH1750_H

#include <Arduino.h>
#include "system/scheduler.h"
#include "helpers.h"
#include "html-content.h"
#include "webserver.h"
#include "BH1750.h" //unfortunate sensor class name clash

namespace BH17 {
    extern const char KEY[] PROGMEM;

    JsonObject &parseHTTPRequest();

    void readConfigJSON(JsonObject &);

    unsigned long process(SimpleScheduler::LoopEventType);

    String getConfigJSON();
    void resultsAsHTML(String &);
    void results(String &);

}


#endif //NAMF_BH1750_H
