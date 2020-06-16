//
// Created by viciu on 08.06.2020.
//

#ifndef NAMF_SENSIRION_SPS30_H
#define NAMF_SENSIRION_SPS30_H

#include "variables.h"
#include "system/scheduler.h"
#include "helpers.h"
#include <sps30.h>

#define SPS_MAX_SERIAL_LEN 32

namespace SPS30 {
    bool started = false;

    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    struct sps30_measurement sum ;
    unsigned int measurement_count;
    char serial[SPS_MAX_SERIAL_LEN];

    void zeroMeasurmentStruct(sps30_measurement &str) {
        sps30_measurement zero = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        str = zero;
    }

    void addMeasurementStruct(sps30_measurement &storage, sps30_measurement reading){
        storage.mc_1p0 += reading.mc_1p0;
        storage.mc_2p5 += reading.mc_2p5;
        storage.mc_4p0 += reading.mc_4p0;
        storage.mc_10p0 += reading.mc_10p0;
        storage.nc_1p0 += reading.nc_1p0;
        storage.nc_2p5 += reading.nc_2p5;
        storage.nc_4p0 += reading.nc_4p0;
        storage.nc_10p0 += reading.nc_10p0;
        storage.typical_particle_size += reading.typical_particle_size;
    }

    unsigned long init() {
        debug_out("************** SPS30 init", DEBUG_MIN_INFO, true);
        zeroMeasurmentStruct(sum);
        sensirion_i2c_init();
        while ((ret = sps30_probe()) != 0) {
            Serial.print("SPS sensor probing failed\n");
            Serial.println(ret);
            delay(500);
        }
        sps30_reset();
        delay(200);
        if (sps30_get_serial(serial) != 0) {
            Serial.println("Error getting SPS serial");
            return 0;
        }
        debug_out("SPS30 serial: ", DEBUG_MIN_INFO, false);
        debug_out(serial, DEBUG_MIN_INFO, true);
        uint8_t major, minor;
        if (sps30_read_firmware_version(&major, &minor) == 0) {
            char tmp[60];
            sprintf(tmp, "SPS30 rev: %i.%i", major, minor);
            debug_out(tmp, DEBUG_MIN_INFO, true);
        }

        ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
        if (ret) {
            Serial.print("error setting the auto-clean interval: ");
            Serial.println(ret);
        }
        ret = sps30_start_measurement();
        if (ret < 0) {
            Serial.print("error starting measurement\n");
        } else {
            started = true;
        }

        return 10 * 1000;
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        struct sps30_measurement m;

        switch (e) {
            case SimpleScheduler::INIT:
                init();
                break;
            case SimpleScheduler::RUN:
                ret = sps30_read_measurement(&m);
                if (ret < 0) {
                    //Error reading
                } else {
                    addMeasurementStruct(sum, m);
                    measurement_count++;
                }
                return 5 * 1000;
        }
        return 15 * 1000;
    }

    void results(String &s) {
        if (!started || measurement_count == 0) return;
        String tmp; tmp.reserve(512);
        tmp += Value2Json(F("SPS30_P0"), String(sum.mc_1p0/measurement_count));
        tmp += Value2Json(F("SPS30_P2"), String(sum.mc_2p5/measurement_count));
        tmp += Value2Json(F("SPS30_P4"), String(sum.mc_4p0/measurement_count));
        tmp += Value2Json(F("SPS30_P1"), String(sum.mc_10p0/measurement_count));
        tmp += Value2Json(F("SPS30_N05"), String(sum.nc_0p5/measurement_count));
        tmp += Value2Json(F("SPS30_N1"), String(sum.nc_1p0/measurement_count));
        tmp += Value2Json(F("SPS30_N25"), String(sum.nc_2p5/measurement_count));
        tmp += Value2Json(F("SPS30_N4"), String(sum.nc_4p0/measurement_count));
        tmp += Value2Json(F("SPS30_N10"), String(sum.nc_10p0/measurement_count));
        tmp += Value2Json(F("SPS30_TS"), String(sum.typical_particle_size/measurement_count));
//        debug_out(tmp,DEBUG_MIN_INFO,true);
        s += tmp;
    }

}

#endif //NAMF_SENSIRION_SPS30_H
