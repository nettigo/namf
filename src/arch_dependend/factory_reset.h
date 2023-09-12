//
// Created by viciu on 9/12/23.
//

#ifndef NAMF_FACTORY_RESET_H
#define NAMF_FACTORY_RESET_H
#ifdef ARDUINO_ARCH_ESP32
#include "esp32_factory_reset.h"
#elif defined(ARDUINO_ARCH_ESP8266)
#include "esp8266_factory_reset.h"
#endif

#endif //NAMF_FACTORY_RESET_H
