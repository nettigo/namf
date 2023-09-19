//
// Created by viciu on 9/19/23.
//

#ifndef NAMF_WIFI_H
#define NAMF_WIFI_H
#include <Arduino.h>
#include "variables.h"
#include "defines.h"
#include "helpers.h"

void wifiConfig();
void waitForWifiToConnect(int maxRetries);
void connectWifi();
void startAP();
#endif //NAMF_WIFI_H
