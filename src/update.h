//
// Created by viciu on 25.01.2020.
//

#ifndef NAMF_UPDATE_H
#define NAMF_UPDATE_H

#include "defines.h"
#include "variables.h"
#include "helpers.h"
#include "sensors/sds011/sds011.h"

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h> /// FOR ESP32
#include <HTTPClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <HTTPUpdate.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <WiFiClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
WiFiClient client;  /// FOR ESP32 HTTP FOTA UPDATE ///

t_httpUpdate_return tryUpdate(const String host, const String port, const String path, const String ver) {
    t_httpUpdate_return ret = httpUpdate.update(client, UPDATE_HOST, UPDATE_PORT, UPDATE_URL, ver);
//no OTA for now
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
    String host;
    String url;
    switch(cfg::update_channel) {
        case UPDATE_CHANNEL_ALFA:
            host = String(UPDATE_HOST_ALFA);
            url = String(UPDATE_URL_ALFA);
            break;
        case UPDATE_CHANNEL_BETA:
            host = String(UPDATE_HOST_BETA);
            url = String(UPDATE_URL_BETA);
            break;
        default:
            host = String(UPDATE_HOST);
            url = String(UPDATE_URL);
            break;
    }
    debug_out(F("Update checked:"), DEBUG_MIN_INFO,true);
    debug_out(host, DEBUG_MIN_INFO,true);
    debug_out(url, DEBUG_MIN_INFO,true);
    return tryUpdate(host,String(UPDATE_PORT), url, ver);
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


String sds_report() {
    String ret = F("");
    if (SDS011::enabled || cfg::sds_read) {
        ret += String(SDS011::failedReadings) + String(F("-")) + String(SDS011::readings);
//        SDS011::failedReadings = SDS011::readings = 0;
    } else {
        ret += F("na-na");
    }
    return ret;
}



void updateFW(const String host, const String port, const String path) {
    debug_out(F("Check for update with "),DEBUG_MIN_INFO,1);
    display_debug(F("Update - check"), F(""));
    debug_out(host,DEBUG_MIN_INFO,1);
    debug_out(port,DEBUG_MIN_INFO,1);
    debug_out(path,DEBUG_MIN_INFO,1);
    Serial.println(SOFTWARE_VERSION);
    String ver = String(SOFTWARE_VERSION)+ String(" ") + esp_chipid() + String(" ") + "SDS" + String(" ") +
                 String(cfg::current_lang) + String(" ") + String(FPSTR(INTL_LANG)) + sds_report();
    t_httpUpdate_return ret = tryUpdate( host, port, path, ver);
    verifyUpdate(ret);
};

void updateFW() {
    Serial.print(F("Check for update with default URL"));
    Serial.println(SOFTWARE_VERSION);
    display_debug(F("Update - check"), F(""));

    t_httpUpdate_return ret = tryUpdate(
            String(SOFTWARE_VERSION) + String(" ") + esp_chipid() + String(" ") + "SDS" + String(" ") +
            String(cfg::current_lang) + String(" ") + String(FPSTR(INTL_LANG)) + String(" ") + sds_report());
    verifyUpdate(ret);
};

#endif //NAMF_UPDATE_H
