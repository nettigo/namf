#include "sds011.h"

namespace SDS011 {
    const char KEY[]
            PROGMEM = "SDS011";
    bool enabled = false;
    bool printOnLCD = false;
    unsigned long warmupTime = WARMUPTIME_SDS_MS;
    unsigned long readTime = READINGTIME_SDS_MS;
    unsigned readingCount = 0;
    SerialSDS channelSDS(serialSDS);

    Sds011Async<SoftwareSerial> sds011(serialSDS);

    constexpr int pm_tablesize = 10;
    int pm25_table[pm_tablesize];
    int pm10_table[pm_tablesize];

    bool hardwareWatchdog = 0;
    byte hwWtdgFailedReadings = 0; //after SDS restart this counter is reset, failedReadings is global
    unsigned long hwWtdgErrors = 0;
    unsigned readings;
    unsigned failedReadings;
    enum {
        SDS_REPLY_HDR = 10,
        SDS_REPLY_BODY = 8
    } SDS_waiting_for;

    typedef enum {
        SER_UNDEF,
        SER_HDR,
        SER_DATA,
        SER_REPLY,
        SER_IDLE
    } SDSSerialState;
    SDSSerialState currState = SER_UNDEF;

    typedef enum {
        POWERON,    //after poweron
        STARTUP,    //first run
        POST,       //measure data on POST
        OFF,        // wait for measure period
        HARDWARE_RESTART,
        HW_RESTART_CLEANUP,
        START,      // start fan
        WARMUP,     // run, but no reading saved
        READ,       // start reading
        READING,    // run and collect data
        STOP,        // turn off
        AFTER_READING

    } SDS011State;

    unsigned long stateChangeTime = 0;
    SDS011State sensorState = POWERON;


    typedef enum {
        SDS_REPORTING,
        SDS_DATA,
        SDS_NEW_DEV_ID,
        SDS_SLEEP,
        SDS_PERIOD,
        SDS_FW_VER,
        SDS_UNKNOWN
    } SDSResponseType;
    const char SRT_0[] PROGMEM = "Reporting mode";
    const char SRT_1[] PROGMEM = "Data packet";
    const char SRT_2[] PROGMEM = "New ID";
    const char SRT_3[] PROGMEM = "Sleep/work";
    const char SRT_4[] PROGMEM = "Working period";
    const char SRT_5[] PROGMEM = "FW version";
    const char SRT_6[] PROGMEM = "Unknown";
    const char *SRT_NAMES[] PROGMEM = {SRT_0, SRT_1, SRT_2, SRT_3, SRT_4, SRT_5, SRT_6};

    SDSResponseType selectResponse(byte x) {
        switch (x) {
            case 7:
                return SDS_FW_VER;
            case 8:
                return SDS_PERIOD;
            case 6:
                return SDS_SLEEP;
            case 5:
                return SDS_NEW_DEV_ID;
            case 2:
                return SDS_REPORTING;
            default:
                return SDS_UNKNOWN;
        }
    }


    //change state and store timestamp
    void updateState(SDS011State newState) {
        if (newState == sensorState) return;
        sensorState = newState;
        stateChangeTime = millis();
    }

#define UPDATE_MIN(MIN, SAMPLE) if (SAMPLE < MIN) { MIN = SAMPLE; }
#define UPDATE_MAX(MAX, SAMPLE) if (SAMPLE > MAX) { MAX = SAMPLE; }
#define UPDATE_MIN_MAX(MIN, MAX, SAMPLE) { UPDATE_MIN(MIN, SAMPLE); UPDATE_MAX(MAX, SAMPLE); }

    void readSingleSDSPacket(int *pm10_serial, int *pm25_serial) {
//        *pm25_serial = replies[SDS_DATA].data[0] | (replies[SDS_DATA].data[1] << 8);
//        *pm10_serial = replies[SDS_DATA].data[2] | (replies[SDS_DATA].data[3] << 8);
//        replies[SDS_DATA].received = false;
        return;

        const uint8_t constexpr hdr_measurement[2] = {0xAA, 0xC0};
        uint8_t data[8];
        while (serialSDS.available() > SDS_waiting_for) {
            switch (SDS_waiting_for) {
                case SDS_REPLY_HDR:
                    if (serialSDS.find(hdr_measurement, sizeof(hdr_measurement)))
                        SDS_waiting_for = SDS_REPLY_BODY;
                    break;
                case SDS_REPLY_BODY:
                    debug_out(FPSTR(DBG_TXT_START_READING), DEBUG_MAX_INFO, 0);
                    debug_out(FPSTR(SENSORS_SDS011), DEBUG_MAX_INFO, 1);
                    size_t read = serialSDS.readBytes(data, sizeof(data));
                    if (read == sizeof(data) && SDS_checksum_valid(data)) {
                        *pm25_serial = data[0] | (data[1] << 8);
                        *pm10_serial = data[2] | (data[3] << 8);
                    } else {
                        *pm10_serial = -1;
                        *pm25_serial = -1;
                    }
                    SDS_waiting_for = SDS_REPLY_HDR;
                    for (byte i = 0; i < sizeof(data); i++) {
                        data[i] = 0;
                    }


            }
        }

    }


/*****************************************************************
 * send SDS011 command (start, stop, continuous mode, version    *
 *****************************************************************/
    void SDS_rawcmd(const uint8_t cmd_head1, const uint8_t cmd_head2, const uint8_t cmd_head3) {
        constexpr uint8_t cmd_len = 19;

        uint8_t buf[cmd_len];
        buf[0] = 0xAA;
        buf[1] = 0xB4;
        buf[2] = cmd_head1;
        buf[3] = cmd_head2;
        buf[4] = cmd_head3;
        for (unsigned i = 5; i < 15; ++i) {
            buf[i] = 0x00;
        }
        buf[15] = 0xFF;
        buf[16] = 0xFF;
        buf[17] = cmd_head1 + cmd_head2 + cmd_head3 - 2;
        buf[18] = 0xAB;
        serialSDS.write(buf, cmd_len);
    }

//    bool SDS_cmd(PmSensorCmd cmd) {
//        switch (cmd) {
//            case PmSensorCmd::Start:
//                SDS_rawcmd(0x06, 0x01, 0x01);
//                break;
//            case PmSensorCmd::Stop:
//                SDS_rawcmd(0x06, 0x01, 0x00);
//                break;
//            case PmSensorCmd::ContinuousMode:
//                // TODO: Check mode first before (re-)setting it
//                SDS_rawcmd(0x08, 0x01, 0x00);
//                SDS_rawcmd(0x02, 0x01, 0x00);
//                break;
//            case PmSensorCmd::VersionDate:
//                SDS_rawcmd(0x07, 0, 0);
//                break;
//        }
//
//        return cmd != PmSensorCmd::Stop;
//    }

//#define SDS_CMD_QUEUE_SIZE  5
//    PmSensorCmd cmdQ[SDS_CMD_QUEUE_SIZE];
//    byte cmdIdx = 0;
//    unsigned long lastCmdSent = 0;
//    PmSensorCmd sentCmd = PmSensorCmd::None;

//    bool SDS_cmd(PmSensorCmd cmd) {
//        if (cmdIdx < SDS_CMD_QUEUE_SIZE) {
//            cmdQ[cmdIdx++] = cmd;
//        }
//        return cmd != PmSensorCmd::Stop;
//
//    }
    bool SDS_cmd(PmSensorCmd cmd) {
        static constexpr uint8_t start_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                0x06, 0xAB
        };
        static constexpr uint8_t stop_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                0x05, 0xAB
        };
        static constexpr uint8_t continuous_mode_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                0x07, 0xAB
        };
        static constexpr uint8_t continuous_mode_cmd2[] PROGMEM = {
                0xAA, 0xB4, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                0x01, 0xAB
        };
        static constexpr uint8_t version_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
                0x05, 0xAB
        };
        constexpr uint8_t cmd_len = 19;

        uint8_t buf[cmd_len];
        switch (cmd) {
            case PmSensorCmd::Start:
//                Serial.println(F("SDS cmd: start"));
                memcpy_P(buf, start_cmd, cmd_len);
                break;
            case PmSensorCmd::Stop:
//                Serial.println(F("SDS cmd: stop"));
                memcpy_P(buf, stop_cmd, cmd_len);
                break;
            case PmSensorCmd::ContinuousMode:
//                Serial.println(F("SDS cmd: continuous"));
                memcpy_P(buf, continuous_mode_cmd, cmd_len);
                break;
            case PmSensorCmd::VersionDate:
//                Serial.println(F("SDS cmd: version"));
                memcpy_P(buf, version_cmd, cmd_len);
                break;
        }
        serialSDS.write(buf, cmd_len);
        return cmd != PmSensorCmd::Stop;
    }


    bool SDS_checksum_valid(const uint8_t (&data)[8]) {
        uint8_t checksum_is = 0;
        for (unsigned i = 0; i < 6; ++i) {
            checksum_is += data[i];
        }
        return (data[7] == 0xAB && checksum_is == data[6]);
    }


    String sds_internals() {
        String ret = "";
        for (byte i = 0; i < SDS_UNKNOWN; i++) {
            ret += F("<p>");
            ret += String(i);
            ret += F("(");
            ret += String(FPSTR(SRT_NAMES[i]));
            ret += F("), ");
            if (channelSDS._replies[i].received) {
                ret += F("otrzymano pakiet ");
                ret += String((millis() - channelSDS._replies[i].lastReply) / 1000);
                ret += F(" sekund temu.");
            }
            for (byte j = 0; j < 5; j++) {
                ret += String(channelSDS._replies[i].data[j], 16);
                ret += F(" ");
            }
            ret += F("</p>");
        }
        return ret;
    }


    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::SDS011);
        //use display?
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::SDS011);
        setVariableFromHTTP(F("w"), warmupTime, SimpleScheduler::SDS011);
        setVariableFromHTTP(String(F("r")), readTime, SimpleScheduler::SDS011);
        setBoolVariableFromHTTP(F("dbg"), hardwareWatchdog, SimpleScheduler::SDS011);

        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        ret[F("w")] = warmupTime;
        ret[F("r")] = readTime;
        ret[F("dbg")] = hardwareWatchdog;
        return ret;


    };

    bool getDisplaySetting() {
        return printOnLCD;
    };

    bool display(byte rows, byte minor, String lines[]) {
        byte row = 0;
        if (rows == 4) {
            lines[row] += FPSTR(INTL_SDS011_LCD_HDR);
        }
        row++;;
        lines[row] += F("PM2.5:");
        lines[row] += (check_display_value(last_value_SDS_P2, -1, 1, 6));
//        lines[row] += (F(" µg/m³"));
        row++;;
        lines[row] += F("PM10: ");
        lines[row] += check_display_value(last_value_SDS_P1, -1, 1, 6);
//        lines[row] += (F(" µg/m³"));

        return false;
    };

    String getConfigJSON() {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        if (printOnLCD) ret += Var2JsonInt(F("d"), printOnLCD);
        addJsonIfNotDefault(ret, F("r"), READINGTIME_SDS_MS, readTime);
        addJsonIfNotDefault(ret, F("w"), WARMUPTIME_SDS_MS, warmupTime);
        addJsonIfNotDefault(ret, F("dbg"), false, hardwareWatchdog);
//        if (readTime != READINGTIME_SDS_MS) ret += Var2Json(F("r"), readTime);
//        if (warmupTime != WARMUPTIME_SDS_MS) ret += Var2Json(F("w"), warmupTime);
        return ret;
    };

    void readConfigJSON(JsonObject &json) {
//        Serial.println("SDS readConfigJson");
//        json.printTo(Serial);
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));
        if (json.containsKey(F("r"))) {
            readTime = json.get<unsigned long>(F("r"));
        }
        if (json.containsKey(F("w"))) {
            warmupTime = json.get<unsigned long>(F("w"));
        }
        if (json.containsKey(F("dbg"))) {
            hardwareWatchdog = json.get<bool>(F("dbg"));
        }


        //register/deregister sensor
        if (enabled && !scheduler.isRegistered(SimpleScheduler::SDS011)) {
            scheduler.registerSensor(SimpleScheduler::SDS011, SDS011::process, FPSTR(SDS011::KEY));
            scheduler.init(SimpleScheduler::SDS011);
            enabled = true;
            debug_out(F("SDS011: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::SDS011)) {   //de
            debug_out(F("SDS011: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::SDS011);
        }
        //register display - separate check to allow enable/disable display not only when turning SDS011 on/off
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::SDS011, 1);
    }


    //did timeout happen (from last state change)
    bool timeout(unsigned long t) {
        //100 ms margin - to complete before sending
        unsigned long diff = millis() + 100 - stateChangeTime;
        if (diff > t) return true;
        return false;
    }

#define STARTUP_TIME  5000
//how many ms before read time reading cycle should be fished
#define SDS011_END_TIME 2000

    //select proper state, depending on time left to
    unsigned long processState() {
        int pm25, pm10 = -1;
        unsigned long t = time2Measure();
//        if (t>1000) t -= 200;
        switch (sensorState) {
            case POWERON:
                updateState(STARTUP);
                return 10;
            case STARTUP:

                updateState(POST);
                return 100;
            case POST:

                if (timeout(STARTUP_TIME)) {
//                    is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
                    updateState(OFF);
                    return 500;
                }
                return 20;
            case OFF:

                if (t < warmupTime + readTime + SDS011_END_TIME)   //aim to finish 2 sec before readingTime
                {
                    if (!sds011.set_sleep(false)) { Serial.println(F("\n\nSDS011 nie wstał!\n\n")); };
                    updateState(START);
                }
                return 10;
            case HARDWARE_RESTART:
                if (timeout(5 * 1000)) {

                }
                return 10;
            case HW_RESTART_CLEANUP:
                if (timeout(5000)) {

                }
                return 10;
            case START:
                sds011.on_query_data_auto_completed([](int n) {
                    Serial.println("Begin Handling SDS011 query data");
                    int pm25;
                    int pm10;
                    Serial.print("n = ");
                    Serial.println(n);
                    if (sds011.filter_data(n, pm25_table, pm10_table, pm25, pm10) &&
                        !isnan(pm10) && !isnan(pm25)) {
//                        Serial.println(F("NEW DATA yaay!"));
                        last_value_SDS_P1 = float(pm10) / 10;
                        last_value_SDS_P2 = float(pm25) / 10;
                    } else {
                        last_value_SDS_P1 = last_value_SDS_P2 = -1;
                        hwWtdgFailedReadings++;
                    }
                    Serial.println("End Handling SDS011 query data");
                });
//                sds011.perform_work();
                if (!sds011.query_data_auto_async(pm_tablesize, pm25_table, pm10_table)) {
                    Serial.println("measurement capture start failed");
                }
//                if (t < SDS011_END_TIME)
                updateState(READING);
                return 100;
            case WARMUP:
                updateState(READ);
                return 10;
            case READ:
                updateState(READING);
                return 10;
            case READING:
                if (t < SDS011_END_TIME)
                    updateState(STOP);
                return 20;
            case STOP:
                debug_out(F("SDS011: end of cycle"), DEBUG_MIN_INFO, 1);
                sds011.set_sleep(true);
                updateState(AFTER_READING);
                return 100;
            default:
                return 1000;
        }
    }

    void results(String &res) {
        readings++;
        if (last_value_SDS_P2 != -1 && last_value_SDS_P1 != -1) {

            res += Value2Json(F("SDS_P1"), String(last_value_SDS_P1));
            res += Value2Json(F("SDS_P2"), String(last_value_SDS_P2));
        } else {
            failedReadings++;
        }
    }

    void sendToLD() {
        if (!enabled) return;
        if (sensorState == AFTER_READING) {
            updateState(OFF);
            const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);
            String data;
            results(data);
            sendLuftdaten(data, 1, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "SDS_");
        } else {
            debug_out(F("SDS011 reading procedure not finished, not sending data."), DEBUG_MIN_INFO);
            debug_out(String(stateChangeTime), DEBUG_MIN_INFO);
            debug_out(String(sensorState), DEBUG_MIN_INFO);
        }
    }

    void getStatusReport(String &res) {
        if (!enabled) return;

        res += FPSTR(EMPTY_ROW);
        res += table_row_from_value(F("SDS011"), F("Status"), String(sensorState), "");
        res += table_row_from_value(F("SDS011"), F("Status change"), String(millis() - stateChangeTime), "");
        res += table_row_from_value(F("SDS011"), F("Version data"), SDS_version_date(), "");
        if (hardwareWatchdog) {
            res += table_row_from_value(F("SDS011"), F("Failed I2C PCF"), String(hwWtdgErrors), "");
        }
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        static unsigned long lastPerform = 0;
        switch (e) {
            case SimpleScheduler::STOP:
                if (sds011.set_sleep(true)) { is_SDS_running = false; }
                return 0;
            case SimpleScheduler::INIT:
                readings = failedReadings = 0;
                Serial.println(F("SDS011 INIT ***********\n\n"));
                Sds011::Report_mode report_mode;
                sds011.set_sleep(false);
                delay(500);

                if (!sds011.get_data_reporting_mode(report_mode)) {
                    Serial.println("Sds011::get_data_reporting_mode() failed");
                }
                if (Sds011::REPORT_ACTIVE != report_mode) {
                    Serial.println("Turning on Sds011::REPORT_ACTIVE reporting mode");
                    if (!sds011.set_data_reporting_mode(Sds011::REPORT_ACTIVE)) {
                        Serial.println("Sds011::set_data_reporting_mode(Sds011::REPORT_ACTIVE) failed");
                    }
                }
                processState();
                if (sds011.set_sleep(true)) {
                    is_SDS_running = false;
                    return 10;
                } else {
                    return 1;
                }

            case SimpleScheduler::RUN:
//                channelSDS.process();
                if (millis() - lastPerform > 100) {
//                    if (serialSDS.available() > 0) {
//                        Serial.print(F("Serial available: "));
//                        Serial.println(serialSDS.available());
//                    }
                    sds011.perform_work();
//                    Serial.println("PRRFW");
                    lastPerform = millis();
                }
                processState();
                return 1;
            default:
                return 0;
        }
    }

    String getConfigHTML(void) {
        String ret = F("");
        String name;
        setHTTPVarName(name, F("r"), SimpleScheduler::SDS011);
        ret += form_input(name, FPSTR(INTL_SDS011_READTIME), String(readTime), 7);

        setHTTPVarName(name, F("w"), SimpleScheduler::SDS011);
        ret += form_input(name, FPSTR(INTL_SDS011_WARMUP), String(warmupTime), 7);

        setHTTPVarName(name, F("dbg"), SimpleScheduler::SDS011);
        ret += form_checkbox(name, F("Hardware restarter"), hardwareWatchdog, true);

        return ret;
    }

    void resultsAsHTML(String &page_content) {
        if (!enabled) return;
        page_content += FPSTR(EMPTY_ROW);
        page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM2.5",
                                             check_display_value(last_value_SDS_P2, -1, 1, 0), F("µg/m³"));
        page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM10",
                                             check_display_value(last_value_SDS_P1, -1, 1, 0), F("µg/m³"));
    }

    /*****************************************************************
     * read SDS011 sensor serial and firmware date                   *
     *****************************************************************/
    String SDS_version_date() {
        static String version_date = "";
        version_date.reserve(15);
        unsigned short id = 0;
        bool old_state;
        debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(DBG_TXT_SDS011_VERSION_DATE), DEBUG_MED_INFO, 1);
        if (version_date.length() < 3) {
            if (!(old_state = is_SDS_running)) {
                is_SDS_running = sds011.set_sleep(false);
//                delay(500);
            }
            if (sds011.device_info(version_date, id)) {
//                version_date += F("(")+String((byte)(id & 0xff),16)+String((byte)(id >> 8),16)+F(")");
                char hex_id[8];
                snprintf_P(hex_id, 8, PSTR("(%02X%02X)"), id & 0xff, id >> 8);
                version_date += hex_id;

            } else {
                version_date = F("n/a");
            }
            if (!old_state && is_SDS_running) {
                is_SDS_running = sds011.set_sleep(true);
            }

        }
        return version_date;


    }
}