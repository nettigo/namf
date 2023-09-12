//
// Created by viciu on 9/12/23.
//

#ifndef NAMF_ESP32_FACTORY_RESET_H
#define NAMF_ESP32_FACTORY_RESET_H

#ifdef ESP_IDF_VERSION_MAJOR  // IDF 4+
#if CONFIG_IDF_TARGET_ESP32   // ESP32/PICO-D4
#include "esp32/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3

#include "esp32s3/rom/rtc.h"

#else
#error Target CONFIG_IDF_TARGET is not supported
#endif
#else  // ESP32 Before IDF 4.0
#include "rom/rtc.h"
#endif

#include <esp_task_wdt.h>

// On ESP32 both reset and power on result in the same reaset reason
//clear Factory Reset markers
void clearFactoryResetMarkers() {
    debug_out(F("Clear factory reset markers"), DEBUG_ERROR);
    if (SPIFFS.exists("/fr2")) SPIFFS.remove("/fr2");
    if (SPIFFS.exists("/fr1")) SPIFFS.remove("/fr1");
    if (SPIFFS.exists("/fr0")) SPIFFS.remove("/fr0");
}

void checkFactoryReset() {
    debug_out(F("mounting FS: "), DEBUG_MIN_INFO, 0);
    if (!SPIFFS.begin()) {
        SPIFFS.format();
        debug_out(F("\nFilesystem formatted, restart!"), DEBUG_ERROR);
        ESP.restart();

        debug_out(F("\n\n***************** Error mounting FS! *****************\n\n"), DEBUG_ERROR);
        return;
    }
    debug_out(F("OK"), DEBUG_MIN_INFO);
    if (1 != rtc_get_reset_reason(1)) {
        //clean up, no RST, no factory reset
        debug_out(F("No factory reset condition"), DEBUG_MED_INFO);
        clearFactoryResetMarkers();
        return;
    }
    debug_out(F("Checking factory reset condition"), DEBUG_MED_INFO);

    if (SPIFFS.exists("/fr2")) {
        //do factory reset
        debug_out(F("\n\n*************** FACTORY RESET! ****************\n\n"), DEBUG_ERROR);
        if (SPIFFS.exists("/config.json"))
            SPIFFS.remove("/config.json");
        clearFactoryResetMarkers();
        delay(1000);
        ESP.restart();
    } else {
        if (SPIFFS.exists(("/fr1"))) {
            debug_out(F("\nFACTORY RESET prepared, press reset once more for Factory Reset to defaults!"), DEBUG_ERROR);
            File f = SPIFFS.open("/fr2", "w");
            f.close();
            SPIFFS.remove("/fr1");
        } else {
            if (SPIFFS.exists(("/fr0"))) {
                debug_out(F("\nFACTORY RESET start - press reset two times"), DEBUG_ERROR);
                File f = SPIFFS.open("/fr1", "w");
                f.close();
                SPIFFS.remove("/fr0");
            } else {
                File f = SPIFFS.open("/fr0", "w");
                f.close();
            }
        }
    }
}

#endif //NAMF_ESP32_FACTORY_RESET_H
