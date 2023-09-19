//
// Created by viciu on 9/19/23.
//

#include "wifi.h"
#include "webserver.h"
#include <DNSServer.h>

namespace NAMWiFi {
    typedef enum {
        UNSET,
        AP_RUNNING,
        AP_CLOSING,
        CLIENT,
    } WiFiStatus;

    WiFiStatus state = UNSET;

    DNSServer *dnsServer = nullptr;

    static int selectChannelForAp(struct struct_wifiInfo *info, int count) {
        std::array<int, 14> channels_rssi;
        std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

        for (int i = 0; i < count; i++) {
            if (info[i].RSSI > channels_rssi[info[i].channel]) {
                channels_rssi[info[i].channel] = info[i].RSSI;
            }
        }

        if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11])) {
            return 1;
        } else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11])) {
            return 6;
        } else {
            return 11;
        }
    }


/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/

    void wifiConfig() {
        debug_out(F("Starting WiFiManager"), DEBUG_MIN_INFO, 1);
        debug_out(F("AP ID: "), DEBUG_MIN_INFO, 0);
        debug_out(cfg::fs_ssid, DEBUG_MIN_INFO, 1);
        debug_out(F("Password: "), DEBUG_MIN_INFO, 0);
        debug_out(cfg::fs_pwd, DEBUG_MIN_INFO, 1);

        wificonfig_loop = true;

        WiFi.disconnect(true);
        debug_out(F("scan for wifi networks..."), DEBUG_MIN_INFO, 1);
        count_wifiInfo = WiFi.scanNetworks(false, true);
        wifiInfo = new struct_wifiInfo[count_wifiInfo];
        for (int i = 0; i < count_wifiInfo; i++) {
            uint8_t * BSSID;
            String SSID;
#if defined(ARDUINO_ARCH_ESP8266)
            WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType, wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel, wifiInfo[i].isHidden);
#else
            //esp32
            WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType, wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel);
#endif

            SSID.toCharArray(wifiInfo[i].ssid, 35);
        }

        WiFi.mode(WIFI_AP);
        const IPAddress apIP(192, 168, 4, 1);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        if (cfg::fs_pwd == nullptr || !strcmp(cfg::fs_pwd, "")) {
            debug_out(F("Starting AP with default password"), DEBUG_MIN_INFO);
            WiFi.softAP(cfg::fs_ssid, "nettigo.pl", selectChannelForAp(wifiInfo, count_wifiInfo));
        } else {
            WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp(wifiInfo, count_wifiInfo));
        }
        if (dnsServer == nullptr)
            dnsServer = new(DNSServer);
        dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer->start(53, "*", apIP);                            // 53 is port for DNS server

        // 10 minutes timeout for wifi config
        last_page_load = millis();

    }

    void waitForWifiToConnect(int maxRetries) {
        int retryCount = 0;
        while ((WiFi.status() != WL_CONNECTED) && (retryCount < maxRetries)) {
            delay(500);
            debug_out(".", DEBUG_MIN_INFO, 0);
            ++retryCount;
        }
    }

    void process() {
        if (state == AP_RUNNING) {
            if (cfg::time_for_wifi_config == 0 || (millis() - last_page_load) < cfg::time_for_wifi_config) {
                dnsServer->processNextRequest();
                server.handleClient();
#ifdef ARDUINO_ARCH_ESP8266
                wdt_reset(); // nodemcu is alive
#endif
                yield();
            } else { //AP timeout
                state = AP_CLOSING;
                last_page_load = millis();
            }


        }
        if (state == AP_CLOSING) {
            if ((millis() - last_page_load) < 500) {
                dnsServer->processNextRequest();
                server.handleClient();
                yield();
            }
            WiFi.disconnect(true);
            WiFi.softAPdisconnect(true);
            state = UNSET;

            delete[]wifiInfo;

            dnsServer->stop();
            delete dnsServer;
            dnsServer = nullptr;
        }


    }

/*****************************************************************
 * Start WiFi in AP mode (for configuration)
 *****************************************************************/

    void startAP(void) {
        String fss = String(cfg::fs_ssid);
        display_debug(fss.substring(0, 16), fss.substring(16));
        wifiConfig();
        state = AP_RUNNING;

    }

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/
    void connectWifi() {
        display_debug(F("Connecting to"), String(cfg::wlanssid));
        debug_out(F("SSID: '"), DEBUG_ERROR, 0);
        debug_out(cfg::wlanssid, DEBUG_ERROR, 0);
        debug_out(F("'"), DEBUG_ERROR, 1);
        debug_out(String(WiFi.status()), DEBUG_MIN_INFO, 1);
        WiFi.disconnect();
#if defined(ARDUINO_ARCH_ESP8266)
        WiFi.setOutputPower(cfg::outputPower);
        WiFi.setPhyMode(WIFI_PHY_MODE_11N);
        WiFi.setPhyMode((WiFiPhyMode_t)cfg::phyMode);
#endif
        WiFi.mode(WIFI_STA);

        //String hostname = F("NAM-");
        //hostname += esp_chipid();
        //WiFi.hostname(hostname.c_str()); // Hostname for DHCP Server.

        WiFi.hostname(cfg::fs_ssid); // Hostname for DHCP Server
        WiFi.begin(cfg::wlanssid, cfg::wlanpwd); // Start WiFI

        waitForWifiToConnect(40);
        debug_out("", DEBUG_MIN_INFO, 1);
        if (WiFi.status() != WL_CONNECTED) {
            if (strlen(cfg::fbssid)) {
                display_debug(F("Connecting to"), String(cfg::fbssid));
                debug_out(F("Failed to connect to WiFi. Trying to connect to fallback WiFi"), DEBUG_ERROR);
                debug_out(cfg::fbssid, DEBUG_ERROR);
                WiFi.begin(cfg::fbssid, cfg::fbpwd); // Start WiFI
                waitForWifiToConnect(40);

            }
            if (WiFi.status() != WL_CONNECTED) {
                startAP();
                cfg::internet = false;
            } else {
                cfg::internet = true;
                state = CLIENT;
            }
        }
        debug_out(F("WiFi connected\nIP address: "), DEBUG_MIN_INFO, 0);
        debug_out(WiFi.localIP().toString(), DEBUG_MIN_INFO, 1);
    }

    void time_is_set(void) {
        sntp_time_is_set = true;
    }

    static bool acquireNetworkTime() {
        int retryCount = 0;
        debug_out(F("Setting time using SNTP"), DEBUG_MIN_INFO, 1);
        time_t now = time(nullptr);
        debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
        debug_out(NTP_SERVER, DEBUG_MIN_INFO, 1);
#ifdef ARDUINO_ARCH_ESP8266
        settimeofday_cb(time_is_set);
        configTime(0, 3600, NTP_SERVER);
#elif defined(ARDUINO_ARCH_ESP32)
        configTime(0, 3600, NTP_SERVER);
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
            time_is_set();
#endif
        while (retryCount++ < 20) {
            // later than 2000/01/01:00:00:00
            if (sntp_time_is_set) {
                now = time(nullptr);
                debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
                return true;
            }
            delay(500);
            debug_out(".", DEBUG_MIN_INFO, 0);
        }
        debug_out(F("\nrouter/gateway:"), DEBUG_MIN_INFO, 1);
        retryCount = 0;
        configTime(0, 0, WiFi.gatewayIP().toString().c_str());
        while (retryCount++ < 20) {
            // later than 2000/01/01:00:00:00
            if (sntp_time_is_set) {
                now = time(nullptr);
                debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
                return true;
            }
            delay(500);
            debug_out(".", DEBUG_MIN_INFO, 0);
        }
        return false;
    }

    void tryToReconnect() {
        if (state == CLIENT && WiFi.status() != WL_CONNECTED) {
            debug_out(F("Connection lost, reconnecting "), DEBUG_MIN_INFO, 0);
            WiFi.reconnect();
            NAMWiFi::waitForWifiToConnect(20);
            debug_out("", DEBUG_MIN_INFO, 1);
            if (WiFi.status() != WL_CONNECTED) {    //still no connection
                debug_out(F("Still no WiFi, turn off..."), DEBUG_MIN_INFO);
                WiFi.mode(WIFI_OFF);
                delay(2000);
                debug_out(F("WiFi, reconnecting"), DEBUG_MIN_INFO);
                WiFi.mode(WIFI_STA);
                WiFi.begin(cfg::wlanssid, cfg::wlanpwd); // Start WiFI
                NAMWiFi::waitForWifiToConnect(20);
            }
        }

    }
}

//config network
void configNetwork() {
    if (strlen(cfg::wlanssid) > 0) {
        NAMWiFi::connectWifi();
        got_ntp = NAMWiFi::acquireNetworkTime();
        debug_out(F("NTP time "), DEBUG_MIN_INFO, 0);
        debug_out(String(got_ntp ? "" : "not ") + F("received"), DEBUG_MIN_INFO, 1);
        if (cfg::auto_update) {
            updateFW();
        }

    } else {
        cfg::internet = false;
        NAMWiFi::startAP();
    }

}
