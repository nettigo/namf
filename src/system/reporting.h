//
// Created by viciu on 09.10.2021.
//

#ifndef NAMF_REPORTING_H
#define NAMF_REPORTING_H

#include "helpers.h"
#include "sensors/sds011.h"
#include <ESP8266WiFi.h>

namespace Reporting {
    const char reportingHost[] PROGMEM = "et.nettigo.pl";
    const unsigned reportingHostPort PROGMEM = 80;
    unsigned long lastPhone = 0;
    unsigned long lastPeriodicCheck = 0;
    bool first = true;
    int8_t minRSSI = 255;
    int8_t maxRSSI = 0;
#define RPRT_PERIODIC_CHECK_INTERVAL    1000


//extern SerialSDS channelSDS(serialSDS);

    //we are using the same code in main loop, but in main namespace we are collecting stats for one
    //measurement period, here we are collecting for 24 hrs, and just those min/max are sending
    memory_stat_t memoryStatsMax;
    memory_stat_t memoryStatsMin;

    void resetMinMaxStats() {
        memoryStatsMax = memory_stat_t{0,0,0, 0};
        memoryStatsMin = memory_stat_t{UINT32_MAX, UINT16_MAX, UINT8_MAX, UINT32_MAX};
        maxRSSI = -128;
        minRSSI = 127;
    }

    void collectPeriodicStats(){
        int8_t rssi = WiFi.RSSI();
        if (rssi > maxRSSI) maxRSSI = rssi;
        if (rssi < minRSSI) minRSSI = rssi;
    }

    void collectMemStats() {
        memory_stat_t memoryStats;
        ESP.getHeapStats(&memoryStats.freeHeap, &memoryStats.maxFreeBlock, &memoryStats.frag);

        if (memoryStats.freeHeap > memoryStatsMax.freeHeap)             memoryStatsMax.freeHeap  = memoryStats.freeHeap;
        if (memoryStats.maxFreeBlock > memoryStatsMax.maxFreeBlock)     memoryStatsMax.maxFreeBlock  = memoryStats.maxFreeBlock;
        if (memoryStats.frag > memoryStatsMax.frag)                     memoryStatsMax.frag  = memoryStats.frag;

        if (memoryStats.freeHeap < memoryStatsMin.freeHeap)             memoryStatsMin.freeHeap  = memoryStats.freeHeap;
        if (memoryStats.maxFreeBlock < memoryStatsMin.maxFreeBlock)     memoryStatsMin.maxFreeBlock  = memoryStats.maxFreeBlock;
        if (memoryStats.frag < memoryStatsMin.frag)                     memoryStatsMin.frag  = memoryStats.frag;

        uint32_t cont = ESP.getFreeContStack();
        if (cont > memoryStatsMax.freeContStack)                     memoryStatsMax.freeContStack  = cont;
        if (cont < memoryStatsMin.freeContStack)                     memoryStatsMin.freeContStack  = cont;


    }

    void registerSensor() {
        WiFiClient client;
        HTTPClient http;
        String uri = F("/register/");
        uri.concat(esp_chipid());
        http.begin(client, reportingHost, reportingHostPort, uri ); //HTTP
//        Serial.println(F("Register"));
        http.addHeader(F("Content-Type"), F("application/json"));
        String body = F("");
        int httpCode = http.POST(body);
//        Serial.println(httpCode);
        String resp = http.getString();
//        Serial.println(resp);
        if (httpCode == HTTP_CODE_OK) {
            cfg::UUID = resp;
            writeConfig();
        }
    }

    void reportBoot() {
        if (!cfg::send_diag) return;
        debug_out(F("Report boot..."), DEBUG_MED_INFO);
        if (cfg::UUID.length() < 36) { registerSensor(); }
        if (cfg::UUID.length() < 36) { return; } //failed register
        WiFiClient client;
        HTTPClient http;

        String body = F("{");
        body.reserve(512);
        body.concat(Var2Json(F("VER"), SOFTWARE_VERSION));
        body.concat(Var2Json(F("MD5"), ESP.getSketchMD5()));
        body.concat(Var2Json(F("resetReason"), ESP.getResetReason()));
        body.concat(Var2Json(F("enabledSubsystems"), scheduler.registeredNames()));
        body.concat(Var2Json(F("updateChannel"), cfg::update_channel));
        body.concat(Var2Json(F("autoUpdate"), cfg::auto_update));
        body.remove(body.length() - 1);
        body.concat(F("}"));

        String uri = F("/store/");
        uri.concat(cfg::UUID);
        http.begin(client, reportingHost, reportingHostPort, uri ); //HTTP
        http.addHeader("Content-Type", "application/json");

//        Serial.println(body);
        int httpCode = http.POST(body);
//        Serial.println(httpCode);
        //server does not have such UUID. Database reset or something other, just re-register next time
        if (httpCode == HTTP_CODE_NOT_FOUND) {
            cfg::UUID = F("");
            writeConfig();
            //well, next time
        }


    }

    void setupHomePhone() {
        resetMinMaxStats();
    }

    //sending stats interval in hours
    unsigned sendingInterval() {
        switch (cfg::update_channel) {
            case UPDATE_CHANNEL_ALFA:
            case UPDATE_CHANNEL_BETA:
                return 8;
            case UPDATE_CHANNEL_STABLE:
                return 24;
            default:
                return 24;
        }
    }


    void homePhone() {
        if (!cfg::send_diag) return;
        collectMemStats();

        if (millis() - lastPeriodicCheck > RPRT_PERIODIC_CHECK_INTERVAL) {
            collectPeriodicStats();
            lastPeriodicCheck = millis();
        }

        if (
                (first && millis() - lastPhone > 15 * 60 * 1000) ||
                millis() - lastPhone > sendingInterval() * 60 * 60 *1000
        ) {
            first = false;
            WiFiClient client;
            HTTPClient http;

            String body = F("{");
            body.reserve(1024);
            if (SDS011::enabled) {
                body.concat(F("\"SDS\":{"));
                body.concat(Var2Json(F("failed"), String(SDS011::failedReadings)));
                body.concat(Var2Json(F("rd"), String(SDS011::readings)));
                body.concat(Var2Json(F("CRCerr"), String(SDS011::channelSDS.errorRate())));
                if (SDS011::hardwareWatchdog) {
                    body.concat(Var2Json(F("wtgCycles"), String(SDS011::hwWtdgCycles)));
                    body.concat(Var2Json(F("wtgErrs"), String(SDS011::hwWtdgErrors)));

                }
                SDS011::failedReadings = SDS011::readings = 0;
                body.remove(body.length() - 1);
                body.concat(F("},"));
            }

            body.concat(Var2Json(F("minMaxFreeBlock"),memoryStatsMin.maxFreeBlock));
            body.concat(Var2Json(F("maxMaxFreeBlock"),memoryStatsMax.maxFreeBlock));
            body.concat(Var2Json(F("minRSSI"),minRSSI));
            body.concat(Var2Json(F("maxRSSI"),maxRSSI));

            Reporting::resetMinMaxStats();

            body.remove(body.length() - 1);
            body.concat(F("}"));
            String uri = F("/store/");
            uri.concat(cfg::UUID);
            http.begin(client, reportingHost, reportingHostPort, uri ); //HTTP
            http.addHeader("Content-Type", "application/json");

            Serial.println(body);
            int httpCode = http.POST(body);
            Serial.println(httpCode);
            //server does not have such UUID. Database reset or something other, just re-register next time
            lastPhone = millis();
        }
    }

}
#endif //NAMF_REPORTING_H
