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
    t_httpUpdate_return ret = ESPhttpUpdate.update(UPDATE_HOST, UPDATE_PORT, UPDATE_URL, ver);
    return ret;
};

#endif

void updateFW() {
    Serial.print("Check for update with ");
    unsigned int size = 0;
    String temp = String(SOFTWARE_VERSION);
//    temp += " ";
//    temp += INTL_LANG;
    size = temp.length()+1;
    char *buf;
    buf= (char *)malloc((size+1)*sizeof(char));
    temp.toCharArray(buf,size);
    Serial.println(buf);

    t_httpUpdate_return ret = tryUpdate(buf);
    switch (ret) {
        case HTTP_UPDATE_FAILED:
            Serial.println("[update] Update failed.");
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("[update] Update no Update.");
            Serial.print("Still running version: ");
            Serial.println(SOFTWARE_VERSION);
            break;
        case HTTP_UPDATE_OK:
            Serial.println("[update] Update ok."); // may not called we reboot the ESP
            break;
    }

}

#endif //NAMF_UPDATE_H
