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

    void readConfigJSON(JsonObject &json);

    unsigned long process(SimpleScheduler::LoopEventType e);

    String getConfigJSON();
    void resultsAsHTML(String &page_content);

}


#endif //NAMF_BH1750_H
