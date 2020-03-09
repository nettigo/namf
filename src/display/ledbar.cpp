//
// Created by irukard on 09.03.2020.
//
#include "ledbar.h"

/*****************************************************************
 * display LEDBAR                                                *
 *****************************************************************/
void displayLEDBar() {

    lightLED(0, 5, 0, 50, 0); // LEDs 1-5 in Green
    
}

/*****************************************************************
 * Light LED I2C command                                         *
 *****************************************************************/

void lightLED(byte mode, byte cnt, byte red, byte green, byte blue) {
    Wire.beginTransmission(0x32);
    Wire.write(mode);
    Wire.write(cnt);
    Wire.write(red);
    Wire.write(green);
    Wire.write(blue);
    Wire.endTransmission();
}