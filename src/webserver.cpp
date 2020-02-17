//
// Created by viciu on 17.02.2020.
//

#include "webserver.h"

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

        String page_content = make_header(" ");
        last_page_load = millis();
        debug_out(F("output root page..."), DEBUG_MIN_INFO, 1);
        page_content += FPSTR(WEB_ROOT_PAGE_CONTENT);
        page_content.replace("{t}", FPSTR(INTL_CURRENT_DATA));
        page_content.replace(F("{map}"), FPSTR(INTL_ACTIVE_SENSORS_MAP));
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
    s.replace("{id}", esp_chipid());
    s.replace("{mac}", WiFi.macAddress());
    s.replace("{fwt}", FPSTR(INTL_FIRMWARE));
    s.replace("{fw}", SOFTWARE_VERSION);
    return s;
}

String make_footer() {
    String s = FPSTR(WEB_PAGE_FOOTER);
    s.replace("{t}", FPSTR(INTL_BACK_TO_HOME));
    return s;
}
