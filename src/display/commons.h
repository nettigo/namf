//
// Created by viciu on 16.04.2020.
//

#ifndef NAMF_DISPLAY_COMMONS_H
#define NAMF_DISPLAY_COMMONS_H
#include <Arduino.h>
#include "variables.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"

void display_values();
String check_display_value(double value, double undef, uint8_t len, uint8_t str_len);

#endif //NAMF_DISPLAY_COMMONS_H
