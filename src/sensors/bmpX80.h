#ifndef NAMF_BMPX80_H
#define NAMF_BMPX80_H

#include <Arduino.h>
#include "ext_def.h"
#include "../system/debug.h"
#include "helpers.h"
#include "html-content.h"


#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>

    bool initBMPx80();

    String sensorBMPx80();

    bool readyBMPx80();

    const char *sensorPrefixBMPx80();


#endif //NAMF_BMPX80_H
