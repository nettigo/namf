//
// Created by viciu on 9/19/23.
//

#ifndef NAMF_WIFI_H
#define NAMF_WIFI_H
#include <Arduino.h>
#include "variables.h"
#include "defines.h"
#include "helpers.h"
#ifdef ETHERNET
#include <ETH.h>
#include <WiFi.h>
#endif
namespace NAMNetwork {
        typedef enum {
            UNSET,
            AP_RUNNING,
            AP_CLOSING,
            CLIENT,
        } WiFiStatus;
    extern WiFiStatus wifi_state;

    typedef enum {
        CONNECTED,
        DISCONNECTED,
    } EthStatus;
    extern EthStatus eth_state;

    inline bool internet() { return (eth_state == CONNECTED || wifi_state == CLIENT); }

    //scan WiFi list
    void rescanWiFi();

    void wifiConfig();

    void waitForWifiToConnect(int maxRetries);

    void connectWifi();

    void stopWifi();

    void restartWiFi();

    void startAP();

    void process();

    //is WiFi connected as client (internet possible)
    bool wifiClient();

    //try to reconnect if client was configured
    void tryToReconnect();

    //collect info about visible networks
    struct struct_wifiInfo* collectWiFiInfo(int&);

    //configure network currently two modes - wifi client or wifi AP
    void configNetwork();
}

#endif //NAMF_WIFI_H
