#include "bmpX80.h"


namespace BMPx80 {
    const char KEY[] PROGMEM = "BMPx80";
    bool enabled = false;
    bool printOnLCD = false;

    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::BMPx80);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::BMPx80);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        return ret;
    };
}
