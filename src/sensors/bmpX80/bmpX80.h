
#ifndef NAMF_SENSORS_BMPX80_H
#define NAMF_SENSORS_BMPX80_H
#include <Arduino.h>
#include "system/scheduler.h"
#include "helpers.h"
namespace BMPx80 {
    extern const char KEY[] PROGMEM;

    JsonObject &parseHTTPRequest();


}

#endif //NAMF_SENSORS_BMPX80_H
