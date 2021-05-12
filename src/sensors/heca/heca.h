//
// Created by viciu on 10.05.2021.
//

#ifndef NAMF_HECA_H
#define NAMF_HECA_H

#include "Arduino.h"
#include "ClosedCube_SHT31D.h" // support for Nettigo Air Monitor HECA
#include "variables.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"   //for sensor name, to remove after move to new scheduler
#include "system/debug.h"

namespace HECA {
    extern const char KEY[] PROGMEM;
    extern bool enabled;
    String sensorHECA();
    bool initHECA();

    JsonObject& parseHTTPRequest();

    String getConfigJSON();
    void readConfigJSON( JsonObject &json);
    unsigned long process(SimpleScheduler::LoopEventType e);
}

#endif //NAMF_HECA_H
