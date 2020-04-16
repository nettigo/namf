//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_HELPERS_H
#define NAMF_HELPERS_H
#include "Arduino.h"
#include <FS.h>                     // must be first

int32_t calcWiFiSignalQuality(int32_t rssi);

void debug_out(const String& text, const int level, const bool linebreak);

//declarations for changing .ino to .cpp
String Float2String(const double value, uint8_t digits);
String Float2String(const double value);
void writeConfig();
String getConfigString(boolean);
String getMaskedConfigString();
int writeConfigRaw(const String &json_string, const char * filename = NULL);
int readAndParseConfigFile(File);
String add_sensor_type(const String& sensor_text);
String Value2Json(const String& type, const String& value);
String Value2Json(const __FlashStringHelper *, const String& value);
String Var2Json(const String& name, const bool value);
String Var2Json(const String& name, const int value);
String Var2Json(const String& name, const float value);
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
void display_debug(const String& text1, const String& text2);
String millisToTime(const unsigned long);
void debugData(const String&, const String&);
void debugData(const String&, const char * ="");
void debugData(const String&, const __FlashStringHelper *);
#endif //NAMF_HELPERS_H
