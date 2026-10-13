//
// Created by viciu on 25.01.2020.
//

#ifndef NAMF_UPDATE_H
#define NAMF_UPDATE_H

#include "defines.h"
#include "variables.h"
#include "helpers.h"
#include "sending.h"
#include "sensors/sds011/sds011.h"
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h> /// FOR ESP32
#include <HTTPClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <HTTPUpdate.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#include <WiFiClient.h> /// FOR ESP32 HTTP FOTA UPDATE ///
#else
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include "helpers.h"
#endif


void setupUpdate();
t_httpUpdate_return tryUpdate(const String& host, const String& path, const String& ver, const bool SSL_only);
t_httpUpdate_return tryUpdate(String const& ver, boolean SSL_only = false);
void updateFW();

#endif //NAMF_UPDATE_H