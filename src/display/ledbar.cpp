//
// Created by irukard on 09.03.2020.
//
#include "ledbar.h"

/*****************************************************************
 * display LEDBAR                                                *
 *****************************************************************/
void displayLEDBar() {
    
    double pm10 = 0;

    if (SDS011::enabled) {
        pm10 = last_value_SDS_P1;
    } else if (cfg::pms_read) {
        pm10 = last_value_PMS_P1;
    }
    byte scale = 4;
    if (pm10 >= 250) lightLED(0, 10, 150/scale, 0, 0);                      // 10 LEDs in red
    else if (pm10 >= 180) lightLED(0, 9, 150/scale, 0, 0);                  // 9 LEDs in red
    else if (pm10 >= 135) lightLED(0, 8, 150/scale, 75/scale, 0);           // 8 LEDs in orange
    else if (pm10 >= 90) lightLED(0, 7, 150/scale, 75/scale, 0);            // 7 LEDs in orange
    else if (pm10 >= 70) lightLED(0, 6, 150/scale, 120/scale, 0);           // 6 LEDs in yellow
    else if (pm10 >= 50) lightLED(0, 5, 150/scale, 120/scale, 0);           // 5 LEDs in yellow
    else if (pm10 >= 37) lightLED(0, 4, 75/scale, 150/scale, 0);            // 4 LEDs in yellow-green
    else if (pm10 >= 25) lightLED(0, 3, 75/scale, 150/scale, 0);            // 3 LEDs in yellow-green
    else if (pm10 >= 12) lightLED(0, 2, 0, 150/scale, 0);                   // 2 LEDs in green
    else if (pm10 >= 0.5) lightLED(0, 1, 0, 150/scale, 0);                  // 1LEDs in green
    else lightLED(0, 0, 0, 2, 0);

}

/*****************************************************************
 * Light LED I2C command                                         *
 *****************************************************************/

void lightLED(byte mode, byte cnt, byte red, byte green, byte blue) {
//    debug_out(F("LEDBAR RGB: "), DEBUG_MIN_INFO,0);
//    debug_out(String(red), DEBUG_MIN_INFO,0);
//    debug_out(F(", "), DEBUG_MIN_INFO,0);
//    debug_out(String(green), DEBUG_MIN_INFO,0);
//    debug_out(F(", "), DEBUG_MIN_INFO,0);
//    debug_out(String(blue), DEBUG_MIN_INFO,1);
//    debug_out(String(cnt), DEBUG_MIN_INFO, 1);
//    debug_out(String(mode), DEBUG_MIN_INFO, 1);

    Wire.beginTransmission(0x32);
    Wire.write(mode);
    Wire.write(cnt);
    Wire.write(red);
    Wire.write(green);
    Wire.write(blue);
//    Wire.write(brightness);
    Wire.endTransmission();
}