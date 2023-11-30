//
// Created by viciu on 9/19/23.
//

#ifndef NAMF_WIFI_H
#define NAMF_WIFI_H
#include <Arduino.h>
#include "variables.h"
#include "defines.h"
#include "helpers.h"
namespace NAMWiFi {
        typedef enum {
            UNSET,
            AP_RUNNING,
            AP_CLOSING,
            CLIENT,
        } WiFiStatus;
    extern WiFiStatus state;

    void wifiConfig();

    void waitForWifiToConnect(int maxRetries);

    void connectWifi();

    void startAP();

    void process();

    //try to reconnect if client was configured
    void tryToReconnect();

    //collect info about visible networks
    struct struct_wifiInfo* collectWiFiInfo(int&);
}
//configure network currently two modes - wifi client or wifi AP
void configNetwork();

#endif //NAMF_WIFI_H
