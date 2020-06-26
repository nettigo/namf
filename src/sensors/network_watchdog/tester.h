//
// Created by viciu on 26.06.2020.
//

#ifndef NAMF_TESTER_H
#define NAMF_TESTER_H

#include <Arduino.h>
#include "../../variables.h"
#include "../../defines.h"
#include "../../helpers.h"
#include <ESP8266WiFi.h>
#include "AsyncPing.h"


namespace NetworkWatchdog {
    extern const char KEY[] PROGMEM;
    extern String getConfigHTML(void);
    extern JsonObject & parseHTTPRequest(void);
}

#endif //NAMF_TESTER_H
