#include <Arduino.h>
#include "sensor.h"
#include "helpers.h"
#include "sending.h"
#include "display/commons.h"
#include "system/scheduler.h"

extern String table_row_from_value(const String &sensor, const String &param, const String &value, const String &unit);


namespace SPS30 {
    const char KEY[] PROGMEM = "SPS30";
    bool started = false;
    bool enabled = false;
    bool printOnLCD = false;

    SensirionI2cSps30 sensor;

    unsigned long refresh = 10;
    int16_t ret;
    uint8_t auto_clean_days = 4;
    uint32_t auto_clean;
    sps30_measurement sum;
    unsigned int measurement_count;
    char serial[SPS_MAX_SERIAL_LEN];

    //helper function to zero measurements struct (for averaging)
    void zeroMeasurementStruct(sps30_measurement &str) {
        sps30_measurement zero = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        str = zero;
    }

    void dumpStruct(sps30_measurement s) {
        debugOutLn(F("SPS30 measurement"), DEBUG_MAX_INFO);
        debugOut(F("mc1p0: "), DEBUG_MAX_INFO);
        debugOut(String(s.mc1p0), DEBUG_MAX_INFO);
        debugOut(F(", mc2p5: "), DEBUG_MAX_INFO);
        debugOut(String(s.mc2p5), DEBUG_MAX_INFO);
        debugOut(F(" mc4p0: "), DEBUG_MAX_INFO);
        debugOut(String(s.mc4p0), DEBUG_MAX_INFO);
        debugOut(F(" mc10p0: "), DEBUG_MAX_INFO);
        debugOut(String(s.mc10p0), DEBUG_MAX_INFO);
        debugOutLn(F(""),DEBUG_MAX_INFO);
    }

    //return string with HTML used to configure SPS30 sensor. Right now it only takes number of seconds to wait between measurements
    //taken to average
    String getConfigHTML(void) {
        String ret = F("");
        ret += formInputGrid(F("refresh"), FPSTR(INTL_SPS30_REFRESH), String(refresh), 4);
        return ret;
    }

    //register LCD display, for internal SPS30 use
    void registerDisplaySPS() {
        // register display
        if (printOnLCD) {
            if (started) {
                switch (getLCDRows()) {
                    case 4:
                        scheduler.registerDisplay(SimpleScheduler::SPS30, 2);
                        break;
                    case 2:
                        scheduler.registerDisplay(SimpleScheduler::SPS30, 5);
                        break;
                    default:
                        break;
                }
            } else {
                scheduler.registerDisplay(SimpleScheduler::SPS30, 1);
            }
        } else {    //disable display if changed state in runtime
            scheduler.registerDisplay(SimpleScheduler::SPS30,0);
        }

    }

    //Start SPS30 sensor
    unsigned long init() {
        debug_out(F("************** SPS30 init"), DEBUG_MIN_INFO, true);
        sensor.begin(Wire, SPS30_I2C_ADDR_69);

        zeroMeasurementStruct(sum);
        sensor.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT);
        registerDisplaySPS();
        return 10 * 1000;
    }


    //callback to parse HTML form sent from `getConfigHTML`
    JsonObject &parseHTTPRequest(void) {
        parseHTTP(F("refresh"), refresh);
        //enabled?
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::SPS30);
        //use display?
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::SPS30);
        registerDisplaySPS();  //register display if enabled on runtime

        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("refresh")] = refresh;
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        return ret;
    }

    //helper function to sum current measurement with previous results - for averaging
    void addMeasurementStruct(sps30_measurement &storage, sps30_measurement reading) {
        storage.mc1p0 += reading.mc1p0;
        storage.mc2p5 += reading.mc2p5;
        storage.mc4p0 += reading.mc4p0;
        storage.mc10p0 += reading.mc10p0;
        storage.nc0p5 += reading.nc0p5;
        storage.nc1p0 += reading.nc1p0;
        storage.nc2p5 += reading.nc2p5;
        storage.nc4p0 += reading.nc4p0;
        storage.nc10p0 += reading.nc10p0;
        storage.typicalParticleSize += reading.typicalParticleSize;
    }

    /************************************************************************
     * main function called periodically. It is called by scheduler with single param taking values:
     * INIT - first run (aka start of sensor)
     * RUN - regular call during work
     * STOP - sensor is being deactivated
     */

    unsigned long process(SimpleScheduler::LoopEventType e) {
        sps30_measurement m;

        switch (e) {
            case SimpleScheduler::STOP:
                zeroMeasurementStruct(sum);
                measurement_count = 0;
                started = false;
                sensor.stopMeasurement();
                return 0;
            case SimpleScheduler::INIT:
                init();
                break;
            case SimpleScheduler::RUN:
                uint16_t dataReadyFlag = 0;
                int error = sensor.readDataReadyFlag(dataReadyFlag);
                debug_out(F("SPS30: process"), DEBUG_MAX_INFO, true);
                if (error) {
                    debugOutLn(F("SPS30: read data ready"), DEBUG_ERROR);
                    return refresh * 1000;
                }
                ret = sensor.readMeasurementValuesFloat(m.mc1p0, m.mc2p5, m.mc4p0, m.mc10p0,
                                              m.nc0p5, m.nc1p0, m.nc2p5, m.nc4p0,
                                              m.nc10p0, m.typicalParticleSize);

                if (ret < 0) {
                    debugOutLn(F("Error reading Float from SPS30"), DEBUG_ERROR);
                } else {
                    addMeasurementStruct(sum, m);
                    measurement_count++;
                }
                return refresh * 1000;
        }
        return 15 * 1000;
    }

    void getStatusReport (String &page_content) {
        if (!enabled) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        uint8_t major, minor;
        if (sensor.readFirmwareVersion(major, minor) == 0) {
            page_content.concat(table_row_from_value(FPSTR(KEY), F("FW ver"), String(major)+String(F("."))+String(minor), ""));
        } else
            page_content.concat(table_row_from_value(FPSTR(KEY), F("FW ver"), FPSTR(INTL_SPS30_FW_FAIL), ""));



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
        if (printOnLCD) ret += Var2JsonInt(F("d"), printOnLCD);
        ret += Var2Json(F("refresh"), refresh);
        return ret;
    }

    //get configuration data from JsonObject and save in own config
    // if sensor is enabled then run init. On shutdown
    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));
        refresh = json.get<int>(F("refresh"));

        if (enabled && !scheduler.isRegistered(SimpleScheduler::SPS30)) {
            scheduler.registerSensor(SimpleScheduler::SPS30, SPS30::process, FPSTR(SPS30::KEY));
            scheduler.init(SimpleScheduler::SPS30);
            enabled = true;
            debug_out(F("SPS30 enabled"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::SPS30)) {   //de
            debug_out(F("SPS30: stop"),DEBUG_MIN_INFO,1);
            scheduler.unregisterSensor(SimpleScheduler::SPS30);
        }
    }

    //append current readings (averaged) to JSON string
    void results(String &s) {
        if (!enabled || !started || measurement_count == 0) return;
        String tmp;
        tmp.reserve(512);

        tmp.concat(Value2Json(F("SPS30_P0"), String(sum.mc1p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_P2"), String(sum.mc2p5 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_P4"), String(sum.mc4p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_P1"), String(sum.mc10p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_N05"), String(sum.nc0p5 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_N1"), String(sum.nc1p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_N25"), String(sum.nc2p5 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_N4"), String(sum.nc4p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_N10"), String(sum.nc10p0 / measurement_count)));
        tmp.concat(Value2Json(F("SPS30_TS"), String(sum.typicalParticleSize / measurement_count)));
//        debug_out(tmp,DEBUG_MIN_INFO,true);
        s.concat(tmp);
    }

    //send data to LD API...
    void sendToLD(){
        const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);

        String data;
        results(data);
        sendLuftdaten(data, 1 , HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "SPS30_");
    };

    //display table elements for current values page
    void resultsAsHTML(String &page_content) {
        if (!enabled) { return; }
        const String unit_PM = "µg/m³";
        const String unit_T = "°C";
        const String unit_H = "%";
        const String unit_P = "hPa";

        page_content.concat(FPSTR(EMPTY_ROW));
        if (measurement_count == 0) {
            page_content.concat(table_row_from_value(FPSTR("SPS30"), FPSTR(INTL_SPS30_NO_RESULT), F(""), unit_PM));

        } else {
            page_content.concat(F("<tr><td colspan='3'>"));
            page_content.concat(FPSTR(INTL_SPS30_CONCENTRATIONS));
            page_content.concat(F("</td></tr>\n"));

            page_content.concat(table_row_from_value(F("SPS30"), F("PM1"), String(sum.mc1p0 / measurement_count), unit_PM));
            page_content.concat(table_row_from_value(F("SPS30"), F("PM2.5"), String(sum.mc2p5 / measurement_count),
                                                 unit_PM));
            page_content.concat(table_row_from_value(F("SPS30"), F("PM4"), String(sum.mc4p0 / measurement_count), unit_PM));
            page_content.concat(table_row_from_value(F("SPS30"), F("PM10"), String(sum.mc10p0 / measurement_count),
                                                 unit_PM));

            page_content.concat(F("<tr><td colspan='3'>"));
            page_content.concat(FPSTR(INTL_SPS30_COUNTS));
            page_content.concat(F("</td></tr>\n"));

            page_content.concat(table_row_from_value(F("SPS30"), F("NC0.5"), String(sum.nc0p5 / measurement_count),
                                                 FPSTR(INTL_SPS30_CONCENTRATION)));
            page_content.concat(table_row_from_value(F("SPS30"), F("NC1.0"), String(sum.nc1p0 / measurement_count),
                                                 FPSTR(INTL_SPS30_CONCENTRATION)));
            page_content.concat(table_row_from_value(F("SPS30"), F("NC2.5"), String(sum.nc2p5 / measurement_count),
                                                 FPSTR(INTL_SPS30_CONCENTRATION)));
            page_content.concat(table_row_from_value(F("SPS30"), F("NC4.0"), String(sum.nc4p0 / measurement_count),
                                                 FPSTR(INTL_SPS30_CONCENTRATION)));
            page_content.concat(table_row_from_value(F("SPS30"), F("NC10.0"), String(sum.nc10p0 / measurement_count),
                                                 FPSTR(INTL_SPS30_CONCENTRATION)));
            page_content.concat(table_row_from_value(F("SPS30"), F("TS"),
                                                 String(sum.typicalParticleSize / measurement_count),
                                                 FPSTR(INTL_SPS30_SIZE)));

        }

    }

    void display(byte cols, byte minor, String lines[]) {
        if (!started) {
            lines[0] = F("SPS30");
            lines[1] = FPSTR(INTL_SPS30_NOT_STARTED);
            return;
        }
        if (measurement_count == 0) {
            lines[0] = F("SPS30");
            lines[1] = FPSTR(INTL_SPS30_NO_RESULT);
            return;
        }
        byte row = 0;

        if (getLCDRows() == 2) {    //16x2
            switch (minor) {
                case 0:
                    lines[row] += (F("SPS: PM1:"));
                    lines[row] += (String(sum.mc1p0 / measurement_count, 1));
                    row++;
                    lines[row] += (F("PM2.5: "));
                    lines[row] += (String(sum.mc2p5 / measurement_count, 1));
                    break;
                case 1:
                    lines[row] += (F("SPS: PM4:"));
                    lines[row] += (String(sum.mc4p0 / measurement_count, 1));
                    row++;
                    lines[row] += (F("PM10: "));
                    lines[row] += (String(sum.mc10p0 / measurement_count, 1));
                    break;
                case 2:
                    lines[row] += (F("SPS: NC1:"));
                    lines[row] += (String(sum.nc1p0 / measurement_count, 1));
                    row++;
                    lines[row] += (F("NC2.5: "));
                    lines[row] += (String(sum.nc2p5 / measurement_count, 1));
                    break;
                case 3:
                    lines[row] += (F("SPS: NC4:"));
                    lines[row] += (String(sum.nc4p0 / measurement_count, 1));
                    row++;
                    lines[row] += (F("NC10: "));
                    lines[row] += (String(sum.nc10p0 / measurement_count, 1));
                    break;
                case 4:
                    lines[row] += (F("Typical size:"));
                    row++;
                    lines[row] += (String(sum.typicalParticleSize / measurement_count, 2));

            }

        } else {    //LCD 20x4 or OLED, more real estate
            switch (minor) {
                case 0:
                    lines[row] = F("SPS: PM1:");
                    lines[row] += String(sum.mc1p0 / measurement_count, 1);
                    row++;
                    lines[row] = F("PM2.5: ");
                    lines[row] += String(sum.mc2p5 / measurement_count, 1);
                    row++;
                    lines[row] = F("PM4:   ");
                    lines[row] += String(sum.mc4p0 / measurement_count, 1);
                    row++;
                    lines[row] = F("PM10:  ");
                    lines[row] += String(sum.mc10p0 / measurement_count, 1);
                    break;
                case 1:
                    lines[row] = F(" SPS: NC1:");
                    lines[row] += String(sum.nc1p0 / measurement_count, 1);
                    row++;
                    lines[row] = F("NC2.5: ");
                    lines[row] += String(sum.nc2p5 / measurement_count, 1);
                    row++;
                    lines[row] = F("NC4:   ");
                    lines[row] += String(sum.nc4p0 / measurement_count, 1);
                    row++;
                    lines[row] = F("NC10:");
                    lines[row] += String(sum.nc10p0 / measurement_count, 1);
                    lines[row] += F(" TS:");
                    lines[row] += String(sum.typicalParticleSize / measurement_count, 1);
                    break;
            }
        }
        return;
    };

    bool getDisplaySetting() {
        return printOnLCD;
    };

}
