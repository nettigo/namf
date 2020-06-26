//
// Created by viciu on 26.06.2020.
//

#include "tester.h"

namespace NetworkWatchdog {
//    const char
    const char KEY[] PROGMEM = "NTW_WTD";
    IPAddress addr;

    JsonObject & parseHTTPRequest(void){
        String host;
        parseHTTP(F("host"),host);
        StaticJsonBuffer<256> jsonBuffer;
        JsonObject & ret = jsonBuffer.createObject();
        if( addr.isValid(host)) {
            addr.fromString(host);
            ret[F("ip")] = host;
        } else{
            ret[F("err")] = FPSTR(INTL_NTW_WTD_ERROR);
        }
        return ret;
    }

    String getConfigHTML(void){
        String ret = F("<h1>Network Watchdog</h1>");
        ret += form_input(F("host"), FPSTR(INTL_NTW_WTD_HOST), String(""), 16);
        return ret;
    }
}
