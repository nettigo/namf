//
// Created by viciu on 26.06.2020.
//

#include "tester.h"
#define NTW_WTD_COUNT   10
namespace NetworkWatchdog {
//    const char

    const char KEY[] PROGMEM = "NTW_WTD";
    bool enabled = false;
    bool configured = false;
    bool finishedTest = false;
    unsigned long lastCheck = 0;
    AsyncPingResponse lastCheckResult;
    byte responseCount;
    byte timeoutCount;
    IPAddress addr;
    AsyncPing pinger;

    typedef enum {
        IDLE,
        PING,   //doing regular ping
        LOST_CONNECTION_1,
        SECOND_CHECK,
        LOST_CONNECTION_2,
        THIRD_CHECK
    } WatchdogState;

    WatchdogState currentState = IDLE;

    JsonObject &parseHTTPRequest(void) {
        String host;
        parseHTTP(F("host"), host);
        String sensorID = F("enabled-{s}");
        sensorID.replace(F("{s}"),String(SimpleScheduler::NTW_WTD));
        parseHTTP(sensorID, enabled);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        if (addr.isValid(host)) {
            addr.fromString(host);
            ret[F("ip")] = host;
        } else {
            ret[F("err")] = FPSTR(INTL_NTW_WTD_ERROR);
        }
        ret[F("e")] = enabled;
        return ret;
    }

    String getConfigJSON(void) {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        ret += Var2Json(F("ip"), (addr.toString()));
        return ret;
    }

    void readConfigJSON(JsonObject &json) {
        String ip;
        enabled = json.get<bool>(F("e"));
        ip = json.get<String>(F("ip"));
        if (addr.isValid(ip)) {
            addr.fromString(ip);
            configured = true;
        } else {
            configured = false;
        }
        if (enabled && !scheduler.isRegistered(SimpleScheduler::NTW_WTD)) {
            scheduler.registerSensor(SimpleScheduler::NTW_WTD, NetworkWatchdog::process, FPSTR(KEY));
            scheduler.init(SimpleScheduler::NTW_WTD);

        } else if (scheduler.isRegistered(SimpleScheduler::NTW_WTD)) {
            scheduler.unregisterSensor(SimpleScheduler::NTW_WTD);
        }
    }

    void resultsAsHTML(String &page_content) {
        if (!enabled) { return; }

        page_content += FPSTR(EMPTY_ROW);
        if (!configured) {
            page_content += table_row_from_value(FPSTR("NTW WTD"), "Wrong config", "", "");
            return;
        }
        if (lastCheck == 0) {
            page_content += table_row_from_value(FPSTR("NTW WTD"), "No test yet", "", "");
            return;
        }

        page_content += table_row_from_value(FPSTR("NTW WTD"), "Status code", String(currentState), "");
        page_content += table_row_from_value(FPSTR("NTW WTD"), "Last check", String((millis() - lastCheck) / 1000),
                                             "sec");
        page_content += table_row_from_value(FPSTR("NTW WTD"), "Pings", String(lastCheckResult.total_sent), "");
        page_content += table_row_from_value(FPSTR("NTW WTD"), "Responses", String(lastCheckResult.total_recv), "");

    }

    bool success(AsyncPingResponse r = lastCheckResult) {
        if (r.total_recv > NTW_WTD_COUNT / 2) {
            return true;
        }
        return false;
    }

    //which state
    WatchdogState nextPingState() {
        WatchdogState ret;

        switch (currentState) {
            case IDLE:
                return PING;
            case PING:
                if (finishedTest && success())
                    ret = IDLE;
                else
                    ret = LOST_CONNECTION_1;
                if (finishedTest) finishedTest = false;
                return ret;
            case SECOND_CHECK:
                if (finishedTest && success())
                    ret = IDLE;
                else
                    ret = LOST_CONNECTION_2;
                if (finishedTest) finishedTest = false;
                return ret;
            case THIRD_CHECK:
                if (finishedTest && success()) {
                    finishedTest = false;
                    return IDLE;
                }
                debug_out(F("Third fail with PING. WiFi lost? Restarting"), DEBUG_MIN_INFO, true);
                delay(500);
                ESP.restart();
            case LOST_CONNECTION_1:
                return SECOND_CHECK;
            case LOST_CONNECTION_2:
                return THIRD_CHECK;
            default:
                return currentState;
        }
    }

    void startPing(void) {
        timeoutCount = 0;
        responseCount = 0;
        finishedTest = false;
        lastCheck = millis();
        //on response
//        pinger.on(true, [](const AsyncPingResponse &response) {
//            return false;   //carry on
//        });
        //on end
        pinger.on(false, [](const AsyncPingResponse &response) {
            //IPAddress addr(response.addr); //to prevent with no const toString() in 2.3.0
            lastCheckResult = response;
            finishedTest = true;
//            currentState = nextPingState();

            return false;
        });

        if (configured) {
            pinger.begin(addr, NTW_WTD_COUNT);
        } else {
            currentState = IDLE;
        }
    }

    unsigned long nextCallTime() {
        switch (currentState) {
            case IDLE:
                return 600 * 1000;
            case PING:
            case LOST_CONNECTION_1:
            case LOST_CONNECTION_2:
                return (NTW_WTD_COUNT + 2) * 1000;
            case SECOND_CHECK:
            case THIRD_CHECK:
                return 60 * 1000;
        }
        return 0;   //disable WTD
    }

    unsigned long processState(void) {
        debug_out(F("PIIIING processor"), DEBUG_MIN_INFO, true);
        switch (currentState) {
            case IDLE:
                debug_out(F("PING connectivity check:"), DEBUG_MIN_INFO, true);
                startPing();
                break;
            case LOST_CONNECTION_1:
                debug_out(F("Failed first PING check, running second"), DEBUG_MIN_INFO, true);
                startPing();
                break;
            case LOST_CONNECTION_2:
                debug_out(F("Failed second PING check, running third"), DEBUG_MIN_INFO, true);
                startPing();
                break;
            default:
                debug_out(F("PING connectivity check state:"), DEBUG_MIN_INFO, false);
                debug_out(String(currentState), DEBUG_MIN_INFO, true);
                break;

        }
        currentState = nextPingState();
        debug_out (F("Next ping state: "), DEBUG_MIN_INFO,false);
        debug_out (String(currentState), DEBUG_MIN_INFO,true);
        return nextCallTime();
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        debug_out(F("Network wtchd 'process'"), DEBUG_MIN_INFO, true);
        if (!enabled || !configured) {
            return 0;
        }
        switch (e) {
            case SimpleScheduler::INIT:
                currentState = IDLE;
                return 60 * 1000;
            case SimpleScheduler::RUN:
                return processState();
            default:
                return 0;
        }

    }

    String getConfigHTML(void) {
        String ret = F("");
        ret += form_input(F("host"), FPSTR(INTL_NTW_WTD_HOST), addr.toString(), 16);
        return ret;
    }
}
