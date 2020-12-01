//
// Created by viciu on 17.02.2020.
//

#include "defines.h"
#include "webserver.h"
template<typename T, std::size_t N> constexpr std::size_t capacity_null_terminated_char_array(const T(&)[N]) {
    return N - 1;
}

String line_from_value(const String& name, const String& value) {
    String s = F("<br/>{n}: {v}");
    s.replace("{n}", name);
    s.replace("{v}", value);
    return s;
}

static int constexpr constexprstrlen(const char* str) {
    return *str ? 1 + constexprstrlen(str + 1) : 0;
}


void sendHttpRedirect(ESP8266WebServer &httpServer) {
    httpServer.sendHeader(F("Location"), F("http://192.168.4.1/config"));
    httpServer.send(302, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), "");
}

String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi) {
    String s = F(	"<tr>"
                     "<td>"
                     "<a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>{n}</a>&nbsp;{e}"
                     "</td>"
                     "<td style='width:80%;vertical-align:middle;'>"
                     "{v}%"
                     "</td>"
                     "</tr>");
    s.replace("{n}", ssid);
    s.replace("{e}", encryption);
    s.replace("{v}", String(calcWiFiSignalQuality(rssi)));
    return s;
}

String warning_first_cycle() {
    String s = FPSTR(INTL_TIME_TO_FIRST_MEASUREMENT);
    unsigned long time_to_first = cfg::sending_intervall_ms - msSince(starttime);
    if (time_to_first > cfg::sending_intervall_ms) {
        time_to_first = 0;
    }
    s.replace("{v}", String((long)((time_to_first + 500) / 1000)));
    return s;
}

String age_last_values() {
    String s = "<b>";
    unsigned long time_since_last = msSince(starttime);
    if (time_since_last > cfg::sending_intervall_ms) {
        time_since_last = 0;
    }
    s += String((long)((time_since_last + 500) / 1000));
    s += FPSTR(INTL_TIME_SINCE_LAST_MEASUREMENT);
    s += F("</b><br/><br/>");
    return s;
}


void webserver_not_found() {
    last_page_load = millis();
    debug_out(F("output not found page..."), DEBUG_MIN_INFO, 1);
    if (WiFi.status() != WL_CONNECTED) {
        if ((server.uri().indexOf(F("success.html")) != -1) || (server.uri().indexOf(F("detect.html")) != -1)) {
            server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), FPSTR(WEB_IOS_REDIRECT));
        } else {
            sendHttpRedirect(server);
        }
    } else {
        server.send(404, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), F("Not found."));
    }
}


void webserver_images() {
    if (server.arg("name") == F("luftdaten_logo")) {
//		debug_out(F("output luftdaten.info logo..."), DEBUG_MAX_INFO, 1);
        server.send(200, FPSTR(TXT_CONTENT_TYPE_IMAGE_SVG), FPSTR(LUFTDATEN_INFO_LOGO_SVG));
    } else {
        webserver_not_found();
    }
}
/*****************************************************************
 * Webserver request auth: prompt for BasicAuth
 *
 * -Provide BasicAuth for all page contexts except /values and images
 *****************************************************************/
bool webserver_request_auth() {
    debug_out(F("validate request auth..."), DEBUG_MIN_INFO, 1);
    if (cfg::www_basicauth_enabled && ! wificonfig_loop) {
        if (!server.authenticate(cfg::www_username, cfg::www_password)) {
            server.requestAuthentication();
            return false;
        }
    }
    return true;
}
void webserver_dump_stack(){
    if (!SPIFFS.exists ("/stack_dump")) {
        server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), "No stack dump");
        return;
    }
    File dump;
    char buf[100];
    dump = SPIFFS.open("/stack_dump","r");
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), "");
    unsigned size = dump.size();
    for (byte i=0; i<=size/100; i++) {
        dump.readBytes(buf,99);
        server.sendContent(buf);
    }
//    sprintf(buf,"File size: %i bytes\n",size);
//    server.sendContent(buf);

}
/*****************************************************************
 * Webserver root: show all options                              *
 *****************************************************************/
void webserver_root() {
    if (WiFi.status() != WL_CONNECTED) {
        sendHttpRedirect(server);
    } else {
        if (!webserver_request_auth())
        { return; }

        String page_content = make_header(cfg::fs_ssid);
        last_page_load = millis();
        debug_out(F("output root page..."), DEBUG_MIN_INFO, 1);
        page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
        page_content.replace("{t}", FPSTR(INTL_CURRENT_DATA));
        //page_content.replace(F("{map}"), FPSTR(INTL_ACTIVE_SENSORS_MAP));
        page_content.replace(F("{conf}"), FPSTR(INTL_CONFIGURATION));
        page_content.replace(F("{conf_delete}"), FPSTR(INTL_CONFIGURATION_DELETE));
        page_content.replace(F("{restart}"), FPSTR(INTL_RESTART_SENSOR));
        page_content.replace(F("{debug_level}"), FPSTR(INTL_DEBUG_LEVEL));
        page_content.replace(F("{none}"), FPSTR(INTL_NONE));
        page_content.replace(F("{error}"), FPSTR(INTL_ERROR));
        page_content.replace(F("{warning}"), FPSTR(INTL_WARNING));
        page_content.replace(F("{min_info}"), FPSTR(INTL_MIN_INFO));
        page_content.replace(F("{med_info}"), FPSTR(INTL_MED_INFO));
        page_content.replace(F("{max_info}"), FPSTR(INTL_MAX_INFO));
        page_content += make_footer();
        server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
    }
}

/*****************************************************************
 * html helper functions                                         *
 *****************************************************************/

String make_header(const String& title) {
    String s = FPSTR(WEB_PAGE_HEADER);
    s.replace("{tt}", FPSTR(INTL_PM_SENSOR));
    s.replace("{h}", FPSTR(INTL_HOME));
    if(title != " ") {
        s.replace("{n}", F("&raquo;"));
    } else {
        s.replace("{n}", "");
    }
    String v = String(SOFTWARE_VERSION);
#ifdef BUILD_TIME
    v+="(";
    char timestamp[16];
    struct tm ts;
    time_t t = BUILD_TIME;
    ts = *localtime(&t);
    strftime(timestamp,16, "%Y%m%d-%H%M%S", &ts);
    v+=String(timestamp);
    v+=")";
#endif
    s.replace("{t}", title);
    s.replace("{sname}", cfg::fs_ssid);
    s.replace("{id}", esp_chipid());
    s.replace("{mac}", WiFi.macAddress());
    s.replace("{fwt}", FPSTR(INTL_FIRMWARE));
    s.replace("{fw}", v);
    return s;
}

String make_footer() {
    String s = FPSTR(WEB_PAGE_FOOTER);
    s.replace("{t}", FPSTR(INTL_BACK_TO_HOME));
    return s;
}
//Webserver - current config as JSON (txt) to save
void webserver_config_json() {

    if (!webserver_request_auth())
    { return; }
    String page_content = getMaskedConfigString();
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), page_content);
}


//Webserver - force update with custom URL
void webserver_config_force_update() {

    if (!webserver_request_auth())
    { return; }
    String page_content = make_header(FPSTR(INTL_CONFIGURATION));
    if (server.method() == HTTP_POST) {
        if (server.hasArg("host") && server.hasArg("path") && server.hasArg("port")) {
            cfg::auto_update = true;
            updateFW(server.arg("host"), server.arg("port"), server.arg("path"));
            server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
            delay(5000);
            ESP.restart();
        }
        else {
            server.sendHeader(F("Location"), F("/"));
        }

    }else {

        page_content += F("<h2>Force update</h2>");
        page_content += F("<form method='POST' action='/forceUpdate' style='width:100%;'>");
        page_content += F("HOST:<input name=\"host\" value=");
        page_content += UPDATE_HOST;
        page_content += ("></br>");
        page_content += F("PORT:<input name=\"port\" value=");
        page_content += UPDATE_PORT;
        page_content += ("></br>");
        page_content += F("PATH:<input name=\"path\" value=");
        page_content += UPDATE_URL;
        page_content += ("></br>");
        page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
        page_content += F("</form>");
        page_content += make_footer();


    }
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}


//Webserver - current config as JSON (txt) to save
void webserver_config_json_save() {

    if (!webserver_request_auth())
    { return; }
    String page_content = make_header(FPSTR(INTL_CONFIGURATION));
    if (server.method() == HTTP_POST) {
        if (server.hasArg("json")) {
            if (writeConfigRaw(server.arg("json"),"/test.json")) {
                server.send(500, TXT_CONTENT_TYPE_TEXT_PLAIN,F("Error writing config"));
                return; //we dont have reason to restart, current config was not altered yet
            };
            File tempCfg = SPIFFS.open ("/test.json", "r");
            if (readAndParseConfigFile(tempCfg)) {
                server.send(500, TXT_CONTENT_TYPE_TEXT_PLAIN,F("Error parsing config"));
                delay(500);
                ESP.restart(); // we dont know in what state config is. Maybe something was read maybe not
                return;
            }
            //now config is mix of and new config file. Should be save to save it
            writeConfig();
            server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
            delay(5000);
            Serial.println(F("RESET"));
            ESP.restart();
        }
        else {
            server.sendHeader(F("Location"), F("/"));
        }

    }else {
        page_content += F("<a href=\"/config.json\" target=\"_blank\">Sensor config in JSON</a></br></br>");

        page_content += F("<form name=\"json_form\" method='POST' action='/configSave.json' style='width:100%;'>");
        page_content += F("<textarea id=\"json\" name=\"json\" rows=\"10\" cols=\"120\"></textarea></br>");
        page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
        page_content += F("</form>");
        page_content += make_footer();


    }
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}



/*****************************************************************
 * Webserver config: show config page                            *
 *****************************************************************/
void webserver_config() {
    if (!webserver_request_auth()) { return; }

    String page_content = make_header(FPSTR(INTL_CONFIGURATION));
    String masked_pwd = "";
    last_page_load = millis();

    debug_out(F("output config page ..."), DEBUG_MIN_INFO, 1);
    if (wificonfig_loop) {  // scan for wlan ssids
        page_content += FPSTR(WEB_CONFIG_SCRIPT);
    }

    using namespace cfg;
    if (server.method() == HTTP_GET) {
        page_content += F("<form method='POST' action='/config' style='width:100%;'>\n<b>");
        page_content += FPSTR(INTL_WIFI_SETTINGS);
        page_content += F("</b><br/>");
        debug_out(F("output config page 1"), DEBUG_MIN_INFO, 1);
        if (wificonfig_loop) {  // scan for wlan ssids
            page_content += F("<div id='wifilist'>");
            page_content += FPSTR(INTL_WIFI_NETWORKS);
            page_content += F("</div><br/>");
        }
        page_content += FPSTR(TABLE_TAG_OPEN);
        page_content += form_input("wlanssid", FPSTR(INTL_FS_WIFI_NAME), wlanssid,
                                   capacity_null_terminated_char_array(wlanssid));
        if (!wificonfig_loop) {
            page_content += form_password("wlanpwd", FPSTR(INTL_PASSWORD), wlanpwd,
                                          capacity_null_terminated_char_array(wlanpwd));
        } else {
            page_content += form_input("wlanpwd", FPSTR(INTL_PASSWORD), "",
                                       capacity_null_terminated_char_array(wlanpwd));
        }
        page_content += FPSTR(TABLE_TAG_CLOSE_BR);
        page_content += F("<hr/>\n<b>");

        page_content += FPSTR(INTL_AB_HIER_NUR_ANDERN);
        page_content += F("</b><br/><br/>\n<b>");

        if (!wificonfig_loop) {
            page_content += FPSTR(INTL_BASICAUTH);
            page_content += F("</b><br/>");
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input("www_username", FPSTR(INTL_USER), www_username,
                                       capacity_null_terminated_char_array(www_username));
            page_content += form_password("www_password", FPSTR(INTL_PASSWORD), www_password,
                                          capacity_null_terminated_char_array(www_password));
            page_content += form_checkbox("www_basicauth_enabled", FPSTR(INTL_BASICAUTH), www_basicauth_enabled);

            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("\n<b>");
            page_content += FPSTR(INTL_FS_WIFI);
            page_content += F("</b><br/>");
            page_content += FPSTR(INTL_FS_WIFI_DESCRIPTION);
            page_content += FPSTR(BR_TAG);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input("fs_ssid", FPSTR(INTL_FS_WIFI_NAME), fs_ssid,
                                       capacity_null_terminated_char_array(fs_ssid));
            page_content += form_password("fs_pwd", FPSTR(INTL_PASSWORD), fs_pwd,
                                          capacity_null_terminated_char_array(fs_pwd));

            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("\n<b>APIs</b><br/>");
            page_content += form_checkbox("send2dusti", F("API Luftdaten.info"), send2dusti, false);
            page_content += F("&nbsp;&nbsp;(");
            page_content += form_checkbox("ssl_dusti", F("HTTPS"), ssl_dusti, false);
            page_content += F(")<br/>");
            page_content += form_checkbox("send2madavi", F("API Madavi.de"), send2madavi, false);
            page_content += F("&nbsp;&nbsp;(");
            page_content += form_checkbox("ssl_madavi", F("HTTPS"), ssl_madavi, false);
            page_content += F(")<br/><br/>\n<b>");

            page_content += FPSTR(INTL_SENSORS);
            page_content += F("</b><br/>");
            page_content += form_checkbox_sensor("sds_read", FPSTR(INTL_SDS011), sds_read);
            page_content += form_checkbox_sensor("pms_read", FPSTR(INTL_PMS), pms_read);
            page_content += form_checkbox_sensor("dht_read", FPSTR(INTL_DHT22), dht_read);
            page_content += form_checkbox_sensor("bmp280_read", FPSTR(INTL_BMP280), bmp280_read);
            page_content += form_checkbox_sensor("bme280_read", FPSTR(INTL_BME280), bme280_read);
            page_content += form_checkbox_sensor("heca_read", FPSTR(INTL_HECA), heca_read);
            page_content += form_checkbox_sensor("ds18b20_read", FPSTR(INTL_DS18B20), ds18b20_read);
            page_content += form_checkbox("winsen_mhz14a_read", FPSTR(INTL_MHZ14A), winsen_mhz14a_read);
            page_content += form_checkbox("gps_read", FPSTR(INTL_NEO6M), gps_read);
            page_content += F("<br/><br/>\n<b>");
        }

        page_content += FPSTR(INTL_MORE_SETTINGS);
        page_content += F("</b><br/>");
        page_content += form_checkbox("auto_update", FPSTR(INTL_AUTO_UPDATE), auto_update, false);
        page_content += F(" <select name=\"channel\">");
        page_content += form_option(String(UPDATE_CHANNEL_STABLE), FPSTR(INTL_UPDATE_STABLE), update_channel == UPDATE_CHANNEL_STABLE);
        page_content += form_option(String(UPDATE_CHANNEL_BETA), FPSTR(INTL_UPDATE_BETA), update_channel == UPDATE_CHANNEL_BETA);
        page_content += form_option(String(UPDATE_CHANNEL_ALFA), FPSTR(INTL_UPDATE_ALFA), update_channel == UPDATE_CHANNEL_ALFA);
        page_content += F("</select></br>");
        page_content += form_checkbox("has_display", FPSTR(INTL_DISPLAY), has_display);
        page_content += form_checkbox("has_lcd", FPSTR(INTL_LCD),
                                      has_lcd1602 || has_lcd1602_27 || has_lcd2004_3f || has_lcd2004_27, false);
        page_content += F(" <select name=\"lcd_type\">");
        page_content += form_option("1", FPSTR(INTL_LCD1602_27), has_lcd1602_27);
        page_content += form_option("2", FPSTR(INTL_LCD1602_3F), has_lcd1602);
        page_content += form_option("3", FPSTR(INTL_LCD2004_27), has_lcd2004_27);
        page_content += form_option("4", FPSTR(INTL_LCD2004_3F), has_lcd2004_3f);
        page_content += F("</select></br>");
        page_content += form_checkbox("show_wifi_info", FPSTR(INTL_SHOW_WIFI_INFO), show_wifi_info);
        page_content += form_checkbox("has_ledbar_32", FPSTR(INTL_LEDBAR_32), has_ledbar_32);
        page_content += F("</br></br>");

        if (wificonfig_loop) { //outputPower should be able to change in both modes
            page_content += form_input("outputPower", FPSTR(INTL_WIFI_TX_PWR), String(outputPower), 5);
            page_content += form_input("phyMode", FPSTR(INTL_WIFI_PHY_MODE), String(phyMode), 5);
        }
        if (!wificonfig_loop) {
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_select_lang();
            page_content += form_input("debug", FPSTR(INTL_DEBUG_LEVEL), String(debug), 1);
            page_content += form_input("sending_intervall_ms", FPSTR(INTL_MEASUREMENT_INTERVAL),
                                       String(sending_intervall_ms / 1000), 5);
            page_content += form_input("time_for_wifi_config", FPSTR(INTL_DURATION_ROUTER_MODE),
                                       String(time_for_wifi_config / 1000), 5);
            page_content += form_input("outputPower", FPSTR(INTL_WIFI_TX_PWR), String(outputPower), 5);
            page_content += form_input("phyMode", FPSTR(INTL_WIFI_PHY_MODE), String(phyMode), 5);
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("\n<b>");

            page_content += FPSTR(INTL_MORE_APIS);
            page_content += F("</b><br/><br/>");
            page_content += form_checkbox("send2csv", tmpl(FPSTR(INTL_SEND_TO), F("CSV")), send2csv);
            page_content += FPSTR(BR_TAG);
            page_content += form_checkbox("send2fsapp", tmpl(FPSTR(INTL_SEND_TO), F("Feinstaub-App")), send2fsapp);
            page_content += FPSTR(BR_TAG);
            page_content += form_checkbox("send2sensemap", tmpl(FPSTR(INTL_SEND_TO), F("OpenSenseMap")), send2sensemap);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input("senseboxid", "senseBox-ID: ", senseboxid,
                                       capacity_null_terminated_char_array(senseboxid));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += form_checkbox("send2custom", FPSTR(INTL_SEND_TO_OWN_API), send2custom);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input(F("host_custom"), FPSTR(INTL_SERVER), host_custom, 60);
            page_content += form_input(F("url_custom"), FPSTR(INTL_PATH), url_custom, 60);
            constexpr int max_port_digits = constexprstrlen("65535");
            page_content += form_input("port_custom", FPSTR(INTL_PORT), String(port_custom), max_port_digits);
            page_content += form_input("user_custom", FPSTR(INTL_USER), user_custom,
                                       capacity_null_terminated_char_array(user_custom));
            page_content += form_password("pwd_custom", FPSTR(INTL_PASSWORD), pwd_custom,
                                          capacity_null_terminated_char_array(pwd_custom));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += form_checkbox("send2influx", tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), send2influx);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input(F("host_influx"), FPSTR(INTL_SERVER), host_influx, 60);
            page_content += form_input(F("url_influx"), FPSTR(INTL_PATH), url_influx, 60);
            page_content += form_input("port_influx", FPSTR(INTL_PORT), String(port_influx), max_port_digits);
            page_content += form_input("user_influx", FPSTR(INTL_USER), user_influx,
                                       capacity_null_terminated_char_array(user_influx));
            page_content += form_password("pwd_influx", FPSTR(INTL_PASSWORD), pwd_influx,
                                          capacity_null_terminated_char_array(pwd_influx));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += form_checkbox("send2mqtt", tmpl(FPSTR(INTL_SEND_TO), F("MQTT")), send2mqtt);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input(F("host_mqtt"), FPSTR(INTL_SERVER), host_mqtt, 60);
            page_content += form_input("port_mqtt", FPSTR(INTL_PORT), String(port_mqtt), max_port_digits);
            page_content += form_input("user_mqtt", FPSTR(INTL_USER), user_mqtt, 128);
            page_content += form_password("pwd_mqtt", FPSTR(INTL_PASSWORD), pwd_mqtt, 256);
            page_content += form_input("client_id_mqtt", FPSTR(INTL_CLIENT_ID), client_id_mqtt,
                                       capacity_null_terminated_char_array(client_id_mqtt));
            page_content += form_input("sensors_topic_mqtt", FPSTR(INTL_SENSORS_TOPIC_MQTT), sensors_topic_mqtt, SENSOR_TOPIC_PREFIX_MQTT_SIZE);

            page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("<br/></form>");

            scheduler.getConfigForms(page_content);
        }
        if (wificonfig_loop) {  // scan for wlan ssids
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("<br/></form>");
            page_content += F("<script>window.setTimeout(load_wifi_list,1000);</script>");
        }
    } else {
#define readCharParam(param) \
        if (server.hasArg(#param)){ \
            server.arg(#param).toCharArray(param, sizeof(param)); \
        }

#define readBoolParam(param) \
        param = false; \
        if (server.hasArg(#param)){ \
            param = server.arg(#param) == "1";\
        }

#define readIntParam(param) \
        if (server.hasArg(#param)){ \
            param = server.arg(#param).toInt(); \
        }

#define readFloatParam(param) \
        if (server.hasArg(#param)){ \
            param = server.arg(#param).toFloat(); \
        }

#define readTimeParam(param) \
        if (server.hasArg(#param)){ \
            int val = server.arg(#param).toInt(); \
            param = val*1000; \
        }

#define readPasswdParam(param) \
        if (server.hasArg(#param)){ \
            masked_pwd = ""; \
            for (uint8_t i=0;i<server.arg(#param).length();i++) \
                masked_pwd += "*"; \
            if (masked_pwd != server.arg(#param) || server.arg(#param) == "") {\
                server.arg(#param).toCharArray(param, sizeof(param)); \
            }\
        }

#define readPasswdStringParam(param) \
        if (server.hasArg(#param)){ \
            masked_pwd = ""; \
            for (uint8_t i=0;i<server.arg(#param).length();i++) \
                masked_pwd += "*";   \
            if (masked_pwd != server.arg(#param) || server.arg(#param) == "") {\
                param = server.arg(#param); \
            }\
        }

        if (server.hasArg("wlanssid") && server.arg("wlanssid") != "") {
            readCharParam(wlanssid);
            readPasswdParam(wlanpwd);
        }
        //always allow to change output power
        readFloatParam(outputPower);
        readIntParam(phyMode);

        if (!wificonfig_loop) {
            readCharParam(current_lang);
            readCharParam(www_username);
            readPasswdParam(www_password);
            readBoolParam(www_basicauth_enabled);
            readCharParam(fs_ssid);
            if (server.hasArg("fs_pwd") &&
                ((server.arg("fs_pwd").length() > 7) || (server.arg("fs_pwd").length() == 0))) {
                readPasswdParam(fs_pwd);
            }
            readBoolParam(send2dusti);
            readBoolParam(ssl_dusti);
            readBoolParam(send2madavi);
            readBoolParam(ssl_madavi);
            readBoolParam(dht_read);
            readBoolParam(sds_read);
            readBoolParam(pms_read);
            readBoolParam(bmp280_read);
            readBoolParam(bme280_read);
            readBoolParam(heca_read);
            readBoolParam(ds18b20_read);
            readBoolParam(gps_read);
            readBoolParam(winsen_mhz14a_read);

            readIntParam(debug);
            readTimeParam(sending_intervall_ms);
            readTimeParam(time_for_wifi_config);

            readBoolParam(send2csv);

            readBoolParam(send2fsapp);

            readBoolParam(send2sensemap);
            readCharParam(senseboxid);

            readBoolParam(send2custom);
            parseHTTP(F("host_custom"), host_custom);
            parseHTTP(F("url_custom"), url_custom);

            readIntParam(port_custom);
            readCharParam(user_custom);
            readPasswdParam(pwd_custom);

            readBoolParam(send2influx);

            parseHTTP(F("host_influx"), host_influx);
            parseHTTP(F("url_influx"), url_influx);

            readIntParam(port_influx);
            readCharParam(user_influx);
            readPasswdParam(pwd_influx);

            readBoolParam(send2mqtt)
            parseHTTP(F("host_mqtt"), host_mqtt);
            readIntParam(port_mqtt);
            parseHTTP(F("user_mqtt"), user_mqtt);
            readPasswdStringParam(pwd_mqtt);
            readCharParam(client_id_mqtt);
            parseHTTP(F("sensors_topic_mqtt"), sensors_topic_mqtt);
            sensors_topic_mqtt.replace(F(" "), F(""));
            if(!sensors_topic_mqtt.endsWith(FPSTR("/"))) {
                debug_out(F("Missing ending slash in mqtt topic prefix"), DEBUG_MIN_INFO, true);
                if (sensors_topic_mqtt.length() == SENSOR_TOPIC_PREFIX_MQTT_SIZE) {
                    sensors_topic_mqtt.remove(SENSOR_TOPIC_PREFIX_MQTT_SIZE-1);
                }
                sensors_topic_mqtt += FPSTR("/");
            }
        }

        readBoolParam(auto_update);
        parseHTTP(F("channel"), update_channel);

        readBoolParam(has_display);
        has_lcd1602 = false;
        has_lcd1602_27 = false;
        has_lcd2004_27 = false;
        has_lcd2004_3f = false;
        if (server.hasArg("has_lcd")) {
            switch (server.arg("lcd_type").toInt()) {
                case 1:
                    has_lcd1602_27 = true;
                    break;
                case 2:
                    has_lcd1602 = true;
                    break;
                case 3:
                    has_lcd2004_27 = true;
                    break;
                case 4:
                    has_lcd2004_3f = true;
                    break;
            }
        }
        readBoolParam(show_wifi_info);
        readBoolParam(has_ledbar_32);

#undef readCharParam
#undef readBoolParam
#undef readIntParam
#undef readTimeParam
#undef readPasswdParam
#undef readPasswdStringParam

        page_content += line_from_value(tmpl(FPSTR(INTL_SEND_TO), F("Luftdaten.info")), String(send2dusti));
        page_content += line_from_value(tmpl(FPSTR(INTL_SEND_TO), F("Madavi")), String(send2madavi));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "DHT"), String(dht_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "SDS"), String(sds_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), F("PMS(1,3,5,6,7)003")), String(pms_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "BMP280"), String(bmp280_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "BME280"), String(bme280_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "HECA"), String(heca_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), "DS18B20"), String(ds18b20_read));
        page_content += line_from_value(tmpl(FPSTR(INTL_READ_FROM), F("GPS")), String(gps_read));
        page_content += line_from_value(FPSTR(INTL_AUTO_UPDATE), String(auto_update));
        page_content += String(update_channel);
        page_content += line_from_value(FPSTR(INTL_DISPLAY), String(has_display));
        page_content += line_from_value(FPSTR(INTL_LCD1602_27), String(has_lcd1602_27));
        page_content += line_from_value(FPSTR(INTL_LCD1602_3F), String(has_lcd1602));
        page_content += line_from_value(FPSTR(INTL_LCD2004_27), String(has_lcd2004_27));
        page_content += line_from_value(FPSTR(INTL_LCD2004_3F), String(has_lcd2004_3f));
        page_content += line_from_value(FPSTR(INTL_SHOW_WIFI_INFO), String(show_wifi_info));
        page_content += line_from_value(FPSTR(INTL_LEDBAR_32), String(has_ledbar_32));
        page_content += line_from_value(FPSTR(INTL_DEBUG_LEVEL), String(debug));
        page_content += line_from_value(FPSTR(INTL_MEASUREMENT_INTERVAL), String(sending_intervall_ms/1000));
        page_content += line_from_value(tmpl(FPSTR(INTL_SEND_TO), F("CSV")), String(send2csv));
        page_content += line_from_value(tmpl(FPSTR(INTL_SEND_TO), F("Feinstaub-App")), String(send2fsapp));
        page_content += line_from_value(tmpl(FPSTR(INTL_SEND_TO), F("opensensemap")), String(send2sensemap));
        page_content += F("<br/>senseBox-ID ");
        page_content += senseboxid;
        page_content += F("<br/><br/>");
        page_content += line_from_value(FPSTR(INTL_SEND_TO_OWN_API), String(send2custom));
        page_content += line_from_value(FPSTR(INTL_SERVER), host_custom);
        page_content += line_from_value(FPSTR(INTL_PATH), url_custom);
        page_content += line_from_value(FPSTR(INTL_PORT), String(port_custom));
        page_content += line_from_value(FPSTR(INTL_USER), user_custom);
        page_content += line_from_value(FPSTR(INTL_PASSWORD), pwd_custom);
        page_content += F("<br/><br/>InfluxDB: ");
        page_content += String(send2influx);
        page_content += line_from_value(FPSTR(INTL_SERVER), host_influx);
        page_content += line_from_value(FPSTR(INTL_PATH), url_influx);
        page_content += line_from_value(FPSTR(INTL_PORT), String(port_influx));
        page_content += line_from_value(FPSTR(INTL_USER), user_influx);
        page_content += line_from_value(FPSTR(INTL_PASSWORD), pwd_influx);
        page_content += F("<br/><br/>MQTT: ");
        page_content += String(send2mqtt);
        page_content += line_from_value(FPSTR(INTL_SERVER), host_mqtt);
        page_content += line_from_value(FPSTR(INTL_PORT), String(port_mqtt));
        page_content += line_from_value(FPSTR(INTL_CLIENT_ID), client_id_mqtt);
        page_content += line_from_value(FPSTR(INTL_USER), user_mqtt);
        page_content += line_from_value(FPSTR(INTL_PASSWORD), pwd_mqtt);
        page_content += line_from_value(FPSTR(INTL_SENSORS_TOPIC_MQTT), sensors_topic_mqtt);

        page_content += F("<br/><br/>");
        page_content += FPSTR(INTL_SENSOR_IS_REBOOTING);
    }
    page_content += make_footer();

    server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    server.sendHeader(F("Pragma"), F("no-cache"));
    server.sendHeader(F("Expires"), F("0"));
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);

    if (server.method() == HTTP_POST) {
        debug_out(F("Writing config and restarting"), DEBUG_MIN_INFO, true);
        display_debug(F("Writing config"), F("and restarting"));
        writeConfig();
        delay(500);
        ESP.restart();
    }
}

/**************************************************************************
 * Parse sensor config - new, simple scheduler
 **************************************************************************/

void webserver_simple_config() {

    if (!webserver_request_auth()) { return; }

    String page_content = make_header(FPSTR(INTL_CONFIGURATION));
    last_page_load = millis();

    debug_out(F("output config page ..."), DEBUG_MIN_INFO, 1);

//    if (server.method() == HTTP_POST) {

        if (server.hasArg(F("sensor"))) {
            SimpleScheduler::LoopEntryType sensor;
            sensor = static_cast<SimpleScheduler::LoopEntryType>(server.arg(F("sensor")).toInt());
            page_content += F("Sensor val: ");
            page_content += String(sensor);
            page_content += F("<br>");
            JsonObject &ret = SimpleScheduler::parseHTTPConfig(sensor);
            ret.printTo(Serial);
            if (ret.containsKey(F("err"))){
                page_content += F("<h2>");
                page_content += String(ret.get<char *>(F("err")) );//ret.get<char *>(F("err"));
                page_content += F("</h2>");

            } else {
                SimpleScheduler::readConfigJSON(sensor, ret);
            }

            writeConfig();

        }

        page_content += make_footer();

        server.sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
        server.sendHeader(F("Pragma"), F("no-cache"));
        server.sendHeader(F("Expires"), F("0"));
        server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
//    } else {
//        webserver_not_found();
//    }
}


String table_row_from_value(const String &sensor, const String &param, const String &value, const String &unit) {
    String s = F("<tr>"
                 "<td>{s}</td>"
                 "<td>{p}</td>"
                 "<td class='r'>{v}&nbsp;{u}</td>"
                 "</tr>");
    s.replace("{s}", sensor);
    s.replace("{p}", param);
    s.replace("{v}", value);
    s.replace("{u}", unit);
    return s;
}

/*****************************************************************
 * Webserver wifi: show available wifi networks                  *
 *****************************************************************/
void webserver_wifi() {
    debug_out(F("wifi networks found: "), DEBUG_MIN_INFO, 0);
    debug_out(String(count_wifiInfo), DEBUG_MIN_INFO, 1);
    String page_content = "";
    if (count_wifiInfo == 0) {
        page_content += BR_TAG;
        page_content += FPSTR(INTL_NO_NETWORKS);
        page_content += BR_TAG;
    } else {
        std::unique_ptr<int[]> indices(new int[count_wifiInfo]);
        debug_out(F("output config page 2"), DEBUG_MIN_INFO, 1);
        for (int i = 0; i < count_wifiInfo; i++) {
            indices[i] = i;
        }
        for (int i = 0; i < count_wifiInfo; i++) {
            for (int j = i + 1; j < count_wifiInfo; j++) {
                if (wifiInfo[indices[j]].RSSI > wifiInfo[indices[i]].RSSI) {
                    std::swap(indices[i], indices[j]);
                }
            }
        }
        debug_out(F("output config page 3"), DEBUG_MIN_INFO, 1);
        int duplicateSsids = 0;
        for (int i = 0; i < count_wifiInfo; i++) {
            if (indices[i] == -1) {
                continue;
            }
            for (int j = i + 1; j < count_wifiInfo; j++) {
                if (strncmp(wifiInfo[indices[i]].ssid, wifiInfo[indices[j]].ssid, 35) == 0) {
                    indices[j] = -1; // set dup aps to index -1
                    ++duplicateSsids;
                }
            }
        }

        page_content += FPSTR(INTL_NETWORKS_FOUND);
        page_content += String(count_wifiInfo - duplicateSsids);
        page_content += FPSTR(BR_TAG);
        page_content += FPSTR(BR_TAG);
        page_content += FPSTR(TABLE_TAG_OPEN);
        //if(n > 30) n=30;
        for (int i = 0; i < count_wifiInfo; ++i) {
            if (indices[i] == -1 || wifiInfo[indices[i]].isHidden) {
                continue;
            }
            // Print SSID and RSSI for each network found
            page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == ENC_TYPE_NONE) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
        }
        page_content += FPSTR(TABLE_TAG_CLOSE_BR);
        page_content += FPSTR(BR_TAG);
    }
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
void webserver_values() {
    if (WiFi.status() != WL_CONNECTED) {
        sendHttpRedirect(server);
    } else {
        String page_content = make_header(FPSTR(INTL_CURRENT_DATA));
        const String unit_PM = "µg/m³";
        const String unit_T = "°C";
        const String unit_H = "%";
        const String unit_P = "hPa";
        last_page_load = millis();

        const int signal_quality = calcWiFiSignalQuality(WiFi.RSSI());
        debug_out(F("output values to web page..."), DEBUG_MIN_INFO, 1);
        if (first_cycle) {
            page_content += F("<b style='color:red'>");
            page_content += warning_first_cycle();
            page_content += F("</b><br/><br/>");
        } else {
            page_content += age_last_values();
        }
        page_content += F("<table cellspacing='0' border='1' cellpadding='5'>");
        page_content += tmpl(F("<tr><th>{v1}</th><th>{v2}</th><th>{v3}</th>"), FPSTR(INTL_SENSOR), FPSTR(INTL_PARAMETER), FPSTR(INTL_VALUE));
        if (cfg::sds_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM2.5", check_display_value(last_value_SDS_P2, -1, 1, 0), unit_PM);
            page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM10", check_display_value(last_value_SDS_P1, -1, 1, 0), unit_PM);
        }
        if (cfg::winsen_mhz14a_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(INTL_MHZ14A_VAL), F("CO2"), String(last_value_WINSEN_CO2),
                                                 F("ppm"));
        }
        if (cfg::pms_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM1", check_display_value(last_value_PMS_P0, -1, 1, 0), unit_PM);
            page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM2.5", check_display_value(last_value_PMS_P2, -1, 1, 0), unit_PM);
            page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM10", check_display_value(last_value_PMS_P1, -1, 1, 0), unit_PM);
        }
        if (cfg::dht_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_DHT_T, -128, 1, 0), unit_T);
            page_content += table_row_from_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_HUMIDITY), check_display_value(last_value_DHT_H, -1, 1, 0), unit_H);
        }
        if (cfg::bmp280_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_BMP280), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_BMP280_T, -128, 1, 0), unit_T);
            page_content += table_row_from_value(FPSTR(SENSORS_BMP280), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMP280_P / 100.0, (-1 / 100.0), 2, 0), unit_P);
        }
        if (cfg::bme280_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_BME280_T, -128, 1, 0), unit_T);
            page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_HUMIDITY), check_display_value(last_value_BME280_H, -1, 1, 0), unit_H);
            page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_PRESSURE),  check_display_value(last_value_BME280_P / 100.0, (-1 / 100.0), 2, 0), unit_P);
        }
        if (cfg::heca_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_HECA_T, -128, 1, 0), unit_T);
            page_content += table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_HUMIDITY), check_display_value(last_value_HECA_H, -1, 1, 0), unit_H);
        }
        if (cfg::ds18b20_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(SENSORS_DS18B20), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_DS18B20_T, -128, 1, 0), unit_T);
        }
        if (cfg::gps_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(F("GPS"), FPSTR(INTL_LATITUDE), check_display_value(last_value_GPS_lat, -200.0, 6, 0), "°");
            page_content += table_row_from_value(F("GPS"), FPSTR(INTL_LONGITUDE), check_display_value(last_value_GPS_lon, -200.0, 6, 0), "°");
            page_content += table_row_from_value(F("GPS"), FPSTR(INTL_ALTITUDE),  check_display_value(last_value_GPS_alt, -1000.0, 2, 0), "m");
            page_content += table_row_from_value(F("GPS"), FPSTR(INTL_DATE), last_value_GPS_date, "");
            page_content += table_row_from_value(F("GPS"), FPSTR(INTL_TIME), last_value_GPS_time, "");
        }

        SimpleScheduler::getResultsAsHTML(page_content);

        page_content += FPSTR(EMPTY_ROW);
        page_content += table_row_from_value(F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH),  String(WiFi.RSSI()), "dBm");
        page_content += table_row_from_value(F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");
        page_content += FPSTR(EMPTY_ROW);
        page_content += table_row_from_value(F("NAM"),FPSTR(INTL_NUMBER_OF_MEASUREMENTS),String(count_sends),"");
        page_content += table_row_from_value(F("NAM"),F("Uptime"), millisToTime(millis()),"");
        page_content += FPSTR(EMPTY_ROW);
        page_content += table_row_from_value(F("ESP"),F("Reset Reason"), String(ESP.getResetReason()),"");
        String tmp = String(memoryStatsMin.maxFreeBlock) + String("/") + String(memoryStatsMax.maxFreeBlock);
        page_content += table_row_from_value(F("ESP"),F("Max Free Block Size"), tmp,"B");
        tmp = String(memoryStatsMin.frag) + String("/") + String(memoryStatsMax.frag);
        page_content += table_row_from_value(F("ESP"),F("Heap Fragmentation"), tmp,"%");
        tmp = String(memoryStatsMin.freeContStack) + String("/") + String(memoryStatsMax.freeContStack);
        page_content += table_row_from_value(F("ESP"),F("Free Cont Stack"), tmp,"B");
        tmp = String(memoryStatsMin.freeHeap) + String("/") + String(memoryStatsMax.freeHeap);
        page_content += table_row_from_value(F("ESP"),F("Free Memory"), tmp,"B");
        page_content += table_row_from_value(F("ESP"),F("Flash ID"), String(ESP.getFlashChipId()),"");
        page_content += table_row_from_value(F("ESP"),F("Flash Vendor ID"), String(ESP.getFlashChipVendorId()),"");
        page_content += table_row_from_value(F("ESP"),F("Flash Speed"), String(ESP.getFlashChipSpeed()/1000000.0),"MHz");
        page_content += table_row_from_value(F("ESP"),F("Flash Mode"), String(ESP.getFlashChipMode()),"");
        page_content += FPSTR(EMPTY_ROW);
        page_content += table_row_from_value(F("ENV"),F("Core version"), String(ESP.getCoreVersion()),"");
        page_content += table_row_from_value(F("ENV"),F("SDK version"), String(ESP.getSdkVersion()),"");
        page_content += FPSTR(TABLE_TAG_CLOSE_BR);
        page_content += make_footer();
        server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
    }
}

/*****************************************************************
 * Webserver set debug level                                     *
 *****************************************************************/
void webserver_debug_level() {
    if (!webserver_request_auth()) { return; }

    String page_content = make_header(FPSTR(INTL_DEBUG_LEVEL));
    last_page_load = millis();
    debug_out(F("output change debug level page..."), DEBUG_MIN_INFO, 1);

    if (server.hasArg("lvl")) {
        const int lvl = server.arg("lvl").toInt();
        if (lvl >= 0 && lvl <= 5) {
            cfg::debug = lvl;
            page_content += F("<h3>");
            page_content += FPSTR(INTL_DEBUG_SETTING_TO);
            page_content += F(" ");

            static constexpr std::array<const char *, 6> lvlText PROGMEM = {
                    INTL_NONE, INTL_ERROR, INTL_WARNING, INTL_MIN_INFO, INTL_MED_INFO, INTL_MAX_INFO
            };

            page_content += FPSTR(lvlText[lvl]);
            page_content += F(".</h3>");
        }
    }
    page_content += make_footer();
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver enable ota                                          *
 *****************************************************************/
 void webserver_enable_ota() {
    String page_content;
    if (cfg::www_basicauth_enabled) {
        if (!webserver_request_auth()) { return; }
        page_content = make_header(FPSTR(INTL_ENABLE_OTA));

        last_page_load = millis();
        enable_ota_time = millis() + 60 * 1000;

        page_content += FPSTR(INTL_ENABLE_OTA_INFO);

        page_content += make_footer();
    } else {
        page_content = make_header(FPSTR(INTL_ENABLE_OTA));
        page_content += FPSTR(INTL_ENABLE_OTA_REFUSE);
        page_content += make_footer();

    }
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
void webserver_removeConfig() {
    if (!webserver_request_auth()) { return; }

    String page_content = make_header(FPSTR(INTL_DELETE_CONFIG));
    String message_string = F("<h3>{v}.</h3>");
    last_page_load = millis();
    debug_out(F("output remove config page..."), DEBUG_MIN_INFO, 1);

    if (server.method() == HTTP_GET) {
        page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);
        page_content.replace("{t}", FPSTR(INTL_CONFIGURATION_REALLY_DELETE));
        page_content.replace("{b}", FPSTR(INTL_DELETE));
        page_content.replace("{c}", FPSTR(INTL_CANCEL));

    } else {
        if (SPIFFS.exists("/config.json")) {	//file exists
            debug_out(F("removing config.json..."), DEBUG_MIN_INFO, 1);
            if (SPIFFS.remove("/config.json")) {
                page_content += tmpl(message_string, FPSTR(INTL_CONFIG_DELETED));
            } else {
                page_content += tmpl(message_string, FPSTR(INTL_CONFIG_CAN_NOT_BE_DELETED));
            }
        } else {
            page_content += tmpl(message_string, FPSTR(INTL_CONFIG_NOT_FOUND));
        }
    }
    page_content += make_footer();
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver reset NodeMCU                                       *
 *****************************************************************/
void webserver_reset() {
    if (!webserver_request_auth()) { return; }

    String page_content = make_header(FPSTR(INTL_RESTART_SENSOR));
    last_page_load = millis();
    debug_out(F("output reset NodeMCU page..."), DEBUG_MIN_INFO, 1);

    if (server.method() == HTTP_GET) {
        page_content += FPSTR(WEB_RESET_CONTENT);
        page_content.replace("{t}", FPSTR(INTL_REALLY_RESTART_SENSOR));
        page_content.replace("{b}", FPSTR(INTL_RESTART));
        page_content.replace("{c}", FPSTR(INTL_CANCEL));
    } else {
        ESP.restart();
    }
    page_content += make_footer();
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
void webserver_data_json() {
    String s1 = "";
    unsigned long age = 0;
    debug_out(F("output data json..."), DEBUG_MIN_INFO, 1);
    if (first_cycle) {
        s1 = FPSTR(data_first_part);
        s1.replace("{v}", String(SOFTWARE_VERSION));
        s1 += "]}";
        age = cfg::sending_intervall_ms - msSince(starttime);
        if (age > cfg::sending_intervall_ms) {
            age = 0;
        }
        age = 0 - age;
    } else {
        s1 = last_data_string;
        debug_out(F("last data: "), DEBUG_MIN_INFO, 0);
        debug_out(s1, DEBUG_MIN_INFO, 1);
        age = msSince(starttime);
        if (age > cfg::sending_intervall_ms) {
            age = 0;
        }
    }
    String s2 = F(", \"age\":\"");
    s2 += String((long)((age + 500) / 1000));
    s2 += String(F("\", \"measurements\":\""));
    s2 += String((unsigned long)(count_sends));
    s2 += F("\", \"sensordatavalues\"");
    debug_out(F("replace with: "), DEBUG_MIN_INFO, 0);
    debug_out(s2, DEBUG_MIN_INFO, 1);
    s1.replace(F(", \"sensordatavalues\""), s2);
    debug_out(F("replaced: "), DEBUG_MIN_INFO, 0);
    debug_out(s1, DEBUG_MIN_INFO, 1);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), s1);
}

/*****************************************************************
 * Webserver prometheus metrics endpoint                         *
 *****************************************************************/
void webserver_prometheus_endpoint() {
    debug_out(F("output prometheus endpoint..."), DEBUG_MIN_INFO, 1);
    String data_4_prometheus = F("software_version{version=\"{ver}\",{id}} 1\nuptime_ms{{id}} {up}\nsending_intervall_ms{{id}} {si}\nnumber_of_measurements{{id}} {cs}\n");
    String id = F("node=\"esp8266-");
    id += esp_chipid() + "\"";
    debug_out(F("Parse JSON for Prometheus"), DEBUG_MIN_INFO, 1);
    debug_out(last_data_string, DEBUG_MED_INFO, 1);
    data_4_prometheus.replace("{id}", id);
    data_4_prometheus.replace("{ver}", String(SOFTWARE_VERSION));
    data_4_prometheus.replace("{up}", String(msSince(time_point_device_start_ms)));
    data_4_prometheus.replace("{si}", String(cfg::sending_intervall_ms));
    data_4_prometheus.replace("{cs}", String(count_sends));
    StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
    JsonObject& json2data = jsonBuffer.parseObject(last_data_string);
    if (json2data.success()) {
        for (uint8_t i = 0; i < json2data["sensordatavalues"].size() - 1; i++) {
            String tmp_str = json2data["sensordatavalues"][i]["value_type"].as<char*>();
            data_4_prometheus += tmp_str + "{" + id + "} ";
            tmp_str = json2data["sensordatavalues"][i]["value"].as<char*>();
            data_4_prometheus += tmp_str + "\n";
        }
        data_4_prometheus += F("last_sample_age_ms{");
        data_4_prometheus += id + "} " + String(msSince(starttime)) + "\n";
    } else {
        debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
    }
    debug_out(data_4_prometheus, DEBUG_MED_INFO, 1);
    server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), data_4_prometheus);
}

/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
void setup_webserver() {
    server.on("/", webserver_root);
    server.on("/config", webserver_config);
    server.on("/simple_config", webserver_simple_config);
    server.on("/config.json", HTTP_GET, webserver_config_json);
    server.on("/configSave.json", webserver_config_json_save);
    server.on("/forceUpdate", webserver_config_force_update);
    server.on("/wifi", webserver_wifi);
    server.on("/values", webserver_values);
    server.on("/debug", webserver_debug_level);
    server.on("/ota", webserver_enable_ota);
    server.on("/removeConfig", webserver_removeConfig);
    server.on("/reset", webserver_reset);
    server.on("/data.json", webserver_data_json);
    server.on("/metrics", webserver_prometheus_endpoint);
    server.on("/images", webserver_images);
    server.on("/stack_dump", webserver_dump_stack);
    server.onNotFound(webserver_not_found);

    debug_out(F("Starting Webserver... "), DEBUG_MIN_INFO, 0);
//	debug_out(IPAddress2String(WiFi.localIP()), DEBUG_MIN_INFO, 1);
    debug_out(WiFi.localIP().toString(), DEBUG_MIN_INFO, 1);
    server.begin();
}
