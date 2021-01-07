//
// Created by viciu on 16.04.2020.
//

#ifndef NAMF_DISPLAY_COMMONS_H
#define NAMF_DISPLAY_COMMONS_H
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "variables.h"
#include "system/components.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"

void display_values();
String check_display_value(double value, double undef, uint8_t len, uint8_t str_len);
//get LCD screen sizes. returns 0 if no LCD or graphical one (SSD1306)
byte getLCDCols();
byte getLCDRows();
#endif //NAMF_DISPLAY_COMMONS_H
