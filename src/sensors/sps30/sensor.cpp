#include "sensor.h"

extern String table_row_from_value(const String& sensor, const String& param, const String& value, const String& unit);


namespace SPS30 {
    bool started = false;

    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    struct sps30_measurement sum ;
    unsigned int measurement_count;
    char serial[SPS_MAX_SERIAL_LEN];

    void zeroMeasurementStruct(sps30_measurement &str) {
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
        zeroMeasurementStruct(sum);
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

    //we will reset average even on API failure
    void afterSend(bool success) {
        zeroMeasurementStruct(sum);
        measurement_count = 0;
    }

    //return JSON with results
    void results(String &s) {
        if (!started || measurement_count == 0) return;
        String tmp; tmp.reserve(512);
        tmp += Value2Json(F("SPS30_P0"), String(sum.mc_1p0/measurement_count));
        tmp += Value2Json(F("SPS30_P2"), String(sum.mc_2p5/measurement_count));
        tmp += Value2Json(F("SPS30_P4"), String(sum.mc_4p0/measurement_count));
        tmp += Value2Json(F("SPS30_P1"), String(sum.mc_10p0 / measurement_count));
        tmp += Value2Json(F("SPS30_N05"), String(sum.nc_0p5 / measurement_count));
        tmp += Value2Json(F("SPS30_N1"), String(sum.nc_1p0 / measurement_count));
        tmp += Value2Json(F("SPS30_N25"), String(sum.nc_2p5 / measurement_count));
        tmp += Value2Json(F("SPS30_N4"), String(sum.nc_4p0 / measurement_count));
        tmp += Value2Json(F("SPS30_N10"), String(sum.nc_10p0 / measurement_count));
        tmp += Value2Json(F("SPS30_TS"), String(sum.typical_particle_size / measurement_count));
//        debug_out(tmp,DEBUG_MIN_INFO,true);
        s += tmp;
    }

    void resultsAsHTML(String &page_content) {
        const String unit_PM = "µg/m³";
        const String unit_T = "°C";
        const String unit_H = "%";
        const String unit_P = "hPa";

        page_content += FPSTR(EMPTY_ROW);
        if (measurement_count == 0) {
            page_content += table_row_from_value(FPSTR("SPS30"), FPSTR(INTL_SPS30_NO_RESULT), F(""), unit_PM);

        }else {
            page_content += F("<tr><td colspan='3'>");
            page_content += FPSTR(INTL_SPS30_CONCENTRATIONS);
            page_content += F("</td></tr>\n");

            page_content += table_row_from_value(F("SPS30"), F("PM1"), String(sum.mc_1p0/measurement_count), unit_PM);
            page_content += table_row_from_value(F("SPS30"), F("PM2.5"), String(sum.mc_2p5/measurement_count), unit_PM);
            page_content += table_row_from_value(F("SPS30"), F("PM4"), String(sum.mc_4p0/measurement_count), unit_PM);
            page_content += table_row_from_value(F("SPS30"), F("PM10"), String(sum.mc_10p0/measurement_count), unit_PM);

            page_content += F("<tr><td colspan='3'>");
            page_content += FPSTR(INTL_SPS30_COUNTS);
            page_content += F("</td></tr>\n");

            page_content += table_row_from_value(F("SPS30"), F("NC0.5"), String(sum.nc_0p5/measurement_count), FPSTR(INTL_SPS30_CONCENTRATION));
            page_content += table_row_from_value(F("SPS30"), F("NC1.0"), String(sum.nc_1p0/measurement_count),FPSTR(INTL_SPS30_CONCENTRATION));
            page_content += table_row_from_value(F("SPS30"), F("NC2.5"), String(sum.nc_2p5/measurement_count), FPSTR(INTL_SPS30_CONCENTRATION));
            page_content += table_row_from_value(F("SPS30"), F("NC4.0"), String(sum.nc_4p0/measurement_count), FPSTR(INTL_SPS30_CONCENTRATION));
            page_content += table_row_from_value(F("SPS30"), F("NC10.0"), String(sum.nc_10p0/measurement_count), FPSTR(INTL_SPS30_CONCENTRATION));
            page_content += table_row_from_value(F("SPS30"), F("TS"), String(sum.typical_particle_size/measurement_count), FPSTR(INTL_SPS30_SIZE));

        }

    }

}
