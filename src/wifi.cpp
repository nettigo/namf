//
// Created by viciu on 9/19/23.
//

#include "wifi.h"
#include "webserver.h"
#include <DNSServer.h>

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
        uint8_t* BSSID;
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
    //When in AP mode it will stay until configured via WWW - so restarted
    setup_webserver();

    DNSServer dnsServer;
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);							// 53 is port for DNS server

    // 10 minutes timeout for wifi config
    last_page_load = millis();
    while (((millis() - last_page_load) < cfg::time_for_wifi_config)) {
        dnsServer.processNextRequest();
        server.handleClient();
#ifdef ARDUINO_ARCH_ESP8266
        wdt_reset(); // nodemcu is alive
#endif
        yield();
    }

    // half second to answer last requests
    last_page_load = millis();
    while ((millis() - last_page_load) < 500) {
        dnsServer.processNextRequest();
        server.handleClient();
        yield();
    }

    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    delete []wifiInfo;

    dnsServer.stop();

    delay(100);

    debug_out(F("Connecting to "), DEBUG_MIN_INFO, 0);
    debug_out(cfg::wlanssid, DEBUG_MIN_INFO, 1);

    // we could set hostname also here
    WiFi.begin(cfg::wlanssid, cfg::wlanpwd);

    debug_out(F("---- Result Webconfig ----"), DEBUG_MIN_INFO, 1);
    debug_out(F("WLANSSID: "), DEBUG_MIN_INFO, 0);
    debug_out(cfg::wlanssid, DEBUG_MIN_INFO, 1);
    debug_out(F("----\nReading ..."), DEBUG_MIN_INFO, 1);
    debug_out(F("SDS: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::sds_read), DEBUG_MIN_INFO, 1);
    debug_out(F("PMS: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::pms_read), DEBUG_MIN_INFO, 1);
    debug_out(F("DHT: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::dht_read), DEBUG_MIN_INFO, 1);
    debug_out(F("DS18B20: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::ds18b20_read), DEBUG_MIN_INFO, 1);
    debug_out(F("----\nSend to ..."), DEBUG_MIN_INFO, 1);
    debug_out(F("Dusti: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::send2dusti), DEBUG_MIN_INFO, 1);
    debug_out(F("Madavi: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::send2madavi), DEBUG_MIN_INFO, 1);
    debug_out(F("CSV: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::send2csv), DEBUG_MIN_INFO, 1);
    debug_out("----", DEBUG_MIN_INFO, 1);
    debug_out(F("Autoupdate: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::auto_update), DEBUG_MIN_INFO, 1);
    debug_out(F("Display: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::has_display), DEBUG_MIN_INFO, 1);
    debug_out(F("LCD 1602: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::has_lcd1602), DEBUG_MIN_INFO, 1);
    debug_out(F("Debug: "), DEBUG_MIN_INFO, 0);
    debug_out(String(cfg::debug), DEBUG_MIN_INFO, 1);
    wificonfig_loop = false;
}

void waitForWifiToConnect(int maxRetries) {
    int retryCount = 0;
    while ((WiFi.status() != WL_CONNECTED) && (retryCount <  maxRetries)) {
        delay(500);
        debug_out(".", DEBUG_MIN_INFO, 0);
        ++retryCount;
    }
}

/*****************************************************************
 * Start WiFi in AP mode (for configuration)
 *****************************************************************/

void startAP(void) {
    String fss = String(cfg::fs_ssid);
    display_debug(fss.substring(0, 16), fss.substring(16));
    wifiConfig();
    if (WiFi.status() != WL_CONNECTED) {
        waitForWifiToConnect(20);
        debug_out("", DEBUG_MIN_INFO, 1);
    }

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
        if (WiFi.status() != WL_CONNECTED)
            startAP();
    }
    debug_out(F("WiFi connected\nIP address: "), DEBUG_MIN_INFO, 0);
    debug_out(WiFi.localIP().toString(), DEBUG_MIN_INFO, 1);
}
