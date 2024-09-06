//
// Created by viciu on 16.04.2020.
//

#ifndef NAMF_DISPLAY_COMMONS_H
#define NAMF_DISPLAY_COMMONS_H
#include <Arduino.h>
#include <time.h>
#include <LiquidCrystal_I2C.h>
#include "variables.h"
#include "system/components.h"
#include "defines.h"
#include "helpers.h"
#include "html-content.h"

//extern unsigned long next_display_millis ;
//extern unsigned long next_display_count ;
//extern unsigned static_screen_count;

void display_values();
String check_display_value(double value, double undef, uint8_t len, uint8_t str_len);
//get LCD screen sizes. returns 0 if no LCD or graphical one (SSD1306)
byte getLCDCols();
byte getLCDRows();
//scan I2C for proper LCD addr
byte getLCDaddr();
//returns "x/y" where x is current screen being displayed and y total count
String getLCDHeader(bool longDisp = true);
//should we display? Should we draw new screen?
void cycleDisplay();
//create "progress bar"
void initCustomChars();
void displaySendSignal();

#endif //NAMF_DISPLAY_COMMONS_H
