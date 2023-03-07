//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_HELPERS_H
#define NAMF_HELPERS_H
#include "Arduino.h"
#include <ArduinoJson.h>
#include "variables.h"
#include <FS.h>                     // must be first

extern const char UNIT_PERCENT[];
extern const char UNIT_CELCIUS[];
extern const unsigned char UNIT_CELCIUS_LCD[];


int32_t calcWiFiSignalQuality(int32_t rssi);

void debug_out(const String& text, const int level, const bool linebreak = true);

//declarations for changing .ino to .cpp
String Float2String(const double value, uint8_t digits);
String Float2String(const double value);
void writeConfig();
String getConfigString(boolean);
String getMaskedConfigString();
int writeConfigRaw(const String &json_string, const char * filename = nullptr);
int readAndParseConfigFile(File);
String add_sensor_type(const String& sensor_text);
String Value2Json(const String& type, const String& value);
String Value2Json(const __FlashStringHelper *, const String& value);
String Var2Json(const String& name, const bool value);
String Var2JsonInt(const String& name, const bool value);
String Var2Json(const String& name, const int value);
String Var2Json(const String& name, const String & value);
String Var2Json(const String& name, const char * value);
String Var2Json(const String& name, const float value);
String Var2Json(const String& name, const unsigned long value);
void addJsonIfNotDefault(String &str,const  __FlashStringHelper *name, const unsigned long defaultValue, const unsigned long current);

void parseHTTP(const __FlashStringHelper *, unsigned long & );
void parseHTTP(const __FlashStringHelper *, bool & );
void parseHTTP(const __FlashStringHelper *, String & );
void parseHTTP(const String &name, bool &value );
void parseHTTP(const __FlashStringHelper * , byte &value );

void setBoolVariableFromHTTP(String const name, bool &v, byte i);
void setVariableFromHTTP(String const name, unsigned long &v, byte i);
void setHTTPVarName(String &varName, String const name, byte id);
void advancedSectionStart( String &html, SimpleScheduler::LoopEntryType sensor);
void advancedSectionEnd( String &html, SimpleScheduler::LoopEntryType sensor);
unsigned long time2Measure(void);
String formSectionHeader(const String& name, byte bold = 0);
void formSectionHeader(String &page_content, const String& name, byte bold = 0);
String form_option(String const &name, const String & info, const bool checked = false);
String form_input(const String& name, const String& info, const String& value, const int length);
String formInputGrid(const String& name, const String& info, const String& value, const int length);
String form_password(const String& name, const String& info, const String& value, const int length);
String formPasswordGrid(const String& name, const String& info, const String& value, const int length);
String form_checkbox(const String& name, const String& info, const bool checked, const bool linebreak = true);
String formCheckboxGrid(const String& name, const String& info, const bool checked);
String formCheckboxOpenGrid(const String& name, const String& info, const bool checked);
String form_checkbox_sensor(const String& name, const String& info, const bool checked);
String form_submit(const String& value);
String formSubmitGrid(const String& value);
String form_select_lang();
void resetMemoryStats();
void collectMemStats();
void dumpCurrentMemStats();
void display_debug(const String& text1, const String& text2);
String millisToTime(const unsigned long);
unsigned stringToChar(char **dst, const String src);
//void dbg(char *v);
String getConfigString(boolean maskPwd = false);


#endif //NAMF_HELPERS_H
