//
// Created by irukard on 09.03.2020.
//

#ifndef NAMF_LEDBAR_H
#define NAMF_LEDBAR_H
#include <Arduino.h>
#include "variables.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"
void displayLEDBar();
void lightLED(byte mode, byte cnt, byte red, byte green, byte blue, byte brightness = 0);
#endif //NAMF_LEDBAR_H
