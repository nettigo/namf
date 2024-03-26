//
// Created by viciu on 3/22/24.
//

#ifndef NAMF_BH1750_H
#define NAMF_BH1750_H

#include <Arduino.h>
#include "system/scheduler.h"
#include "helpers.h"

namespace BH1750 {
    extern const char KEY[] PROGMEM;

    JsonObject &parseHTTPRequest();

    void readConfigJSON(JsonObject &json);

    unsigned long process(SimpleScheduler::LoopEventType e);

    String getConfigJSON();

}


#endif //NAMF_BH1750_H
