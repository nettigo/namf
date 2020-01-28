//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_HELPERS_H
#define NAMF_HELPERS_H
#include "Arduino.h"

void debug_out(const String& text, const int level, const bool linebreak);

//declarations for changing .ino to .cpp
String Float2String(const double value, uint8_t digits);
String Float2String(const double value);
void writeConfig();
String add_sensor_type(const String& sensor_text);
String Value2Json(const String& type, const String& value);
String Var2Json(const String& name, const bool value);
String Var2Json(const String& name, const int value);
//Nettigo NAM 0.3.2 factory firmware - test

#endif //NAMF_HELPERS_H
