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
