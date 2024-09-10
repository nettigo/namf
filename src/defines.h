//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_DEFINES_H
#define NAMF_DEFINES_H

#include <stdint.h>
#include <Arduino.h>
/******************************************************************
 * Constants                                                      *
 ******************************************************************/
const unsigned long SAMPLETIME_SDS_MS = 1000;
const unsigned long WARMUPTIME_SDS_MS = 15000;
const unsigned long READINGTIME_SDS_MS = 10000;
const unsigned long SAMPLETIME_GPS_MS = 50;
const unsigned long DISPLAY_UPDATE_INTERVAL_MS = 5000;
const unsigned long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
extern unsigned long PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS;        // interval for check for firmware update
const unsigned long DURATION_BEFORE_FORCED_RESTART_MS = ONE_DAY_IN_MS * 28;  // force a reboot every ~4 weeks
#define SDS_SERIAL_BUFF_SIZE    36

typedef struct memory_stat_t {
    uint32_t freeHeap;
    uint16_t maxFreeBlock;
    uint8_t frag;
    uint32_t freeContStack;
} memory_stat_t;

extern String tmpl(const String& patt, const String& value1, const String& value2, const String& value3);
extern String tmpl(const String& patt, const String& value1, const String& value2);
extern String tmpl(const String& patt, const String& value);

#define UPDATE_CHANNEL_ALFA 0
#define UPDATE_CHANNEL_BETA 1
#define UPDATE_CHANNEL_STABLE 2

#ifdef ARDUINO_ARCH_ESP8266
#define PROCESSOR_ARCH  "esp8266"
#else
#define PROCESSOR_ARCH  "esp32"
#endif

#define HOST_MADAVI "api-rrd.madavi.de"
#define URL_MADAVI "/data.php"
#define PORT_MADAVI 80

#define HOST_DUSTI "api.sensor.community"
#define URL_DUSTI "/v1/push-sensor-data/"
#define PORT_DUSTI 80

#define HOST_SENSEMAP "ingress.opensensemap.org"
#define URL_SENSEMAP "/boxes/BOXID/data?luftdaten=1"
#define PORT_SENSEMAP 443

#define HOST_FSAPP "www.h2801469.stratoserver.net"
#define URL_FSAPP "/data.php"
#define PORT_FSAPP 80

#define UPDATE_HOST F("fw.nettigo.pl")
#ifdef ARDUINO_ARCH_ESP8266
#define UPDATE_URL F("/NAMF/index.php")
#define UPDATE_URL_ALFA F("/NAMF/index.php")
#define UPDATE_URL_BETA F("/NAMF/index.php")
#elif defined(ARDUINO_ARCH_ESP32)
#define UPDATE_URL F("/NAMF32/index.php")
#define UPDATE_URL_ALFA F("/NAMF32/index.php")
#define UPDATE_URL_BETA F("/NAMF32/index.php")
#endif
#define UPDATE_HOST_ALFA F("alfa.fw.nettigo.pl")
#define UPDATE_HOST_BETA F("beta.fw.nettigo.pl")
#define UPDATE_PORT 80

#define JSON_BUFFER_SIZE 2600

#define msSince(timestamp_before) (act_milli - (timestamp_before))


#endif //NAMF_DEFINES_H
