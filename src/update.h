//
// Created by viciu on 25.01.2020.
//

#ifndef NAMF_UPDATE_H
#define NAMF_UPDATE_H

#include "defines.h"
#include "variables.h"
#include "helpers.h"

#ifdef ESP32
#include <WiFi.h> /// FOR ESP32
#include <HTTPClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <HTTPUpdate.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <WiFiClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
WiFiClient client;  /// FOR ESP32 HTTP FOTA UPDATE ///

t_httpUpdate_return tryUpdate(char * const ver) {
    t_httpUpdate_return ret = httpUpdate.update(client, UPDATE_HOST, UPDATE_PORT, UPDATE_URL, ver);
    return ret;
}
#else

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include "helpers.h"
t_httpUpdate_return tryUpdate(const String host, const String port, const String path, const String ver) {
    WiFiClient client;
    Serial.println(ver);
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, host, port.toInt(), path, ver);
    return ret;
};
#endif

t_httpUpdate_return tryUpdate(String const ver) {
    return tryUpdate(String(UPDATE_HOST),String(UPDATE_PORT), String(UPDATE_URL), ver);
};

void verifyUpdate (t_httpUpdate_return result) {
    last_update_attempt = millis();
    switch (result) {
        case HTTP_UPDATE_FAILED:
            display_debug(F("[update] Update failed."),"");
            Serial.println(F("[update] Update failed."));
            break;
        case HTTP_UPDATE_NO_UPDATES:
            display_debug(F("[update] no Update."), String(SOFTWARE_VERSION));
            Serial.println(F("[update] no Update."));
            Serial.print(F("Still running version: "));
            Serial.println(SOFTWARE_VERSION);
            break;
        case HTTP_UPDATE_OK:
            Serial.println(F("[update] Update ok.")); // may not called we reboot the ESP
            break;
    }


}

void updateFW(const String host, const String port, const String path) {
    debug_out(F("Check for update with "),DEBUG_MIN_INFO,1);
    debug_out(host,DEBUG_MIN_INFO,1);
    debug_out(port,DEBUG_MIN_INFO,1);
    debug_out(path,DEBUG_MIN_INFO,1);
    Serial.println(SOFTWARE_VERSION);
    String ver = String(SOFTWARE_VERSION)+ String(" ") + esp_chipid() + String(" ") + "SDS" + String(" ") +
                 String(cfg::current_lang) + String(" ") + String(FPSTR(INTL_LANG));
    t_httpUpdate_return ret = tryUpdate( host, port, path, ver);
    verifyUpdate(ret);
};


void updateFW() {
    Serial.print(F("Check for update with default URL"));
    Serial.println(SOFTWARE_VERSION);

    t_httpUpdate_return ret = tryUpdate(String(SOFTWARE_VERSION)+ String(" ") + esp_chipid() + String(" ") + "SDS" + String(" ") +
                                        String(cfg::current_lang) + String(" ") + String(FPSTR(INTL_LANG)) );
    verifyUpdate(ret);
};

#endif //NAMF_UPDATE_H
