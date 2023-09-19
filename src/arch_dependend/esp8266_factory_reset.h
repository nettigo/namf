//
// Created by viciu on 9/12/23.
//

#ifndef NAMF_ESP8266_FACTORY_RESET_H
#define NAMF_ESP8266_FACTORY_RESET_H


//clear Factory Reset markers
void clearFactoryResetMarkers() {
    debug_out(F("Clear factory reset markers"), DEBUG_ERROR);
    if (SPIFFS.exists("/fr1")) SPIFFS.remove("/fr1");
    if (SPIFFS.exists("/fr")) SPIFFS.remove("/fr");
}


void checkFactoryReset() {
    debug_out(F("mounting FS: "), DEBUG_MIN_INFO, 0);
    if (!SPIFFS.begin()) {
#ifdef ARDUINO_ARCH_ESP32
        SPIFFS.format();
        debug_out(F("\nFilesystem formatted, restart!"), DEBUG_ERROR);
        ESP.restart();

#endif
        debug_out(F("\n\n***************** Error mounting FS! *****************\n\n"), DEBUG_ERROR);
        return;
    }
    debug_out(F("OK"), DEBUG_MIN_INFO);
#ifdef ARDUINO_ARCH_ESP32
    if ( 14 !=  rtc_get_reset_reason(1)) {
#elif defined(ARDUINO_ARCH_ESP8266)
        uint32 r = ESP.getResetInfoPtr()->reason;
    if (r !=  REASON_EXT_SYS_RST) {
#endif
        //clean up, no RST, no factory reset
        debug_out(F("No factory reset condition"), DEBUG_MED_INFO);
        clearFactoryResetMarkers();
        return;
    }
    debug_out(F("Checking factory reset condition"), DEBUG_MED_INFO);

    if (SPIFFS.exists("/fr1")) {
        //do factory reset
        debug_out(F("\n\n*************** FACTORY RESET! ****************\n\n"), DEBUG_ERROR);
        if (SPIFFS.exists("/config.json"))
            SPIFFS.remove("/config.json");
        clearFactoryResetMarkers();
        delay(1000);
        ESP.restart();
    } else {
        if(SPIFFS.exists(("/fr"))) {
            debug_out(F("\n\n********************* FACTORY RESET prepared, press reset once more for Factory Reset to defaults! *************\n\n"), DEBUG_ERROR);
            File f=SPIFFS.open("/fr1","w");
            f.close();
            SPIFFS.remove("/fr");
        } else {
            debug_out(F("\nFACTORY RESET start - press reset two times"), DEBUG_ERROR);
            File f=SPIFFS.open("/fr","w");
            f.close();
        }
    }

}

#endif //NAMF_ESP8266_FACTORY_RESET_H
