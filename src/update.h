//
// Created by viciu on 25.01.2020.
//

#ifndef NAMF_UPDATE_H
#define NAMF_UPDATE_H

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
t_httpUpdate_return tryUpdate(char *const ver) {
    Serial.println(ver);
    t_httpUpdate_return ret = ESPhttpUpdate.update(UPDATE_HOST, UPDATE_PORT, UPDATE_URL, ver);
    return ret;
};



t_httpUpdate_return tryUpdate(String const ver) {
    Serial.print(ver);
    Serial.println(F("."));
    char *buf;
    unsigned size = ver.length();
    buf= (char *)malloc((size+2)*sizeof(char));
    ver.toCharArray(buf,size+1);
    return tryUpdate(buf);
};

#endif

void updateFW() {
    Serial.print(F("Check for update with "));
    Serial.println(SOFTWARE_VERSION);

    t_httpUpdate_return ret = tryUpdate(String(SOFTWARE_VERSION)+ String(" ") + esp_chipid + String(" ") + "SDS" + String(" ") +
                                        String(cfg::current_lang) + String(" ") + String(INTL_LANG) );
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.println(F("[update] Update failed."));
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println(F("[update] no Update."));
            Serial.print(F("Still running version: "));
            Serial.println(SOFTWARE_VERSION);
            break;
        case HTTP_UPDATE_OK:
            Serial.println(F("[update] Update ok.")); // may not called we reboot the ESP
            break;
    }

}

#endif //NAMF_UPDATE_H
