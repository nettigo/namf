#include <Arduino.h>
#include "sensor.h"

extern String table_row_from_value(const String& sensor, const String& param, const String& value, const String& unit);


namespace SPS30 {
    const char KEY[] PROGMEM = "SPS30";
    bool started = false;
    bool enabled = false;
    unsigned long refresh = 10;
    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    struct sps30_measurement sum;
    unsigned int measurement_count;
    char serial[SPS_MAX_SERIAL_LEN];

    //helper function to zero measurements struct (for averaging)
    void zeroMeasurementStruct(sps30_measurement &str) {
        sps30_measurement zero = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        str = zero;
    }

    //return string with HTML used to configure SPS30 sensor. Right now it only takes number of seconds to wait between measurements
    //taken to average
    String getConfigHTML(void) {
        String ret = F("<h1>SPS30</h1>");
        ret += form_input(F("refresh"), FPSTR(INTL_SPS30_REFRESH), String(refresh), 4);
        return ret;
    }

    //callback to parse HTML form sent from `getConfigHTML`
    JsonObject& parseHTTPRequest(void) {
        parseHTTP(F("refresh"), refresh);
        String sensorID = F("enabled-{s}");
        sensorID.replace(F("{s}"),String(SimpleScheduler::SPS30));
        parseHTTP(sensorID, enabled);

        StaticJsonBuffer<256> jsonBuffer;
        JsonObject & ret = jsonBuffer.createObject();
        ret[F("refresh")] = refresh;
        ret[F("e")] = enabled;
        return ret;
    }

    //helper function to sum current measurement with previous results - for averaging
    void addMeasurementStruct(sps30_measurement &storage, sps30_measurement reading) {
        storage.mc_1p0 += reading.mc_1p0;
        storage.mc_2p5 += reading.mc_2p5;
        storage.mc_4p0 += reading.mc_4p0;
        storage.mc_10p0 += reading.mc_10p0;
        storage.nc_0p5 += reading.nc_0p5;
        storage.nc_1p0 += reading.nc_1p0;
        storage.nc_2p5 += reading.nc_2p5;
        storage.nc_4p0 += reading.nc_4p0;
        storage.nc_10p0 += reading.nc_10p0;
        storage.typical_particle_size += reading.typical_particle_size;
    }

    //Start SPS30 sensor
    unsigned long init() {
        debug_out("************** SPS30 init", DEBUG_MIN_INFO, true);
        zeroMeasurementStruct(sum);
        sensirion_i2c_init();
        byte cnt = 0;
        while (
                ((ret = sps30_probe()) != 0) &&
                (cnt++ < 15)
                ) {
            delay(500);
            if (cnt == 10) {
                debug_out(F("SPS30 probing failed, disabling sensor"), DEBUG_ERROR, true);
                return 0;
            }
        }
        sps30_reset();
        delay(200);
        if (sps30_get_serial(serial) != 0) {
            debug_out(F("Error getting SPS30 serial"), DEBUG_ERROR, true);
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
            debug_out(F("error setting the auto-clean interval: "), DEBUG_ERROR, true);
            Serial.println(ret);
        }
        ret = sps30_start_measurement();
        if (ret < 0) {
            debug_out(F("error starting measurement"), DEBUG_ERROR, true);
        } else {
            started = true;
        }
        return 10 * 1000;
    }

    /************************************************************************
     * main function called periodically. It is called by scheduler with single param taking values:
     * INIT - first run (aka start of sensor)
     * RUN - regular call during work
     * STOP - sensor is being deactivated
     */

    unsigned long process(SimpleScheduler::LoopEventType e) {
        struct sps30_measurement m;

        switch (e) {
            case SimpleScheduler::STOP:
                zeroMeasurementStruct(sum);
                measurement_count = 0;
                started = false;
                sps30_stop_measurement();
                return 0;
            case SimpleScheduler::INIT:
                init();
                break;
            case SimpleScheduler::RUN:
                debug_out(F("SPS30: process"), DEBUG_MAX_INFO, true);
                ret = sps30_read_measurement(&m);
                if (ret < 0) {
                    //Error reading
                } else {
                    addMeasurementStruct(sum, m);
                    measurement_count++;
                }
                return refresh * 1000;
        }
        return 15 * 1000;
    }

    //callback called after each sending data to APIs Bool parameter tell if there was success, or some API call failed
    //in SPS30 it is being used start averaging results
    void afterSend(bool success) {
        zeroMeasurementStruct(sum);
        measurement_count = 0;
    }

    //return JSON string with config (for save in SPIFFS)
    String getConfigJSON(void) {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        ret += Var2Json(F("refresh"), refresh);
        return ret;
    }

    //get configuration data from JsonObject and save in own config
    // if sensor is enabled then run init. On shutdown
    void readConfigJSON(JsonObject &json){
        enabled = json.get<bool>(F("e"));
        refresh = json.get<int>(F("refresh"));

        if (enabled && !scheduler.isRegistered(SimpleScheduler::SPS30) ) {
            scheduler.registerSensor(SimpleScheduler::SPS30, SPS30::process, FPSTR(SPS30::KEY));
            scheduler.init(SimpleScheduler::SPS30);
            enabled = true;
            Serial.println(F("SPS30 enabled"));
        } else if(scheduler.isRegistered(SimpleScheduler::SPS30)) {   //de
            Serial.println(F("SPS30: stop"));
            scheduler.unregisterSensor(SimpleScheduler::SPS30);
            enabled = false;
        }
    }

    //append current readings (averaged) to JSON string
    void results(String &s) {
        if (!enabled || !started || measurement_count == 0) return;
        String tmp;
        tmp.reserve(512);
        tmp += Value2Json(F("SPS30_P0"), String(sum.mc_1p0 / measurement_count));
        tmp += Value2Json(F("SPS30_P2"), String(sum.mc_2p5 / measurement_count));
        tmp += Value2Json(F("SPS30_P4"), String(sum.mc_4p0 / measurement_count));
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

    //display table elements for current values page
    void resultsAsHTML(String &page_content) {
        if (!enabled) {return;}
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
