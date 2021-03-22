//
// Created by viciu on 28.01.2020.
//
#include "variables.h"
#include "helpers.h"
#include "system/scheduler.h"

int32_t calcWiFiSignalQuality(int32_t rssi) {
    if (rssi > -50) {
        rssi = -50;
    }
    if (rssi < -100) {
        rssi = -100;
    }
    return (rssi + 100) * 2;
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




/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/
void debug_out(const String& text, const int level, const bool linebreak) {
    static bool timestamp=true;
    if (level <= cfg::debug) {
        if (timestamp) {
            time_t now = time(nullptr);
            String tmp=(ctime(&now));
            tmp.trim();
            tmp+=": ";
            Serial.print(tmp);
            timestamp=false;
        }

        if (linebreak) {
            timestamp = true;
            Serial.println(text);
        } else {
            Serial.print(text);
        }
    }
}




//Create string with config as JSON
String getConfigString(boolean maskPwd = false) {
    using namespace cfg;
    String json_string = "{";
    debug_out(F("saving config..."), DEBUG_MIN_INFO, 1);

#define copyToJSON_Bool(varname) json_string += Var2Json(#varname,varname);
#define copyToJSON_Int(varname) json_string += Var2Json(#varname,varname);

    json_string += Var2Json(F("current_lang"), current_lang);
    json_string += Var2Json(F("SOFTWARE_VERSION"), SOFTWARE_VERSION);
    json_string += Var2Json(F("wlanssid"), wlanssid);
    //mask WiFi password?
    if (maskPwd) {
        json_string += Var2Json(F("wlanpwd"), String("***"));
    } else {
        json_string += Var2Json(F("wlanpwd"), wlanpwd);
    }
    json_string += Var2Json(F("www_username"), www_username);
    json_string += Var2Json(F("www_password"), www_password);
    json_string += Var2Json(F("fs_ssid"), fs_ssid);
    json_string += Var2Json(F("fs_pwd"), fs_pwd);
    copyToJSON_Bool(www_basicauth_enabled);
    copyToJSON_Bool(dht_read);
    copyToJSON_Bool(sds_read);
    copyToJSON_Bool(pms_read);
    copyToJSON_Bool(bmp280_read);
    copyToJSON_Bool(bme280_read);
    copyToJSON_Bool(heca_read);
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
    copyToJSON_Bool(has_lcd1602_27);
    copyToJSON_Bool(has_lcd2004_27);
    copyToJSON_Bool(has_lcd2004_3f);
    copyToJSON_Bool(show_wifi_info);
    copyToJSON_Bool(has_ledbar_32);
    json_string += Var2Json(F("debug"), debug);
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

    copyToJSON_Bool(send2influx);
    json_string += Var2Json(F("host_influx"), host_influx);
    json_string += Var2Json(F("url_influx"), url_influx);
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

int readAndParseConfigFile(File configFile) {
    using namespace cfg;
    String json_string = "";
    bool pms24_read = false;
    bool pms32_read = false;
    if (configFile) {
        debug_out(F("opened config file..."), DEBUG_MIN_INFO, 1);
        const size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        debug_out(F("Config - JSON object memory used:"),DEBUG_MIN_INFO, false);
        debug_out(String(jsonBuffer.size()),DEBUG_MIN_INFO, true);

        json.printTo(json_string);
        debug_out(F("File content: "), DEBUG_MED_INFO, 0);
        debug_out(String(buf.get()), DEBUG_MED_INFO, 1);
        debug_out(F("JSON Buffer content: "), DEBUG_MED_INFO, 0);
        debug_out(json_string, DEBUG_MED_INFO, 1);
        if (json.success()) {
            debug_out(F("parsed json..."), DEBUG_MIN_INFO, 1);
            if (json.containsKey("SOFTWARE_VERSION")) {
                strcpy(version_from_local_config, json["SOFTWARE_VERSION"]);
            }

#define setFromJSON(key)    if (json.containsKey(#key)) key = json[#key];
#define strcpyFromJSON(key) if (json.containsKey(#key)) strcpy(key, json[#key]);
            strcpyFromJSON(current_lang);
            strcpyFromJSON(wlanssid);
            strcpyFromJSON(wlanpwd);
            strcpyFromJSON(www_username);
            strcpyFromJSON(www_password);
            strcpyFromJSON(fs_ssid);
            strcpyFromJSON(fs_pwd);
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
            setFromJSON(has_lcd1602_27);
            setFromJSON(has_lcd2004_27);
            setFromJSON(has_lcd2004_3f);
            setFromJSON(show_wifi_info);
            setFromJSON(has_ledbar_32);
            setFromJSON(debug);
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
//            strcpyFromJSON(host_custom);
            if (json.containsKey(F("host_custom"))) host_custom =  json.get<String>(F("host_custom"));
            if (json.containsKey(F("url_custom"))) url_custom =  json.get<String>(F("url_custom"));
            setFromJSON(port_custom);
            strcpyFromJSON(user_custom);
            strcpyFromJSON(pwd_custom);
            setFromJSON(send2influx);
            if (json.containsKey(F("host_influx"))) host_influx =  json.get<String>(F("host_influx"));
            if (json.containsKey(F("url_influx"))) url_influx =  json.get<String>(F("url_influx"));
            setFromJSON(port_influx);
            strcpyFromJSON(user_influx);
            strcpyFromJSON(pwd_influx);
            if (host_influx.equals(F("api.luftdaten.info"))) {
                host_influx = F("");
                send2influx = 0;
            }
            configFile.close();
            if (pms24_read || pms32_read) {
                pms_read = 1;
                writeConfig();
            }
#undef setFromJSON
#undef strcpyFromJSON
            //Sensor configs from simple scheduler
            if (json.containsKey(F("sensors"))) {
                JsonObject& item = json[F("sensors")];
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
    debug_out(json_string, DEBUG_MIN_INFO, 1);
    File configFile;
    if (filename) {
        configFile = SPIFFS.open(filename, "w");
    } else {
        configFile = SPIFFS.open("/config.json", "w");
    }
    if (configFile) {
        configFile.print(json_string);
        debug_out(F("Config written: "), DEBUG_MIN_INFO, 0);
        debug_out(json_string, DEBUG_MIN_INFO, 1);
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
    String json_string  = getConfigString();
    writeConfigRaw(json_string);
}

unsigned long  parseHTTP(const __FlashStringHelper *name, unsigned long &value ){
    if (server.hasArg(name)) {
        value = server.arg(name).toInt();
    }
};
void  parseHTTP(const __FlashStringHelper *name, byte &value ){
    if (server.hasArg(name)) {
        value = server.arg(name).toInt();
    }
};
unsigned long  parseHTTP(const __FlashStringHelper *name, String &value ){
    value = F("");
    if (server.hasArg(name)) {
        value = server.arg(name);
    }
};

unsigned long  parseHTTP(const __FlashStringHelper *name, bool &value ){
    value = false;
    if (server.hasArg(name)) {
        value = server.arg(name) == "1";
    }
};

unsigned long  parseHTTP(const String &name, bool &value ){
    value = false;
    if (server.hasArg(name)) {
        value = server.arg(name) == "1";
    }
};

/* get read and set bool variables for new scheduler. Works with checkbox
 named var_name-SENSOR_ID (var name it is enable or display currently
 Geting enable val from client is code done by each sensor, to make it easier this
 helper was created. It supports any bool var.
*/
void setBoolVariableFromHTTP(String const name, bool &v, byte i){
    String sensorID = F("{n}-{s}");
    sensorID.replace(F("{s}"), String(i));
    sensorID.replace(F("{n}"), name);
    parseHTTP(sensorID, v);


}

//Form helpers
//
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

String form_password(const String& name, const String& info, const String& value, const int length) {
    String s = F(	"<tr>"
                     "<td>{i} </td>"
                     "<td style='width:90%;'>"
                     "<input type='password' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
                     "</td>"
                     "</tr>");
    String password = "";
    for (uint8_t i = 0; i < value.length(); i++) {
        password += "*";
    }
    s.replace("{i}", info);
    s.replace("{n}", name);
    s.replace("{v}", password);
    s.replace("{l}", String(length));
    return s;
}

String form_checkbox(const String& name, const String& info, const bool checked, const bool linebreak) {
    String s = F("<label for='{n}'><input type='checkbox' name='{n}' value='1' id='{n}' {c}/> {i}</label><br/>");
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
                     "<input type='submit' name='submit' value='{v}' class='s_red'/>"
                     "</td>"
                     "</tr>");
    s.replace("{v}", value);
    return s;
}

String form_select_lang() {
    String s_select = F(" selected='selected'");
    String s = F(	"<tr>"
                     "<td>{t}</td>"
                     "<td>"
                     "<select name='current_lang'>"
                     //					"<option value='DE'{s_DE}>Deutsch (DE)</option>"
                     //					"<option value='BG'{s_BG}>Bulgarian (BG)</option>"
                     //					"<option value='CZ'{s_CZ}>Český (CZ)</option>"
                     "<option value='EN'{s_EN}>English (EN)</option>"
                     "<option value='HU'{s_HU}>Hungarian (HU)</option>"
                     //					"<option value='ES'{s_ES}>Español (ES)</option>"
                     //					"<option value='FR'{s_FR}>Français (FR)</option>"
                     //					"<option value='IT'{s_IT}>Italiano (IT)</option>"
                     //					"<option value='LU'{s_LU}>Lëtzebuergesch (LU)</option>"
                     //					"<option value='NL'{s_NL}>Nederlands (NL)</option>"
                     "<option value='PL'{s_PL}>Polski (PL)</option>"
                     //					"<option value='PT'{s_PT}>Português (PT)</option>"
                     //					"<option value='RU'{s_RU}>Русский (RU)</option>"
                     //					"<option value='SE'{s_SE}>Svenska (SE)</option>"
                     "</select>"
                     "</td>"
                     "</tr>");

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
    ESP.getHeapStats(&memoryStats.freeHeap, &memoryStats.maxFreeBlock, &memoryStats.frag);

    if (memoryStats.freeHeap > memoryStatsMax.freeHeap)             memoryStatsMax.freeHeap  = memoryStats.freeHeap;
    if (memoryStats.maxFreeBlock > memoryStatsMax.maxFreeBlock)     memoryStatsMax.maxFreeBlock  = memoryStats.maxFreeBlock;
    if (memoryStats.frag > memoryStatsMax.frag)                     memoryStatsMax.frag  = memoryStats.frag;

    if (memoryStats.freeHeap < memoryStatsMin.freeHeap)             memoryStatsMin.freeHeap  = memoryStats.freeHeap;
    if (memoryStats.maxFreeBlock < memoryStatsMin.maxFreeBlock)     memoryStatsMin.maxFreeBlock  = memoryStats.maxFreeBlock;
    if (memoryStats.frag < memoryStatsMin.frag)                     memoryStatsMin.frag  = memoryStats.frag;

    uint32_t cont = ESP.getFreeContStack();
    if (cont > memoryStatsMax.freeContStack)                     memoryStatsMax.freeContStack  = cont;
    if (cont < memoryStatsMin.freeContStack)                     memoryStatsMin.freeContStack  = cont;


}
void dumpCurrentMemStats() {
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
void debugData(const String &d, const __FlashStringHelper  *msg) {
    PGM_P p = reinterpret_cast<PGM_P>(msg);
    debugData(d,p);
}
void debugData(const String&d, const String&m){
    debugData(d,m.c_str());
}
void debugData(const String &d, const char *msg){
    Serial.print(F("\n****["));
    time_t now = time(nullptr);
    String tmp=(ctime(&now));
    tmp.trim();
    Serial.print(tmp);
    Serial.print(F("]**** "));
    Serial.print(msg);
    if (d.endsWith("\n")) {
        Serial.print(d);
    } else {
        Serial.println(d);
    }
}