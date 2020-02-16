//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_HELPERS_H
#define NAMF_HELPERS_H
#include "Arduino.h"
#include <FS.h>                     // must be first

void debug_out(const String& text, const int level, const bool linebreak);

//declarations for changing .ino to .cpp
String Float2String(const double value, uint8_t digits);
String Float2String(const double value);
void writeConfig();
String getConfigString(boolean);
String getMaskedConfigString();
void writeConfigRaw(String json_string);
String add_sensor_type(const String& sensor_text);
String Value2Json(const String& type, const String& value);
String Var2Json(const String& name, const bool value);
String Var2Json(const String& name, const int value);
//Nettigo NAM 0.3.2 factory firmware - test

String form_option(String const &name, const String & info, const bool checked = false);
String form_input(const String& name, const String& info, const String& value, const int length);
String form_password(const String& name, const String& info, const String& value, const int length);
String form_checkbox(const String& name, const String& info, const bool checked, const bool linebreak = true);
String form_checkbox_sensor(const String& name, const String& info, const bool checked);
String form_submit(const String& value);
String form_select_lang();
void resetMemoryStats();
void collectMemStats();
#endif //NAMF_HELPERS_H
