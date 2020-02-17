//
// Created by viciu on 17.02.2020.
//

#ifndef NAMF_HECA_H
#define NAMF_HECA_H
#include <Arduino.h>
#include "ClosedCube_SHT31D.h" // support for Nettigo Air Monitor HECA
#include "variables.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"
String sensorHECA();
bool initHECA();
#endif //NAMF_HECA_H
