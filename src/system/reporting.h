//
// Created by viciu on 09.10.2021.
//

#ifndef NAMF_REPORTING_H
#define NAMF_REPORTING_H

#include "helpers.h"

namespace Reporting {
    const char reportingHost[] PROGMEM = "192.168.1.206";
    const unsigned reportingHostPort PROGMEM = 8080;

    void registerSensor() {
        WiFiClient client;
        HTTPClient http;
        String uri = F("/register/");
        uri.concat(esp_chipid());
        http.begin(client, reportingHost, reportingHostPort, uri ); //HTTP
        Serial.println(F("Register"));
        http.addHeader("Content-Type", "application/json");
        String body = F("");
        int httpCode = http.POST(body);
        Serial.println(httpCode);
        String resp = http.getString();
        Serial.println(resp);
        if (httpCode == HTTP_CODE_OK) {
            cfg::UUID = resp;
            writeConfig();
        }
    }

    void reportBoot() {
        Serial.println(F("Reprot boot"));

        debug_out(F("Report boot..."), DEBUG_MED_INFO);
        Serial.println(cfg::UUID);
        Serial.println(cfg::UUID.length());
        if (cfg::UUID.length() < 36) { registerSensor(); }
        if (cfg::UUID.length() < 36) { return; } //failed register
        WiFiClient client;
        HTTPClient http;

        String body = F("{");
        body.reserve(512);
        body.concat(Var2Json(F("VER"), SOFTWARE_VERSION));
        body.concat(Var2Json(F("MD5"), ESP.getSketchMD5()));
        body.concat(Var2Json(F("resetReason"), ESP.getResetReason()));
        body.remove(body.length() - 1);
        body.concat(F("}"));

        String uri = F("/store/");
        uri.concat(cfg::UUID);
        http.begin(client, reportingHost, reportingHostPort, uri ); //HTTP
        http.addHeader("Content-Type", "application/json");

        Serial.println(body);
        int httpCode = http.POST(body);
        Serial.println(httpCode);
        String resp = http.getString();
        Serial.println(resp);



    }

}
#endif //NAMF_REPORTING_H
