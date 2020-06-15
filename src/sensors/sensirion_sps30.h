//
// Created by viciu on 08.06.2020.
//

#ifndef NAMF_SENSIRION_SPS30_H
#define NAMF_SENSIRION_SPS30_H

#include "variables.h"
#include "system/scheduler.h"
#include "helpers.h"
#include "sps30_i2c.h"

namespace SPS30 {
    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    struct sps30_measurement m;
    uint16_t data_ready;
    char serial[SPS_MAX_SERIAL_LEN];

    unsigned long init() {
        debug_out("************** SPS30 init", DEBUG_MIN_INFO, true);
        Serial.println("Starting SPS30");
        sps30_reset();
        delay(200);
        if ( sps30_get_serial(serial) != 0 ) {
            Serial.println("Error getting SPS serial");
            return 0;
        }

//        sensirion_i2c_init();
//        while ((ret = sps30_probe()) != 0) {
//            Serial.print("SPS sensor probing failed\n");
//            Serial.println(ret);
//            delay(500);
//        }

//        ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
//        if (ret) {
//            Serial.print("error setting the auto-clean interval: ");
//            Serial.println(ret);
//        }
        ret = sps30_start_measurement();
        if (ret < 0) {
            Serial.print("error starting measurement\n");
        }

        return 10 * 1000;
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                init();
                break;
            case SimpleScheduler::RUN:
                ret = sps30_read_measurement(&m);
                if (ret < 0) {
                    Serial.print("error reading measurement\n");
                } else {
                    Serial.print("PM  1.0: ");
                    Serial.println(m.mc_1p0);
                    Serial.print("PM  2.5: ");
                    Serial.println(m.mc_2p5);
                    Serial.print("PM  4.0: ");
                    Serial.println(m.mc_4p0);
                    Serial.print("PM 10.0: ");
                    Serial.println(m.mc_10p0);
#ifndef SPS30_LIMITED_I2C_BUFFER_SIZE
                    Serial.print("NC  0.5: ");
                    Serial.println(m.nc_0p5);
                    Serial.print("NC  1.0: ");
                    Serial.println(m.nc_1p0);
                    Serial.print("NC  2.5: ");
                    Serial.println(m.nc_2p5);
                    Serial.print("NC  4.0: ");
                    Serial.println(m.nc_4p0);
                    Serial.print("NC 10.0: ");
                    Serial.println(m.nc_10p0);

//                        Serial.print("Typical particle size: ");
//                        Serial.println(m.typical_particle_size);
#endif
                    Serial.println();

                }
                return 30 * 1000;
                break;
        }
        debug_out("************** SPS30 process", DEBUG_MIN_INFO, true);
        return 15 * 1000;
    }

    String results(void) {
        String s("\"SPS30\":{\"PM10\":0.20}");
        return s;
    }

}

#endif //NAMF_SENSIRION_SPS30_H
