#include "sds011.h"

namespace SDS011 {
    const char KEY[]
    PROGMEM = "SDS011";
    bool enabled = false;
    bool printOnLCD = false;
    unsigned long SDS_error_count;
    unsigned long warmupTime = WARMUPTIME_SDS_MS;
    unsigned long readTime = READINGTIME_SDS_MS;
    unsigned long pm10Sum, pm25Sum = 0;
    unsigned readingCount = 0;
    SerialSDS channelSDS(serialSDS);
    bool hardwareWatchdog = 0;
    byte hwWtdgFailedReadings = 0; //after SDS restart this counter is reset, failedReadings is global

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
    const char SRT_0 [] PROGMEM = "Reporting mode";
    const char SRT_1 [] PROGMEM = "Data packet";
    const char SRT_2 [] PROGMEM = "New ID";
    const char SRT_3 [] PROGMEM = "Sleep/work";
    const char SRT_4 [] PROGMEM = "Working period";
    const char SRT_5 [] PROGMEM = "FW version";
    const char SRT_6 [] PROGMEM = "Unknown";
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

    typedef struct {
        bool sent;
        bool received;
        unsigned long lastRequest;
        unsigned long lastReply;
        byte data[5];
    } SDSReplyInfo;

    SDSReplyInfo replies[SDS_UNKNOWN];
    unsigned checksumFailed = 0;
    unsigned long packetCount = 0;
    PCF8574 *PCF = nullptr;

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
        *pm25_serial = replies[SDS_DATA].data[0] | (replies[SDS_DATA].data[1] << 8);
        *pm10_serial = replies[SDS_DATA].data[2] | (replies[SDS_DATA].data[3] << 8);
        replies[SDS_DATA].received = false;
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
                0xAA, 0xB4, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x06, 0xAB
        };
        static constexpr uint8_t stop_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB
        };
        static constexpr uint8_t continuous_mode_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x07, 0xAB
        };
        static constexpr uint8_t continuous_mode_cmd2[] PROGMEM = {
                0xAA, 0xB4, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0xAB
        };
        static constexpr uint8_t version_cmd[] PROGMEM = {
                0xAA, 0xB4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB
        };
        constexpr uint8_t cmd_len = 19;

        uint8_t buf[cmd_len];
        switch (cmd) {
            case PmSensorCmd::Start:
                Serial.println(F("SDS cmd: start"));
                memcpy_P(buf, start_cmd, cmd_len);
                break;
            case PmSensorCmd::Stop:
                Serial.println(F("SDS cmd: stop"));
                memcpy_P(buf, stop_cmd, cmd_len);
                break;
            case PmSensorCmd::ContinuousMode:
                Serial.println(F("SDS cmd: continuous"));
                memcpy_P(buf, continuous_mode_cmd, cmd_len);
                break;
            case PmSensorCmd::VersionDate:
                Serial.println(F("SDS cmd: version"));
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

    void initReplyData() {
        for (byte i = 0; i < SDS_UNKNOWN; i++) {
            replies[i].sent = false;
            replies[i].received = false;
            replies[i].lastRequest = 0;
            replies[i].lastReply = 0;

        }
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
            for (byte j=0; j<5;j++){
                ret += String(channelSDS._replies[i].data[j],16);
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
            initReplyData();
            if (hardwareWatchdog) {
                PCF = new PCF8574(0x26);
                PCF -> begin(0xFF);
            }
            debug_out(F("SDS011: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::SDS011)) {   //de
            debug_out(F("SDS011: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::SDS011);
        }
        //register display - separate check to allow enable/disable display not only when turning SDS011 on/off
        if (enabled && printOnLCD) scheduler.registerDisplay(SimpleScheduler::SDS011, 1);
    }

    void startSDS() {
        SDS011::SDS_cmd(PmSensorCmd::Start);
        delay(100);
        SDS011::SDS_cmd(PmSensorCmd::ContinuousMode);
        delay(100);
        int pm10 = -1, pm25 = -1;
        unsigned long timeOutCount = millis();
        SDS_waiting_for = SDS_REPLY_HDR;
        while ((millis() - timeOutCount) < 500 && (pm10 == -1 || pm25 == -1)) {
            readSingleSDSPacket(&pm10, &pm25);
            delay(5);
        }
        if (pm10 == -1 || pm25 == -1) {
            debug_out(F("SDS011 not sending data!"), DEBUG_ERROR, 1);
        } else {
            debug_out(F("PM10/2.5:"), DEBUG_ERROR, 1);
            debug_out(String(pm10 / 10.0, 2), DEBUG_ERROR, 1);
            debug_out(String(pm25 / 10.0, 2), DEBUG_ERROR, 1);
            last_value_SDS_P1 = double(pm10 / 10.0);
            last_value_SDS_P2 = double(pm25 / 10.0);
        }
        is_SDS_running = SDS011::SDS_cmd(PmSensorCmd::Stop);

    }

    //reset counters
    void resetReadings() {
        pm10Sum = pm25Sum = readingCount = 0;
    };

    void storeReadings(const int pm10, const int pm25) {
        if (pm10 == -1 || pm25 == -1) return;
        pm10Sum += pm10;
        pm25Sum += pm25;
        readingCount++;
    };

    //store last values pm
    void processReadings() {
        if (readingCount > 0) {
            last_value_SDS_P1 = pm10Sum / readingCount / 10.0;
            last_value_SDS_P2 = pm25Sum / readingCount / 10.0;
        } else {
            last_value_SDS_P1 = last_value_SDS_P2 = -1;
            hwWtdgFailedReadings++;
        }

    };

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
                is_SDS_running = SDS_cmd(PmSensorCmd::Start);
                delay(500);
                is_SDS_running = SDS_cmd(PmSensorCmd::VersionDate);
                resetReadings();
                updateState(POST);
                return 100;
            case POST:
                if (channelSDS.fetchReading(pm10, pm25)) {
                    storeReadings(pm10, pm25);
                }
                if (timeout(STARTUP_TIME)) {
                    processReadings();
                    is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
                    updateState(OFF);
                    return 500;
                }
                return 20;
            case OFF:
                if (hardwareWatchdog && hwWtdgFailedReadings > 0) {
                    Serial.println("HW & failed");
                    Serial.println(PCF!=nullptr);
                    Serial.println(PCF!= nullptr && PCF->isConnected());
                    if (PCF != nullptr && PCF->isConnected()) {
                        PCF->write8(0);
                        Serial.print(F("PCF status: "));
                        Serial.println(PCF->lastError());
                        debug_out(F("SDS not responding hardware restart !!!! "), DEBUG_ERROR);
                        updateState(HARDWARE_RESTART);
                        return 10;
                    }

                }
                if (t < warmupTime + readTime + SDS011_END_TIME)   //aim to finish 2 sec before readingTime
                    updateState(START);
                return 10;
            case HARDWARE_RESTART:
                if (timeout(5*1000)) {
                    if (PCF->isConnected()) {
                        debug_out(F("Starting SDS (power ON)"), DEBUG_ERROR);
                        PCF->write8(0xFF);
                        byte e;
                        if (e=PCF->lastError()) {
                            Serial.print(F("PCF status: "));
                            Serial.println(e);
                            return 1;
                        }
                        hwWtdgFailedReadings = 0;
                        updateState(OFF);
                    }
                }
                return 10;
            case START:
                is_SDS_running = SDS_cmd(PmSensorCmd::Start);
                updateState(WARMUP);
                return 100;
            case WARMUP:
                if (t < readTime + SDS011_END_TIME)
                    updateState(READ);
                return 10;
            case READ:
//                serialSDS.flush();
                resetReadings();
                SDS_waiting_for = SDS_REPLY_HDR;
                updateState(READING);
                return 10;
            case READING:
                if (channelSDS.fetchReading(pm10, pm25)) {
                    storeReadings(pm10, pm25);
                }
                if (timeout(readTime)) {
                    updateState(STOP);
                    return 50;
                }
                return 20;
            case STOP:
                processReadings();
                debug_out(F("SDS011: end of cycle"),DEBUG_MIN_INFO, 1);
                is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
                updateState(AFTER_READING);
                return 100;
            default:
                return 1000;
        }
    }

    void results(String &res) {
        readings++;
        if (last_value_SDS_P2 != -1 && last_value_SDS_P1 != -1) {

            res += Value2Json(F("SDS_P1"),String(last_value_SDS_P1));
            res += Value2Json(F("SDS_P2"),String(last_value_SDS_P2));
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
            debug_out(F("SDS011 reading procedure not finished, not sending data."),DEBUG_MIN_INFO);
            debug_out(String(stateChangeTime),DEBUG_MIN_INFO);
            debug_out(String(sensorState),DEBUG_MIN_INFO);
        }
    }

    void getStatusReport(String &res) {
        if (!enabled) return;
        SDS_cmd(PmSensorCmd::Start);
        SDS_cmd(PmSensorCmd::VersionDate);
        res += FPSTR(EMPTY_ROW);
        res += table_row_from_value(F("SDS011"), F("Status"), String(sensorState), "");
        res += table_row_from_value(F("SDS011"), F("Status change"), String(millis() - stateChangeTime), "");
        res += table_row_from_value(F("SDS011"), F("Version data"), SDS_version_date(), "");
        res += table_row_from_value(F("SDS011"), F("Failed RD"), String(hwWtdgFailedReadings), "");
        float fr = 0;
        if (channelSDS.toalPacketCnt()) fr = channelSDS.checksumErrCnt() / (float) channelSDS.toalPacketCnt() * 100.0;
        res += table_row_from_value(F("SDS011"), F("Checksum failures"),
                                    String(fr) + F("% ") + String(channelSDS.checksumErrCnt()) + F("/") + String(channelSDS.toalPacketCnt()), "");
    }

//    void popCmd(PmSensorCmd &t) {
//        t = cmdQ[0];
//        cmdIdx--;
//        for (byte i = 0; i < cmdIdx; i++) {
//            cmdQ[i] = cmdQ[i + 1];
//        }
//    }
//
//    void processCmdQueue() {
//        if (cmdIdx) {
//            if (lastCmdSent != PmSensorCmd::None ) {
//                PmSensorCmd cmd;
//                popCmd(cmd);
//
//            }
//        }
//    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::STOP:
                is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
                return 0;
            case SimpleScheduler::INIT:
                readings = failedReadings = 0;
                return processState();
            case SimpleScheduler::RUN:
                channelSDS.process();
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
        String version_date = "";

        debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(DBG_TXT_SDS011_VERSION_DATE), DEBUG_MED_INFO, 1);
        if(!replies[SDS_FW_VER].received) {
            is_SDS_running = SDS_cmd(PmSensorCmd::Start);
            is_SDS_running = SDS_cmd(PmSensorCmd::VersionDate);
        }

        if (replies[SDS_FW_VER].received) {
            char reply[30];
            byte *buff = replies[SDS_FW_VER].data;
            char fmt[20];
            String a = F("%i-%i-%i(%02X%02X)");
            a.toCharArray(fmt, 20);
            sprintf_P(reply, fmt, buff[0], buff[1], buff[2], buff[3], buff[4]);
            version_date = String(reply);
        } else {
            version_date = F("n/a");
        }
        return version_date;
    }


/*****************************************************************
 * read SDS011 sensor values                                     *
 *****************************************************************/
    void sensorSDS(String &s) {
        int pm25_serial, pm10_serial;
        if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS) &&
            msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
            if (is_SDS_running) {
                is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
            }
        } else {
            if (!is_SDS_running) {
                is_SDS_running = SDS_cmd(PmSensorCmd::Start);
                SDS_waiting_for = SDS_REPLY_HDR;
            }

            while (serialSDS.available() >= SDS_waiting_for) {
                readSingleSDSPacket(&pm10_serial, &pm25_serial);
                if (pm10_serial > -1 && pm25_serial > -1) {

                    if (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS)) {
                        sds_pm10_sum += pm10_serial;
                        sds_pm25_sum += pm25_serial;
                        UPDATE_MIN_MAX(sds_pm10_min, sds_pm10_max, pm10_serial);
                        UPDATE_MIN_MAX(sds_pm25_min, sds_pm25_max, pm25_serial);
                        sds_val_count++;
                    }
                }
            }

            if (send_now) {
                last_value_SDS_P1 = -1;
                last_value_SDS_P2 = -1;
                if (sds_val_count > 2) {
                    sds_pm10_sum = sds_pm10_sum - sds_pm10_min - sds_pm10_max;
                    sds_pm25_sum = sds_pm25_sum - sds_pm25_min - sds_pm25_max;
                    sds_val_count = sds_val_count - 2;
                }
                if (sds_val_count > 0) {
                    last_value_SDS_P1 = float(sds_pm10_sum) / (sds_val_count * 10.0f);
                    last_value_SDS_P2 = float(sds_pm25_sum) / (sds_val_count * 10.0f);
//            debug_outln_info(FPSTR(DBG_TXT_SEP));
                    if (sds_val_count < 3) {
                        SDS_error_count++;
                    }
                } else {
                    SDS_error_count++;
                }
                s += Value2Json(F("SDS_P1"), Float2String(last_value_SDS_P1));
                s += Value2Json(F("SDS_P2"), Float2String(last_value_SDS_P2));
                sds_pm10_sum = 0;
                sds_pm25_sum = 0;
                sds_val_count = 0;
                sds_pm10_max = 0;
                sds_pm10_min = 20000;
                sds_pm25_max = 0;
                sds_pm25_min = 20000;
                if ((cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {

                    if (is_SDS_running) {
                        is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
                    }
                }
            }
        }

    }
}