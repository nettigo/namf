//
// Created by viciu on 28.01.2020.
//
#include "variables.h"
#include "helpers.h"
#include "system/scheduler.h"

extern const char UNIT_PERCENT[] PROGMEM = "%";
extern const char UNIT_CELCIUS[] PROGMEM = "°C";
extern const unsigned char UNIT_CELCIUS_LCD[] PROGMEM = {0xDF, 0x43, 0x0};


LoggingSerial Debug;

#if defined(ARDUINO_ARCH_ESP8266)
LoggingSerial::LoggingSerial()
        : HardwareSerial(UART0)
        , m_buffer(new circular_queue<uint8_t>(1024))
{
    skipBuffer = false;
}
#else
LoggingSerial::LoggingSerial()
        : HardwareSerial(0)
        , m_buffer(new circular_queue<uint8_t>(1024))
{
    skipBuffer = false;
}
#endif


void LoggingSerial::stopWebCopy(void) {
    skipBuffer = true;
}

void LoggingSerial::resumeWebCopy(void) {
    skipBuffer = false;
}

size_t LoggingSerial::write(uint8_t c)
{
    if (!skipBuffer) m_buffer->push(c);
    return HardwareSerial::write(c);
}

size_t LoggingSerial::write(const uint8_t *buffer, size_t size)
{
    if (!skipBuffer) m_buffer->push_n(buffer, size);
    return HardwareSerial::write(buffer, size);
}

String LoggingSerial::popLines()
{
    String r;
    while (m_buffer->available() > 0) {
        uint8_t c = m_buffer->pop();
        r += (char) c;

//        if (c == '\n' && r.length() > m_buffer->available())
//            break;
    }
    return r;
}


int32_t calcWiFiSignalQuality(int32_t rssi) {
    if (rssi > -50) {
        rssi = -50;
    }
    if (rssi < -100) {
        rssi = -100;
    }
    return (rssi + 100) * 2;
}

//store string as char array, for functions
unsigned stringToChar(char **dst, const String src) {
//    Serial.println(F("stringToChar"));
//    Serial.println(src.c_str());
    if (*dst != nullptr) {
//        Serial.println(F("Deleting DST"));
        delete (*dst);
    }
//    Serial.println(F("allocating"));
    unsigned int len = src.length() + 1;
    *dst = new char[len];
//    Serial.println(F("after allocating"));

    if (*dst == nullptr) return 0;   //does it return nullptr or raises Abort? Probaby fails in 3.0.0 ArduinoCore
//    Serial.print(F("copying len: "));
//    Serial.println(len);
    strncpy(*dst, src.c_str(), len);
//    Serial.println(F("copied..."));
//    Serial.println(*dst);
    return len;
}

unsigned setDefault(char **dst, const __FlashStringHelper *defaultValue) {
//    Serial.println(F("setDefault"));
    if (dst == nullptr || !strlen(*dst)) {
        String src = String(defaultValue);
//        Serial.println(F("SRC READY"));
        return stringToChar(dst, src);
    }
    return strlen(*dst) + 1;
}

//same as stringToChar but for JSON char string as source
unsigned charStringToChar(char *dst, const char *src) {
    if (dst != nullptr) delete (dst);
    size_t len = strlen(src);
    dst = new(char[len + 1]);
    if (dst == nullptr) return 0;
    strncpy(dst, src, len+1);
    return len + 1;
}

/*****************************************************************
 * convert float to string with a                                *
 * precision of two (or a given number of) decimal places        *
 *****************************************************************/
String Float2String(const double value) {
    return Float2String(value, 2);
}

String Float2String(const double value, uint8_t digits) {
    // Convert a float to String with two decimals.
    char temp[15];

    dtostrf(value, 13, digits, temp);
    String s = temp;
    s.trim();
    return s;
}

/*****************************************************************
 * convert value to json string                                  *
 *****************************************************************/
String Value2Json(const String& type, const String& value) {
    String s = F("{\"value_type\":\"{t}\",\"value\":\"{v}\"},");
    s.replace("{t}", type);
    s.replace("{v}", value);
    return s;
}


String Value2Json( const __FlashStringHelper * t, const String& value) {
    String type(t);
    String s = F("{\"value_type\":\"{t}\",\"value\":\"{v}\"},");
    s.replace("{t}", type);
    s.replace("{v}", value);
    return s;
}

/*****************************************************************
 * convert string value to json string                           *
 *****************************************************************/
String Var2Json(const String& name, const String& value) {
    String s = F("\"{n}\":\"{v}\",");
    String tmp = value;
    tmp.replace("\\", "\\\\"); tmp.replace("\"", "\\\"");
    s.replace("{n}", name);
    s.replace("{v}", tmp);
    return s;
}

/*****************************************************************
 * convert boolean value to json string                          *
 *****************************************************************/
String Var2Json(const String &name, const bool value) {
    String s = F("\"{n}\":\"{v}\",");
    s.replace("{n}", name);
    s.replace("{v}", (value ? "true" : "false"));
    return s;
}

String Var2Json(const String &name, const char *value) {
    return Var2Json(name, String(value));
}

String Var2JsonInt(const String &name, const bool value) {
    String s = F("\"{n}\":{v},");
    s.replace("{n}", name);
    s.replace("{v}", (value ? "1" : "0"));
    return s;
}

/*****************************************************************
 * convert boolean value to json string                          *
 *****************************************************************/
String Var2Json(const String &name, const int value) {
    String s = F("\"{n}\":\"{v}\",");
    s.replace("{n}", name);
    s.replace("{v}", String(value));
    return s;
}

String Var2Json(const String &name, const unsigned long value) {
    String s = F("\"{n}\":\"{v}\",");
    s.replace(F("{n}"), name);
    s.replace(F("{v}"), String(value));
    return s;
}


String Var2Json(const String& name, const float value) {
    String s = F("\"{n}\":\"{v}\",");
    s.replace("{n}", name);
    s.replace("{v}", String(value));
    return s;
}

void addJsonIfNotDefault(String &str,const  __FlashStringHelper *name, const unsigned long defaultValue, const unsigned long current) {
    if (defaultValue != current){
        str += Var2Json(name, current);
    }
    return;
}




/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/
void debug_out(const String& text, const int level, const bool linebreak) {
    static bool timestamp=true;
    if (level <= cfg::debug) {
        if (timestamp) {
            time_t rawtime;
            struct tm *timeinfo;
            char buffer[30];
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(buffer, 30, "%F %T: ", timeinfo);

            Debug.print(buffer);
            timestamp = false;
        }

        if (linebreak) {
            timestamp = true;
            Debug.println(text);
        } else {
            Debug.print(text);
        }
    }
}




//Create string with config as JSON
String getConfigString(boolean maskPwd) {
    using namespace cfg;
    String json_string = "{";

#define copyToJSON_Bool(varname) json_string += Var2Json(#varname,varname);
#define copyToJSON_Int(varname) json_string += Var2Json(#varname,varname);

    json_string += Var2Json(F("current_lang"), current_lang);
    json_string += Var2Json(F("SOFTWARE_VERSION"), SOFTWARE_VERSION);
    json_string += Var2Json(F("wlanssid"), wlanssid);
    //mask WiFi password?
    if (maskPwd) {
        json_string += Var2Json(F("wlanpwd"), String(F("****")));
        json_string += Var2Json(F("fbpwd"), String(F("****")));
        json_string += Var2Json(F("fs_pwd"), String(F("****")));

    } else {
        json_string += Var2Json(F("wlanpwd"), wlanpwd);
        json_string += Var2Json(F("fbpwd"), fbpwd);
        json_string += Var2Json(F("fs_pwd"), fs_pwd);

    }
    json_string += Var2Json(F("www_username"), www_username);
    json_string += Var2Json(F("www_password"), www_password);
    json_string += Var2Json(F("fs_ssid"), fs_ssid);
    json_string += Var2Json(F("fbssid"), fbssid);

#ifdef NAM_LORAWAN

    json_string += Var2Json(F("lw_en"), lw_en);
    json_string += Var2Json(F("lw_d_eui"), lw_d_eui);
    json_string += Var2Json(F("lw_a_eui"), lw_a_eui);
    json_string += Var2Json(F("lw_app_key"), lw_app_key);
//    json_string += Var2Json(F("lw_nws_key"), lw_nws_key);
//    json_string += Var2Json(F("lw_apps_key"), lw_apps_key);
//    json_string += Var2Json(F("lw_dev_addr"), lw_dev_addr);
#endif

    copyToJSON_Bool(www_basicauth_enabled);
    copyToJSON_Bool(dht_read);
//    copyToJSON_Bool(sds_read);
    copyToJSON_Bool(pms_read);
//    copyToJSON_Bool(bmp280_read);
//    copyToJSON_Bool(bme280_read);
//    copyToJSON_Bool(heca_read);
    copyToJSON_Bool(ds18b20_read);
    copyToJSON_Bool(gps_read);
    copyToJSON_Bool(send2dusti);
    copyToJSON_Bool(ssl_dusti);
    copyToJSON_Bool(send2madavi);
    copyToJSON_Bool(ssl_madavi);
    copyToJSON_Bool(send2sensemap);
    copyToJSON_Bool(send2fsapp);
    copyToJSON_Bool(send2lora);
    copyToJSON_Bool(send2csv);
    copyToJSON_Bool(auto_update);
    copyToJSON_Int(update_channel);
    copyToJSON_Bool(has_display);
    copyToJSON_Bool(has_lcd1602);
    copyToJSON_Bool(has_lcd2004);
    json_string.concat(F("\"bl\":["));
    json_string.concat(String(backlight_start));
    json_string.concat(F(","));
    json_string.concat(String(backlight_stop));
    json_string.concat(F("],"));
    copyToJSON_Bool(show_wifi_info);
    copyToJSON_Bool(sh_dev_inf);
    copyToJSON_Bool(has_ledbar_32);
    json_string += Var2Json(F("debug"), debug);
    json_string += Var2Json(F("send_diag"), send_diag);

    json_string += Var2Json(F("sending_intervall_ms"), sending_intervall_ms);
    json_string += Var2Json(F("time_for_wifi_config"), time_for_wifi_config);
    copyToJSON_Int(outputPower);
    copyToJSON_Int(phyMode);
    json_string += Var2Json(F("senseboxid"), senseboxid);
    copyToJSON_Bool(send2custom);
    json_string += Var2Json(F("host_custom"), host_custom);
    json_string += Var2Json(F("url_custom"), url_custom);
    copyToJSON_Int(port_custom);
    json_string += Var2Json(F("user_custom"), user_custom);
    json_string += Var2Json(F("pwd_custom"), pwd_custom);

    copyToJSON_Bool(send2aqi);
    json_string += Var2Json(F("token_AQI"), token_AQI);

    json_string += Var2Json(F("host_custom"), host_custom);
    copyToJSON_Bool(send2influx);
    json_string += Var2Json(F("host_influx"), host_influx);
    json_string += Var2Json(F("url_influx"), url_influx);
    copyToJSON_Bool(ssl_influx);

    json_string.concat(Var2Json(F("UUID"), UUID));

    copyToJSON_Int(port_influx);
    json_string += Var2Json(F("user_influx"), user_influx);
    json_string += Var2Json(F("pwd_influx"), pwd_influx);
#undef copyToJSON_Bool
#undef copyToJSON_Int
#undef Var2Json
    SimpleScheduler::getConfigJSON(json_string);
    json_string += "}";

    return json_string;

}

//Create string with config as JSON
String getMaskedConfigString() {
    return getConfigString(true);
}
//make sure lang is on supported list
void verifyLang(char *cl) {
    if (!strcmp(cl,"PL")) return;
    if (!strcmp(cl,"EN")) return;
    if (!strcmp(cl,"HU")) return;
    if (!strcmp(cl,"RO")) return;
    strcpy(cl,"EN");
}

void dbg(char *v) { if (v == nullptr) Serial.println(F("NULL")); else Serial.println(v);}


void setCharVar(const JsonObject &json, char **var, const __FlashStringHelper *key, const __FlashStringHelper *def = nullptr) {
    if (json.containsKey(key)) stringToChar(var, json[key]);
    if (def != nullptr) setDefault(var, def);
}
void setCharVar(const JsonObject &json, String &var, const __FlashStringHelper *key, const __FlashStringHelper *def = nullptr) {
    if (json.containsKey(key)) var = json.get<String>(key);
    if (def != nullptr) var = def;
}

int readAndParseConfigFile(File configFile) {
    using namespace cfg;
    String json_string = "";
    bool pms24_read = false;
    bool pms32_read = false;
    if (configFile) {
        debug_out(F("opened config file..."), DEBUG_MED_INFO, 1);
        const size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        debug_out(F("Config - JSON object memory used:"),DEBUG_MED_INFO, false);
        debug_out(String(jsonBuffer.size()),DEBUG_MED_INFO);

        json.printTo(json_string);
        debug_out(F("File content: "), DEBUG_MAX_INFO, 0);
        debug_out(String(buf.get()), DEBUG_MAX_INFO, 1);
        debug_out(F("JSON Buffer content: "), DEBUG_MAX_INFO, 0);
        Debug.stopWebCopy();
        debug_out(json_string, DEBUG_MAX_INFO, 1);
        Debug.resumeWebCopy();
        if (json.success()) {
            debug_out(F("JSON parsed"), DEBUG_MED_INFO, 1);
            setCharVar(json, &wlanssid, F("wlanssid"), FPSTR(EMPTY_STRING));
            setCharVar(json, &wlanpwd, F("wlanpwd"), FPSTR(EMPTY_STRING));
            setCharVar(json, &fbssid, F("fbssid"), FPSTR(EMPTY_STRING));
            setCharVar(json, &fbpwd, F("fbpwd"), FPSTR(EMPTY_STRING));
            setCharVar(json, &www_username, F("www_username"), FPSTR(WWW_USERNAME));
            setCharVar(json, &www_password, F("www_password"), FPSTR(WWW_PASSWORD));
            setCharVar(json, &fs_ssid, F("fs_ssid"), FPSTR(FS_SSID));
            setCharVar(json, &fs_pwd, F("fs_pwd"), FPSTR(FS_PWD));
#define setFromJSON(key)    if (json.containsKey(#key)) key = json[#key];
#define strcpyFromJSON(key) if (json.containsKey(#key)) strcpy(key, json[#key]);

#ifdef NAM_LORAWAN
            setFromJSON(lw_en);
            setCharVar(json, lw_d_eui, F("lw_d_eui"));
            setCharVar(json, lw_a_eui, F("lw_a_eui"));
            setCharVar(json, lw_app_key, F("lw_app_key"));
//            setCharVar(json, lw_nws_key, F("lw_nws_key"));
//            setCharVar(json, lw_apps_key, F("lw_apps_key"));
//            setCharVar(json, lw_dev_addr, F("lw_dev_addr"));
#endif
            setCharVar(json, &user_custom, F("user_custom"), FPSTR(USER_CUSTOM));
            setCharVar(json, &pwd_custom, F("pwd_custom"), FPSTR(PWD_CUSTOM));
            setCharVar(json, &user_influx, F("user_influx"), FPSTR(USER_INFLUX));
            setCharVar(json, &pwd_influx, F("pwd_influx"), FPSTR(PWD_INFLUX));

            strcpyFromJSON(current_lang);
            verifyLang(current_lang);


            setFromJSON(www_basicauth_enabled);
            setFromJSON(dht_read);
            setFromJSON(sds_read);
            setFromJSON(pms_read);
            setFromJSON(pms24_read);
            setFromJSON(pms32_read);
            setFromJSON(bmp280_read);
            setFromJSON(bme280_read);
            setFromJSON(heca_read);
            setFromJSON(ds18b20_read);
            setFromJSON(gps_read);
            setFromJSON(send2dusti);
            setFromJSON(ssl_dusti);
            setFromJSON(send2madavi);
            setFromJSON(ssl_madavi);
            setFromJSON(send2sensemap);
            setFromJSON(send2fsapp);
            setFromJSON(send2lora);
            setFromJSON(send2csv);
            setFromJSON(auto_update);
            setFromJSON(update_channel);
            setFromJSON(has_display);
            setFromJSON(has_lcd1602);
            setFromJSON(has_lcd2004);
            if(json.containsKey(F("bl"))) {
                backlight_start = json[F("bl")][0];
                backlight_stop = json[F("bl")][1];
            }
            //need to migrate old config values to new ones - can not use JSON helpers
            if (json.containsKey(F("has_lcd1602_27"))) { has_lcd1602 = json[F("has_lcd1602_27")];}
            if (json.containsKey(F("has_lcd2004_27"))) {
                has_lcd2004 = json[F("has_lcd2004_27")];
                debug_out("2004 z 27 ustawiony na ", DEBUG_MED_INFO,0);
                debug_out(String(has_lcd2004), DEBUG_MED_INFO);

            }
            if (json.containsKey(F("has_lcd2004_3f"))) {
                has_lcd2004 = json[F("has_lcd2004_3f")];
                debug_out("2004 z 3F ustawiony na ", DEBUG_MED_INFO,0);
                debug_out(String(has_lcd2004), DEBUG_MED_INFO);

            }
            setFromJSON(show_wifi_info);
            setFromJSON(sh_dev_inf);
            setFromJSON(has_ledbar_32);
            setFromJSON(debug);
            setFromJSON(send_diag);
            setFromJSON(sending_intervall_ms);
            setFromJSON(time_for_wifi_config);
            setFromJSON(outputPower);
            setFromJSON(phyMode);
            strcpyFromJSON(senseboxid);
            if (strcmp(senseboxid, "00112233445566778899aabb") == 0) {
                strcpy(senseboxid, "");
                send2sensemap = 0;
            }
            setFromJSON(send2custom);

            if (json.containsKey(F("host_custom"))) host_custom =  json.get<String>(F("host_custom"));
            if (json.containsKey(F("url_custom"))) url_custom =  json.get<String>(F("url_custom"));
            setFromJSON(port_custom);

            setFromJSON(send2aqi);

            if (json.containsKey(F("token_AQI"))) token_AQI =  json.get<String>(F("token_AQI"));

            setFromJSON(send2influx);
            setFromJSON(ssl_influx);

            if (json.containsKey(F("host_influx"))) host_influx =  json.get<String>(F("host_influx"));
            if (json.containsKey(F("url_influx"))) url_influx =  json.get<String>(F("url_influx"));
            setFromJSON(port_influx);

            if (host_influx.equals(F("api.luftdaten.info"))) {
                host_influx = F("");
                send2influx = 0;
            }

            if (json.containsKey(F("UUID"))) UUID = json.get<String>(F("UUID"));

            configFile.close();
            if (pms24_read || pms32_read) {
                pms_read = 1;
                writeConfig();
            };
#undef setFromJSON
#undef strcpyFromJSON
            //Sensor configs from simple scheduler
            if (!json.containsKey(F("sensors")))
                json.createNestedObject(F("sensors"));
            if (json.containsKey(F("sensors"))) {
                JsonObject& item = json[F("sensors")];
                if (cfg::sds_read) {
                    if (!item.containsKey(F("SDS011"))) {
                        item.createNestedObject(F("SDS011"));
                    }
                    item[F("SDS011")][F("e")] = 1;
                    item[F("SDS011")][F("d")] = 1;
                }
                if (cfg::bme280_read) {
                    if (!item.containsKey(F("BME280"))) {
                        item.createNestedObject(F("BME280"));
                    }
                    item[F("BME280")][F("e")] = 1;
//                    item[F("BME280")][F("d")] = 1;
                }
                if (cfg::heca_read) {
                    if (!item.containsKey(F("HECA"))) {
                        item.createNestedObject(F("HECA"));
                    }
                    item[F("HECA")][F("e")] = 1;
                    item[F("HECA")][F("d")] = 1;
                }
                SimpleScheduler::readConfigJSON(item);
            }
            return 0;
        } else {
            debug_out(F("failed to load json config"), DEBUG_ERROR, 1);
            return -1;
        }
    }
    return -1;
}

/*****************************************************************
 * write config to spiffs                                        *
 *****************************************************************/
int writeConfigRaw(const String &json_string, const char * filename) {
    Debug.stopWebCopy();
    debug_out(json_string, DEBUG_MAX_INFO, 1);
    Debug.resumeWebCopy();
    File configFile;
    if (filename) {
        configFile = SPIFFS.open(filename, "w");
    } else {
        configFile = SPIFFS.open("/config.json", "w");
    }
    if (configFile) {
        configFile.print(json_string);
        debug_out(F("Config written OK."), DEBUG_MIN_INFO, 0);
        Debug.stopWebCopy();
        debug_out(json_string, DEBUG_MIN_INFO, 1);
        Debug.resumeWebCopy();
        configFile.close();
        return 0;
    } else {
        debug_out(F("failed to open config file for writing"), DEBUG_ERROR, 1);
        return -1;
    }
}

/*
 * get current config and save it
 */

void writeConfig(){
    debug_out(F("saving config..."), DEBUG_MIN_INFO);
    String json_string  = getConfigString();
    writeConfigRaw(json_string);
}

void parseHTTP(const __FlashStringHelper *name, unsigned long &value ){
    if (server.hasArg(name)) {
        value = server.arg(name).toInt();
    }
};

void parseHTTP(const String name, unsigned long &value ){
    if (server.hasArg(name)) {
        value = server.arg(name).toInt();
    }
};

void  parseHTTP(const __FlashStringHelper *name, byte &value ){
    if (server.hasArg(name)) {
        value = server.arg(name).toInt();
    }
};
void parseHTTP(const __FlashStringHelper *name, String &value ){
    value = F("");
    if (server.hasArg(name)) {
        value = server.arg(name);
    }
};

void parseHTTP(const __FlashStringHelper *name, bool &value ){
    value = false;
    if (server.hasArg(name)) {
        value = server.arg(name) == "1";
    }
};

void parseHTTP(const String &name, bool &value ){
    value = false;
    if (server.hasArg(name)) {
        value = server.arg(name) == "1";
    }
};

void setHTTPVarName(String &varName, String const name, byte id) {
    varName = F("{n}-{s}");
    varName.replace(F("{s}"), String(id));
    varName.replace(F("{n}"), name);

}


void advancedSectionStart( String &html, SimpleScheduler::LoopEntryType sensor){

    html.concat(F("</div><div class='gc'><div class='row'><input type='button' value='"));
    html.concat(FPSTR(INTL_ADVANCED_BUTTON));
    html.concat("' data-code=");
    html.concat(String(sensor));
    html.concat(F(" class='asb'/></div></div><div class='advSect' id='as-"));
    html.concat(String(sensor));
    html.concat(F("'><div class='gc'><div class='row'><p>"));
    html.concat(FPSTR(INTL_ADVANCED_DISCL));
    html.concat(F("</p></div>\n"));
}

void advancedSectionEnd( String &html, SimpleScheduler::LoopEntryType sensor){
     html.concat(F("</div>"));
}
/* get read and set bool variables for new scheduler. Works with checkbox
 named var_name-SENSOR_ID (var name it is enable or display currently
 Getting enable val from client is code done by each sensor, to make it easier this
 helper was created. It supports any bool var.
*/
void setBoolVariableFromHTTP(String const name, bool &v, byte i){
    String sensorID;
    setHTTPVarName(sensorID, name, i);
    parseHTTP(sensorID, v);
}

void setVariableFromHTTP(String const name, unsigned long &v, byte i){
    String sensorID;
    setHTTPVarName(sensorID, name, i);
    parseHTTP(sensorID, v);
}
void setVariableFromHTTP(const __FlashStringHelper *name, unsigned long &v, byte i){
    String sensorID;
    setHTTPVarName(sensorID, name, i);
    parseHTTP(sensorID, v);
}

unsigned long time2Measure(void){
    unsigned long timeFromStart = millis() - starttime;
    if ( timeFromStart > cfg::sending_intervall_ms) return 0;
    return cfg::sending_intervall_ms - timeFromStart;
};

//Form helpers
//

// bold
// 0 - use <b>
// 1 - use <h1>
// 2 - use <h2>
// 3 - do not use any tags
void formSectionHeader(String &page_content, const String& name, byte bold) {
    page_content.concat(formSectionHeader(name, bold));
}

String formSectionHeader(const String& name, byte bold) {

    String s;
    switch (bold){
        case(1):
            s = F("<div class='row sect'><h1>{n}</h1></div>\n");
            break;
        case(2):
            s = F("<div class='row sect'><h2>{n}</h2></div>\n");
            break;
        case(3):
            s = F("<div class='row sect'>{n}</div>\n");
            break;
        default:
            s = F("<div class='row sect'><b>{n}</b></div>\n");
    }
    s.replace("{n}", name);
    return s;
}

String formInputGrid(const String& name, const String& info, const String& value, const int length) {
    String s = F(	"<div>{i}</div><div class='c2'>"
                     "<input type='text' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
                     "</div>");
    String t_value = value;
    t_value.replace("'", "&#39;");
    s.replace("{i}", info);
    s.replace("{n}", name);
    s.replace("{v}", t_value);
    s.replace("{l}", String(length));
    return s;
}
String form_input(const String& name, const String& info, const String& value, const int length) {
    String s = F(	"<tr>"
                     "<td>{i} </td>"
                     "<td style='width:90%;'>"
                     "<input type='text' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
                     "</td>"
                     "</tr>");
    String t_value = value;
    t_value.replace("'", "&#39;");
    s.replace("{i}", info);
    s.replace("{n}", name);
    s.replace("{v}", t_value);
    s.replace("{l}", String(length));
    return s;
}
const char form_password_templ[] PROGMEM = "<tr>"
                                           "<td>{i} </td>"
                                           "<td style='width:90%;'>"
                                           "<input type='password' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
                                           "</td>"
                                           "</tr>";
String form_password(const String& name, const String& info, const String& value, const int length) {
    unsigned size = strlen_P(form_password_templ) + 1 ;
    size += name.length() + info.length() + value.length() - 9;

    String s;
    s.reserve(size);
    s = FPSTR(form_password_templ);
    String password = "";
    password.reserve(value.length());
    for (uint8_t i = 0; i < value.length(); i++) {
        password.concat(F("*"));
    }
    s.replace("{i}", info);
    s.replace("{n}", name);
    s.replace("{v}", password);
    s.replace("{l}", String(length));
    return s;
}
const char formPasswordGrid_templ[] PROGMEM = "<div>{i}</div><div class='c2'><input type='password' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/></div>\n";

String formPasswordGrid(const String& name, const String& info, const String& value, const int length) {
    unsigned size = strlen_P(formPasswordGrid_templ) + 1 ;
    size += name.length() + info.length() + value.length() - 9;

    String s;
    s.reserve(size);
    s = FPSTR(formPasswordGrid_templ);
    String password = "";
    password.reserve(value.length());
    for (uint8_t i = 0; i < value.length(); i++) {
        password.concat(F("*"));
    }
    s.replace("{i}", info);
    s.replace("{n}", name);
    s.replace("{v}", password);
    s.replace("{l}", String(length));
    return s;
}

const char formCheckboxGrid_templ[] PROGMEM = "<div class='row'><input type='checkbox' name='{n}' value='1' id='{n}' {c}/><label for='{n}'>{i}</label></div>";
String formCheckboxGrid(const String& name, const String& info, const bool checked) {
    String newInfo = add_sensor_type(info);
    unsigned size = strlen_P(formCheckboxGrid_templ) + 1 ;
    size += name.length() + newInfo.length();
    size += checked ? 19 : 0;

    String s;
    s.reserve(size);
    s = FPSTR(formCheckboxGrid_templ);
    if (checked) {
        s.replace("{c}", F(" checked='checked'"));
    } else {
        s.replace("{c}", "");
    };
    s.replace("{i}", newInfo);
    s.replace("{n}", name);
    return s;
}

const char formCheckboxOpenGrid_templ[] PROGMEM = "<div><input type='checkbox' name='{n}' value='1' id='{n}' {c}/><label for='{n}'>{i}</label></div>";
//Use only columns 1 & 2 from grid
String formCheckboxOpenGrid(const String& name, const String& info, const bool checked) {
    String newInfo = add_sensor_type(info);

    unsigned size = strlen_P(formCheckboxOpenGrid_templ) + 1 ;
    size += name.length() + newInfo.length();
    size += checked ? 19 : 0;

    String s;
    s.reserve(size);
    s = FPSTR(formCheckboxOpenGrid_templ);
    if (checked) {
        s.replace("{c}", F(" checked='checked'"));
    } else {
        s.replace("{c}", "");
    };
    s.replace("{i}", newInfo);
    s.replace("{n}", name);
    return s;
}

const char form_checkbox_templ[] PROGMEM = "<label for='{n}'><input type='checkbox' name='{n}' value='1' id='{n}' {c}/> {i}</label><br/>";
String form_checkbox(const String& name, const String& info, const bool checked, const bool linebreak) {

    unsigned size = strlen_P(form_checkbox_templ) + 1 ;
    size += name.length() + info.length();
    size += checked ? 19 : 0;
    size += linebreak ? 6 : 0;

    String s;
    s.reserve(size);
    s = FPSTR(form_checkbox_templ);
    if (checked) {
        s.replace("{c}", F(" checked='checked'"));
    } else {
        s.replace("{c}", "");
    };
    s.replace("{i}", info);
    s.replace("{n}", name);
    if (! linebreak) {
        s.replace("<br/>", "");
    }
    return s;
}

String form_option(String const &name, const String & info, const bool checked){
    String ret;
    ret = F("<option value=");
    ret += name;
    if (checked) {
        ret += F(" selected=\"selected\"");
    }
    ret += F(">");
    ret += info;
    ret += "</option>";
    return ret;
}

String form_checkbox_sensor(const String& name, const String& info, const bool checked) {
    return form_checkbox(name, add_sensor_type(info), checked);
}

String form_submit(const String& value) {
    String s = F(	"<tr>"
                     "<td>&nbsp;</td>"
                     "<td>"
                     "<input type='submit' name='submit' value='{v}' class='s_r'/>"
                     "</td>"
                     "</tr>");
    s.replace("{v}", value);
    return s;
}
String formSubmitGrid(const String& value) {
    String s = F(	"<div><input type='submit' name='submit' value='{v}' class='s_r'/></div>");
    s.replace("{v}", value);
    return s;
}

String form_select_lang() {
    String s_select = F(" selected='selected'");
    String s = F(	"<div>{t}</div><div class='c2'>"
                     "<select name='current_lang'>"
                     "<option value='EN'{s_EN}>English (EN)</option>"
                     "<option value='HU'{s_HU}>Hungarian (HU)</option>"
                     "<option value='PL'{s_PL}>Polski (PL)</option>"
                     "<option value='RO'{s_RO}>ROMÂNĂ (RO)</option>"
                     "</select>"
                     "</div>");
    s.reserve(s.length()+40);

    s.replace("{t}", FPSTR(INTL_LANGUAGE));

    s.replace("{s_" + String(cfg::current_lang) + "}", s_select);
    while (s.indexOf("{s_") != -1) {
        s.remove(s.indexOf("{s_"), 6);
    }
    return s;
}
void resetMemoryStats() {
    memoryStatsMax = memory_stat_t{0,0,0, 0};
    memoryStatsMin = memory_stat_t{UINT32_MAX, UINT16_MAX, UINT8_MAX, UINT32_MAX};
}
void collectMemStats() {
    memory_stat_t memoryStats;
#if defined(ARDUINO_ARCH_ESP8266)
    ESP.getHeapStats(&memoryStats.freeHeap, &memoryStats.maxFreeBlock, &memoryStats.frag);
#else
    memoryStats.freeHeap =  ESP.getFreeHeap();
    memoryStats.maxFreeBlock = 0;
    memoryStats.frag = 0;
#endif


    if (memoryStats.freeHeap > memoryStatsMax.freeHeap)             memoryStatsMax.freeHeap  = memoryStats.freeHeap;
    if (memoryStats.maxFreeBlock > memoryStatsMax.maxFreeBlock)     memoryStatsMax.maxFreeBlock  = memoryStats.maxFreeBlock;
    if (memoryStats.frag > memoryStatsMax.frag)                     memoryStatsMax.frag  = memoryStats.frag;

    if (memoryStats.freeHeap < memoryStatsMin.freeHeap)             memoryStatsMin.freeHeap  = memoryStats.freeHeap;
    if (memoryStats.maxFreeBlock < memoryStatsMin.maxFreeBlock)     memoryStatsMin.maxFreeBlock  = memoryStats.maxFreeBlock;
    if (memoryStats.frag < memoryStatsMin.frag)                     memoryStatsMin.frag  = memoryStats.frag;

#if defined(ARDUINO_ARCH_ESP8266)
    uint32_t cont = ESP.getFreeContStack();
#else
    uint32_t cont = 0;
#endif

    if (cont > memoryStatsMax.freeContStack)                     memoryStatsMax.freeContStack  = cont;
    if (cont < memoryStatsMin.freeContStack)                     memoryStatsMin.freeContStack  = cont;


}
void dumpCurrentMemStats() {
#if defined(ARDUINO_ARCH_ESP8266)

    memory_stat_t memoryStats;
    ESP.getHeapStats(&memoryStats.freeHeap, &memoryStats.maxFreeBlock, &memoryStats.frag);
    debug_out(F("Memory stats: "),DEBUG_MIN_INFO,true);
    debug_out(F("Free heap: \t"),DEBUG_MIN_INFO,false);
    debug_out(String(memoryStats.freeHeap),DEBUG_MIN_INFO,true);
    debug_out(F("Mx free blk: \t"),DEBUG_MIN_INFO,false);
    debug_out(String(memoryStats.maxFreeBlock),DEBUG_MIN_INFO,true);
    debug_out(F("Frag: \t"),DEBUG_MIN_INFO,false);
    debug_out(String(memoryStats.frag),DEBUG_MIN_INFO,true);
    debug_out(F("Free cont stack: \t"),DEBUG_MIN_INFO,false);
    debug_out(String(ESP.getFreeContStack()),DEBUG_MIN_INFO,true);
#endif
}

/*****************************************************************
 * display values                                                *
 *****************************************************************/
void display_debug(const String& text1, const String& text2) {
    debug_out(F("output debug text to displays..."), DEBUG_MIN_INFO, 1);
    debug_out(text1 + "\n" + text2, DEBUG_MAX_INFO, 1);
    if (display) {
        display->clear();
        display->displayOn();
        display->setTextAlignment(TEXT_ALIGN_LEFT);
        display->drawString(0, 12, text1);
        display->drawString(0, 24, text2);
        display->display();
    }
    if (char_lcd) {
        char_lcd -> clear();
        char_lcd->setCursor(0, 0);
        char_lcd->print(text1);
        char_lcd->setCursor(0, 1);
        char_lcd->print(text2);
    }
}

/* Convert millis value to string like (2 d 3h 20m 30s)
 */
String millisToTime(const unsigned long time) {
    String tmp = "";
    unsigned dt = 0;
    unsigned long t = time;
    if ((dt = t / (24 * 60 * 60 * 1000)) > 0) {
        t = t - dt * 24 * 60 * 60 * 1000;
        tmp += String(dt) + String("d ");
    }
    //they are some hours left or hours = 0 but already have some time value in string
    if ((dt = t / (60 * 60 * 1000)) > 0 || tmp.length() > 0) {
        t = t - dt * 60 * 60 * 1000;
        tmp += String(dt) + String("h ");
    }
    if ((dt = t / (60 * 1000)) > 0 || tmp.length() > 0) {
        t = t - dt * 60 * 1000;
        tmp += String(dt) + String("m ");
    }
    tmp += String(t / 1000) + String("s");
    return tmp;

}
