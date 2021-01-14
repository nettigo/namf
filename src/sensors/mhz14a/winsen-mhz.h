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

namespace SHT3x {
    const char KEY[] PROGMEM = "SHT3x";
    bool enabled = false;
typedef enum {
    RANGE_2K,
    RANGE_10K
} range_t;

bool process_rx(uint8_t b, uint8_t cmd, uint8_t data[]);
int prepare_tx(uint8_t cmd, const uint8_t *data, uint8_t buffer[], int size);
static bool exchange_command(SoftwareSerial &,uint8_t cmd, uint8_t data[], unsigned int timeout);
static bool set_range(SoftwareSerial &, range_t r);
static bool read_temp_co2(SoftwareSerial&,unsigned int *co2, unsigned int *temp);
void setupWinsenMHZ(SoftwareSerial&);
void readWinsenMHZ(SoftwareSerial&);
String sensorMHZ();
}
#endif //NAMF_WINSEN_MHZ_H
