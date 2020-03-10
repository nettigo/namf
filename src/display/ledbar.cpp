//
// Created by irukard on 09.03.2020.
//
#include "ledbar.h"

/*****************************************************************
 * display LEDBAR                                                *
 *****************************************************************/
void displayLEDBar() {
    
    double pm10 = 0;

    if (cfg::sds_read) {
        pm10 = last_value_SDS_P1;
    } else if (cfg::pms_read) {
        pm10 = last_value_PMS_P1;
    }

    if (pm10 < 1) {
        // disable LED
        lightLED(0, 0, 0, 0, 0);
    } else if ((pm10 >= 1) && (pm10 < 12)) {
        // 1 LEDs in green
        lightLED(0, 1, 0, 150, 0);
    } else if ((pm10 >= 12) && (pm10 < 25)) {        
        // 2 LEDs in green
        lightLED(0, 2, 0, 150, 0);
    } else if ((pm10 >= 25) && (pm10 < 37)) {                
        // 3 LEDs in yellow-green
        lightLED(0, 3, 75, 150, 0);
    } else if ((pm10 >= 37) && (pm10 < 50)) {                
        // 4 LEDs in yellow-green
        lightLED(0, 4, 75, 150, 0);
    } else if ((pm10 >= 50) && (pm10 < 70)) {
        // 5 LEDs in yellow
        lightLED(0, 5, 150, 120, 0);
    } else if ((pm10 >= 70) && (pm10 < 90)) {
        // 6 LEDs in yellow
        lightLED(0, 6, 150, 120, 0);
    } else if ((pm10 >= 90) && (pm10 < 135)) {
        // 7 LEDs in orange
        lightLED(0, 7, 150, 75, 0);
    } else if ((pm10 >= 135) && (pm10 < 180)) {        
        // 8 LEDs in orange
        lightLED(0, 8, 150, 75, 0);
    } else if ((pm10 >= 180) && (pm10 < 250)) {        
        // 9 LEDs in red
        lightLED(0, 9, 150, 0, 0);
    } else {
        // 10 LEDs in red
        lightLED(0, 10, 150, 0, 0);
    }
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