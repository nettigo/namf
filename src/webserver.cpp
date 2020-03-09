//
// Created by viciu on 17.02.2020.
//

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
    s.replace("{t}", title);
    s.replace("{sname}", cfg::fs_ssid);
    s.replace("{id}", esp_chipid());
    s.replace("{mac}", WiFi.macAddress());
    s.replace("{fwt}", FPSTR(INTL_FIRMWARE));
    s.replace("{fw}", String(SOFTWARE_VERSION));
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
            Serial.println("RESET");
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
            page_content += form_checkbox("gps_read", FPSTR(INTL_NEO6M), gps_read);
            page_content += F("<br/><br/>\n<b>");
        }

        page_content += FPSTR(INTL_MORE_SETTINGS);
        page_content += F("</b><br/>");
        page_content += form_checkbox("auto_update", FPSTR(INTL_AUTO_UPDATE), auto_update);
        page_content += form_checkbox("use_beta", FPSTR(INTL_USE_BETA), use_beta);
        page_content += form_checkbox("has_display", FPSTR(INTL_DISPLAY), has_display);
        page_content += form_checkbox("has_lcd", FPSTR(INTL_LCD),
                                      has_lcd1602 || has_lcd1602_27 || has_lcd2004_3f || has_lcd2004_27, false);
        page_content += F(" <select name=\"lcd_type\">");
        page_content += form_option("1", FPSTR(INTL_LCD1602_27), has_lcd1602_27);
        page_content += form_option("2", FPSTR(INTL_LCD1602_3F), has_lcd1602);
        page_content += form_option("3", FPSTR(INTL_LCD2004_27), has_lcd2004_27);
        page_content += form_option("4", FPSTR(INTL_LCD2004_3F), has_lcd2004_3f);
        page_content += F("</select></br>");
        page_content += form_checkbox("has_ledbar_32", FPSTR(INTL_LEDBAR_32), has_ledbar_32);
        page_content += F("</select></br></br>");

        if (wificonfig_loop) { //outputPower should be able to change in both modes
            page_content += form_input("outputPower", FPSTR(INTL_WIFI_TX_PWR), String(outputPower), 5);
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
            page_content += form_input("host_custom", FPSTR(INTL_SERVER), host_custom,
                                       capacity_null_terminated_char_array(host_custom));
            page_content += form_input("url_custom", FPSTR(INTL_PATH), url_custom,
                                       capacity_null_terminated_char_array(url_custom));
            constexpr int max_port_digits = constexprstrlen("65535");
            page_content += form_input("port_custom", FPSTR(INTL_PORT), String(port_custom), max_port_digits);
            page_content += form_input("user_custom", FPSTR(INTL_USER), user_custom,
                                       capacity_null_terminated_char_array(user_custom));
            page_content += form_password("pwd_custom", FPSTR(INTL_PASSWORD), pwd_custom,
                                          capacity_null_terminated_char_array(pwd_custom));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += form_checkbox("send2influx", tmpl(FPSTR(INTL_SEND_TO), F("InfluxDB")), send2influx);
            page_content += FPSTR(TABLE_TAG_OPEN);
            page_content += form_input("host_influx", FPSTR(INTL_SERVER), host_influx,
                                       capacity_null_terminated_char_array(host_influx));
            page_content += form_input("url_influx", FPSTR(INTL_PATH), url_influx,
                                       capacity_null_terminated_char_array(url_influx));
            page_content += form_input("port_influx", FPSTR(INTL_PORT), String(port_influx), max_port_digits);
            page_content += form_input("user_influx", FPSTR(INTL_USER), user_influx,
                                       capacity_null_terminated_char_array(user_influx));
            page_content += form_password("pwd_influx", FPSTR(INTL_PASSWORD), pwd_influx,
                                          capacity_null_terminated_char_array(pwd_influx));
            page_content += form_submit(FPSTR(INTL_SAVE_AND_RESTART));
            page_content += FPSTR(TABLE_TAG_CLOSE_BR);
            page_content += F("<br/></form>");
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

        if (server.hasArg("wlanssid") && server.arg("wlanssid") != "") {
            readCharParam(wlanssid);
            readPasswdParam(wlanpwd);
        }
        //always allow to change output power
        readFloatParam(outputPower);

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

            readIntParam(debug);
            readTimeParam(sending_intervall_ms);
            readTimeParam(time_for_wifi_config);

            readBoolParam(send2csv);

            readBoolParam(send2fsapp);

            readBoolParam(send2sensemap);
            readCharParam(senseboxid);

            readBoolParam(send2custom);
            readCharParam(host_custom);
            readCharParam(url_custom);
            readIntParam(port_custom);
            readCharParam(user_custom);
            readPasswdParam(pwd_custom);

            readBoolParam(send2influx);
            readCharParam(host_influx);
            readCharParam(url_influx);
            readIntParam(port_influx);
            readCharParam(user_influx);
            readPasswdParam(pwd_influx);

        }

        readBoolParam(auto_update);
        readBoolParam(use_beta);
        readBoolParam(has_display);
        has_lcd1602 = false;
        has_lcd1602_27 = false;
        has_lcd2004_27 = false;
        has_lcd2004_3f = false;
        if (server.hasArg("has_lcd")) {
            switch (server.arg("lcd_type").toInt()) {
                case 1:
                    has_lcd1602 = true;
                    break;
                case 2:
                    has_lcd1602_27 = true;
                    break;
                case 3:
                    has_lcd2004_27 = true;
                    break;
                case 4:
                    has_lcd2004_3f = true;
                    break;
            }
        }
        readBoolParam(has_ledbar_32);

#undef readCharParam
#undef readBoolParam
#undef readIntParam
#undef readTimeParam
#undef readPasswdParam

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
        page_content += line_from_value(FPSTR(INTL_USE_BETA), String(use_beta));
        page_content += line_from_value(FPSTR(INTL_DISPLAY), String(has_display));
        page_content += line_from_value(FPSTR(INTL_LCD1602_27), String(has_lcd1602_27));
        page_content += line_from_value(FPSTR(INTL_LCD1602_3F), String(has_lcd1602));
        page_content += line_from_value(FPSTR(INTL_LCD2004_27), String(has_lcd2004_27));
        page_content += line_from_value(FPSTR(INTL_LCD2004_3F), String(has_lcd2004_3f));
        page_content += line_from_value(FPSTR(INTL_LEDBAR_32), String(has_ledbar_32));
        page_content += line_from_value(FPSTR(INTL_DEBUG_LEVEL), String(debug));
        page_content += line_from_value(FPSTR(INTL_MEASUREMENT_INTERVAL), String(sending_intervall_ms));
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