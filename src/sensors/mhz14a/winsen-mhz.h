//
// Created by viciu on 22.02.2020.
//
// Winsen MH-Z14A CO2 sensor

#ifndef NAMF_WINSEN_MHZ_H
#define NAMF_WINSEN_MHZ_H

#include <Arduino.h>
#include <stdbool.h>
#include "variables.h"
#include "helpers.h"
#include "webserver.h"

namespace MHZ14A {
    extern const char KEY[] PROGMEM;
    extern bool enabled;
    extern bool printOnLCD;
    typedef enum {
        RANGE_2K,
        RANGE_10K
    } range_t;

    JsonObject &parseHTTPRequest();

    //send data to LD API...
    extern void sendToLD();

    void getResults (String &);

    void resultsAsHTML (String &);

    void readConfigJSON(JsonObject &);

    String getConfigJSON(void);

    bool display(LiquidCrystal_I2C *lcd, byte minor);

    bool getDisplaySetting();

    unsigned long process(SimpleScheduler::LoopEventType);

    bool process_rx(uint8_t b, uint8_t cmd, uint8_t data[]);

    int prepare_tx(uint8_t cmd, const uint8_t *data, uint8_t buffer[], int size);

    bool exchange_command(SoftwareSerial &, uint8_t cmd, uint8_t data[], unsigned int timeout);

    bool set_range(SoftwareSerial &, range_t r);

    bool read_temp_co2(SoftwareSerial &, unsigned int *co2, unsigned int *temp);

    void setupWinsenMHZ(SoftwareSerial &);

    void readWinsenMHZ(SoftwareSerial &);

    void afterSend(bool);

    String sensorMHZ();
}
#endif //NAMF_WINSEN_MHZ_H
