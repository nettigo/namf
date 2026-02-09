//
// Created by viciu on 9/13/23.
//
#ifdef NAM_LORAWAN

#include "lorawan.h"
#include "stdio.h"
#include <Preferences.h>
#include <ArduinoJson.h>
#include "helpers.h"
#include "radio/sx126x/radio.cpp"

namespace LoRaWan {
    Preferences appConfig;
    hw_config hwConfig;
    ModuleState state = STATE_OK;

    uint8_t nodeDeviceEUI[8];
    uint8_t nodeAppEUI[8];
    uint8_t nodeAppKey[16];
    uint8_t NetworkSessionKey[16];
    uint8_t AppSessionKey[16];
    uint32_t nodeDevAddr;
    uint32_t downlinkRcv = 0;
    uint32_t uplinkSend = 0;


    uint32_t lastAirTime = 0;

    unsigned long lastSend = 0;
    constexpr byte airTimeTableSize = 10;
    uint32_t airTimeData[airTimeTableSize] = { };
    byte airTimeIndex = 0;
    byte airTimeSamples = 0;
    bool OTAA_request = true;

    CayenneLPP lpp(20);

    lmh_error_status lastSendStatus = LMH_SUCCESS;
    // ESP32 - SX126x pin configuration
    int PIN_LORA_RESET = 12;     // LORA RESET
    int PIN_LORA_NSS = 8;     // LORA SPI CS
    int PIN_LORA_SCLK = 9;     // LORA SPI CLK
    int PIN_LORA_MISO = 11;     // LORA SPI MISO
    int PIN_LORA_DIO_1 = 14; // LORA DIO_1
    int PIN_LORA_BUSY = 13;     // LORA SPI BUSY
    int PIN_LORA_MOSI = 10;     // LORA SPI MOSI
    int RADIO_TXEN = -1;     // LORA ANTENNA TX ENABLE
    int RADIO_RXEN = -1;     // LORA ANTENNA RX ENABLE


    // APP_TIMER_DEF(lora_tx_timer_id);                                              ///< LoRa tranfer timer instance.
    TimerEvent_t appTimer;                                                          ///< LoRa tranfer timer instance.
    uint8_t m_lora_app_data_buffer[LORAWAN_APP_DATA_BUFF_SIZE];              ///< Lora user application data buffer.
    lmh_app_data_t m_lora_app_data = {m_lora_app_data_buffer, 0, 0, 0,
                                      0}; ///< Lora user application data structure.

    static bool load_lorawan_session(lmh_session_t *out);
    static bool save_lorawan_session(const lmh_session_t *in);
    static void persist_lorawan_session(void);
    static Preferences lmhPrefs;
    static const char kPrefNs[] = "lorawan";
    static const char kPrefKey[] = "session";

    static bool getStoredKeys(void){
        if (!appConfig.isKey("ntwsk") || !appConfig.isKey("appsk") || !appConfig.isKey("devaddr"))
            return false;
        if (!appConfig.getBytes("ntwsk", NetworkSessionKey, 16))
            return false;
        if (!appConfig.getBytes("appsk", AppSessionKey, 16))
            return false;
        nodeDevAddr = appConfig.getUInt("devaddr");
        return true;
    }

    void dump_lmh_session(const lmh_session_t &s)
{
	debugOutLn("LoRaWAN session:", DEBUG_MED_INFO);

	String line;
	line.reserve(128);

	line = "magic=0x";
	line += String(s.magic, HEX);
	line += " version=";
	line += String(s.version);
	line += " region=";
	line += String(s.region);
	debugOutLn(line, DEBUG_MED_INFO);

	line = "class=";
	line += String(s.device_class);
	line += " public=";
	line += String(s.public_network);
	line += " adr=";
	line += String(s.adr_enabled);
	debugOutLn(line, DEBUG_MED_INFO);

	line = "net_id=0x";
	line += String(s.net_id, HEX);
	line += " dev_addr=0x";
	line += String(s.dev_addr, HEX);
	debugOutLn(line, DEBUG_MED_INFO);

	debugOut("nwk_skey=", DEBUG_MED_INFO);
	for (int i = 0; i < 16; i++)
	{
		if (i) debugOut("-", DEBUG_MED_INFO);
		uint8_t b = s.nwk_skey[i];
		if (b < 0x10) debugOut("0", DEBUG_MED_INFO);
		debugOut(String(b, HEX), DEBUG_MED_INFO);
	}
	debugOutLn("", DEBUG_MED_INFO);

	debugOut("app_skey=", DEBUG_MED_INFO);
	for (int i = 0; i < 16; i++)
	{
		if (i) debugOut("-", DEBUG_MED_INFO);
		uint8_t b = s.app_skey[i];
		if (b < 0x10) debugOut("0", DEBUG_MED_INFO);
		debugOut(String(b, HEX), DEBUG_MED_INFO);
	}
	debugOutLn("", DEBUG_MED_INFO);

	line = "fcnt_up=";
	line += String(s.fcnt_up);
	line += " fcnt_down=";
	line += String(s.fcnt_down);
	debugOutLn(line, DEBUG_MED_INFO);

	line = "tx_power=";
	line += String(s.tx_power);
	line += " data_rate=";
	line += String(s.data_rate);
	line += " nb_rep=";
	line += String(s.nb_rep);
	debugOutLn(line, DEBUG_MED_INFO);

	debugOut("channels_mask=", DEBUG_MED_INFO);
	for (int i = 0; i < 6; i++)
	{
		if (i) debugOut(" ", DEBUG_MED_INFO);
		uint16_t v = s.channels_mask[i];
		if (v < 0x1000) debugOut("0", DEBUG_MED_INFO);
		if (v < 0x100) debugOut("0", DEBUG_MED_INFO);
		if (v < 0x10) debugOut("0", DEBUG_MED_INFO);
		debugOut(String(v, HEX), DEBUG_MED_INFO);
	}
	debugOutLn("", DEBUG_MED_INFO);

	debugOut("channels_default_mask=", DEBUG_MED_INFO);
	for (int i = 0; i < 6; i++)
	{
		if (i) debugOut(" ", DEBUG_MED_INFO);
		uint16_t v = s.channels_default_mask[i];
		if (v < 0x1000) debugOut("0", DEBUG_MED_INFO);
		if (v < 0x100) debugOut("0", DEBUG_MED_INFO);
		if (v < 0x10) debugOut("0", DEBUG_MED_INFO);
		debugOut(String(v, HEX), DEBUG_MED_INFO);
	}
	debugOutLn("", DEBUG_MED_INFO);

	line = "rx2: freq=";
	line += String(s.rx2.Frequency);
	line += " dr=";
	line += String(s.rx2.Datarate);
	debugOutLn(line, DEBUG_MED_INFO);

	line = "rx2_default: freq=";
	line += String(s.rx2_default.Frequency);
	line += " dr=";
	line += String(s.rx2_default.Datarate);
	debugOutLn(line, DEBUG_MED_INFO);

	line = "crc32=0x";
	line += String(s.crc32, HEX);
	debugOutLn(line, DEBUG_MED_INFO);
}


    void getNetworkSessionKey(String &out){
        out.reserve(49);
        char tmp[3];
        out = F("");
        for (byte i=0; i < 16; i++ ){
            sprintf(tmp,"%02X", NetworkSessionKey[i]);
            out.concat(String(tmp));
//            out.concat(F(","));
        }

    }

    void getAppSessionKey(String &out) {
        out.reserve(49);
        char tmp[3];
        out = F("");
        for (byte i = 0; i < 16; i++) {
            sprintf(tmp, "%02X", AppSessionKey[i]);
            out.concat(String(tmp));
//            out.concat(F(","));
        }

    }

    void getStateDescription(String &out) {
        out = String(F("Joined"));
        if (state == JOIN_FAILED) out = String(F("Join failed!"));
        if (state == ERR_APP_EUI) out = String(F("App EUI not parsed!"));
        if (state == ERR_APP_KEY) out = String(F("App key not parsed!"));
        if (state == ERR_DEV_EUI) out = String(F("Dev EUI not parsed!"));
        if (state == STATE_OK) out = String(F("Ready to join"));
    }

    //print session keys on Serial
    static void printKeys(byte level){
        debug_out(F("Network session key: "), level, false);
        String tmp1;
        tmp1.reserve(64);
        getNetworkSessionKey(tmp1);
        debug_out(String(tmp1), level);
        tmp1.clear();
        char tmp[3];
        debug_out(F("App session key: "), level, false);
        for (byte i=0; i < 16; i++ ){
            sprintf(tmp, "%02X", AppSessionKey[i]);
            tmp1.concat(tmp);
//            tmp1.concat(F(","));
        }
        debug_out(String(tmp1), level);
        debug_out(F("Node addr: "), level, false);
        debug_out(String(lmh_getDevAddr(), HEX), level);
    }


    //store LoRaWAN keys in ESP32 preferences
    static boolean storeKeys() {
        if ( !appConfig.putBytes("ntwsk", NetworkSessionKey, 16)){
            appConfig.remove("ntwsk");
            return false;
        };
        if (!appConfig.putBytes("appsk", AppSessionKey, 16)) {
            appConfig.remove("appsk");
            return false;
        };
        if (appConfig.putUInt("devaddr",lmh_getDevAddr()) == 0) {
            appConfig.remove("devaddr");
            return false;
        }
        return true;
    }

    /**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
    lmh_param_t lora_param_init = {LORAWAN_ADR_ON, LORAWAN_DEFAULT_DATARATE, LORAWAN_PUBLIC_NETWORK,
                                   JOINREQ_NBTRIALS, LORAWAN_DEFAULT_TX_POWER, LORAWAN_DUTYCYCLE_OFF};




/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
    lmh_callback_t lora_callbacks = {BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
                                     lorawan_rx_handler, lorawan_has_joined_handler,
                                     lorawan_confirm_class_handler, lorawan_join_failed_handler};

    //parse string containing hex values (no whitespace, no separators, no 0x) like 'AB11EE'
    //stores bytes in array. return true if everything was parsed
    boolean parseStringToArray(const String src, byte dest[], const byte size) {
        //maximum string size
        const byte BUFF_SIZE = 60;
        debug_out(src, DEBUG_ERROR);
        if (src.length() > BUFF_SIZE) {
            return false;
        }

        char *end = nullptr;
        char *p = nullptr;
        char buff[BUFF_SIZE + 1];
        src.toCharArray(buff, BUFF_SIZE);
        p = buff;
        char n[3];
        n[2] = 0; //null termination
        byte idx = 0;
        while (strlen(p) > 1) {
            if (idx >= size) return false;  //dest array is too small
            strncpy(n, p, 2);
            dest[idx++] = (byte) strtoul(n, nullptr, 16);
            p += 2;
        }
        return true;
    }



    void setup() {
        hwConfig.CHIP_TYPE = SX1262_CHIP;          // Example uses an eByte E22 module with an SX1262
        hwConfig.PIN_LORA_RESET = PIN_LORA_RESET; // LORA RESET
        hwConfig.PIN_LORA_NSS = PIN_LORA_NSS;      // LORA SPI CS
        hwConfig.PIN_LORA_SCLK = PIN_LORA_SCLK;      // LORA SPI CLK
        hwConfig.PIN_LORA_MISO = PIN_LORA_MISO;      // LORA SPI MISO
        hwConfig.PIN_LORA_DIO_1 = PIN_LORA_DIO_1; // LORA DIO_1
        hwConfig.PIN_LORA_BUSY = PIN_LORA_BUSY;      // LORA SPI BUSY
        hwConfig.PIN_LORA_MOSI = PIN_LORA_MOSI;      // LORA SPI MOSI
        hwConfig.RADIO_TXEN = RADIO_TXEN;          // LORA ANTENNA TX ENABLE
        hwConfig.RADIO_RXEN = RADIO_RXEN;          // LORA ANTENNA RX ENABLE
        hwConfig.USE_DIO2_ANT_SWITCH = true;      // Example uses an CircuitRocks Alora RFM1262 which uses DIO2 pins as antenna control
        hwConfig.USE_DIO3_TCXO = true;              // Example uses an CircuitRocks Alora RFM1262 which uses DIO3 to control oscillator voltage
        hwConfig.USE_DIO3_ANT_SWITCH = false;      // Only Insight ISP4520 module uses DIO3 as antenna control

        uint32_t err_code = lora_hardware_init(hwConfig);
        if (err_code != 0)
        {
            Serial.printf("lora_hardware_init failed - %d\n", err_code);
        }

        // Initialize Scheduler and timer (Must be after lora_hardware_init)
        err_code = timers_init();
        if (err_code != 0)
        {
            Serial.printf("timers_init failed - %d\n", err_code);
        }
        if (!cfg::lw_en) {
            return;
        }
        appConfig.begin("lorawan");

        // Setup the EUIs and Keys
        if (!parseStringToArray(cfg::lw_d_eui, nodeDeviceEUI, 8)) {
            debug_out(F("Failed to parse node Device EUI!"), DEBUG_ERROR);
            LoRaWan::state = ERR_DEV_EUI;
            return;
        };
        if (!parseStringToArray(cfg::lw_a_eui, nodeAppEUI, 8)) {
            debug_out(F("Failed to parse node App EUI!"), DEBUG_ERROR);
            LoRaWan::state = ERR_APP_EUI;
            return;
        };
        if (!parseStringToArray(cfg::lw_app_key, nodeAppKey, 16)) {
            debug_out(F("Failed to parse node App Key!"), DEBUG_ERROR);
            LoRaWan::state = ERR_APP_KEY;
            return;
        };
        lmh_setDevEui(nodeDeviceEUI);
        lmh_setAppEui(nodeAppEUI);
        lmh_setAppKey(nodeAppKey);
//        if (getStoredKeys()){
//            OTAA_request = false;
//            debug_out(F("LoRaWAN Keys restored from storage"),DEBUG_MED_INFO);
//            lmh_setAppSKey(AppSessionKey);
//            lmh_setNwkSKey(NetworkSessionKey);
//            lmh_setDevAddr(nodeDevAddr);
//            printKeys(DEBUG_MED_INFO);
//        }

//        lmh_setNwkSKey(nodeNwsKey);
//        lmh_setAppSKey(nodeAppsKey);
//        lmh_setDevAddr(nodeDevAddr);

        // Initialize LoRaWan
//        if (OTAA_request) { debug_out(F("OTAA!!!"), DEBUG_MED_INFO); } else { debug_out(F("NO OTAA, keys restored"), DEBUG_MED_INFO); }
        err_code = lmh_init(&lora_callbacks, lora_param_init, OTAA_request, CLASS_A, LORAMAC_REGION_EU868);
        if (err_code != 0)
        {
            debug_out(F("lmh_init failed - "), DEBUG_ERROR, false);
            debug_out(String(err_code), DEBUG_ERROR);
        }
        if (!lmh_setSubBandChannels(1))
        {
            debug_out(F("lmh_setSubBandChannels failed. Wrong sub band requested?"), DEBUG_ERROR);
        }

        // Restore session (if available), otherwise start join
        lmh_session_t session;
        bool restored = false;

        if (load_lorawan_session(&session) && lmh_session_is_valid(&session))
        {
            if (lmh_session_apply(&session) == LMH_SUCCESS)
            {
                debugOutLn("LoRaWAN session restored from flash", DEBUG_MIN_INFO);
                dump_lmh_session(session);
                restored = true;
                lmh_class_request(CLASS_A);
                state = STATE_JOINED;
            }
        }
        if (!restored)
        {
            // Join procedure will be at first sending data
        }


        // we will join before sending data
//        lmh_join();

    }



    static bool load_lorawan_session(lmh_session_t *out)
    {
        if (out == NULL)
        {
            return false;
        }

        if (!lmhPrefs.begin(kPrefNs, true))
        {
            return false;
        }

        size_t len = lmhPrefs.getBytesLength(kPrefKey);
        if (len != sizeof(lmh_session_t))
        {
            lmhPrefs.end();
            return false;
        }

        size_t read = lmhPrefs.getBytes(kPrefKey, out, sizeof(lmh_session_t));
        lmhPrefs.end();
        return (read == sizeof(lmh_session_t));
    }

    static bool save_lorawan_session(const lmh_session_t *in)
    {
        if (in == NULL)
        {
            return false;
        }

        if (!lmhPrefs.begin(kPrefNs, false))
        {
            return false;
        }

        size_t written = lmhPrefs.putBytes(kPrefKey, in, sizeof(lmh_session_t));
        lmhPrefs.end();
        return (written == sizeof(lmh_session_t));
    }

    static void persist_lorawan_session(void)
    {
        lmh_session_t session;
        if (lmh_session_get(&session) != LMH_SUCCESS)
        {
            return;
        }
        save_lorawan_session(&session);
    }

    void lorawan_join_failed_handler(void)
    {
        debug_out("OVER_THE_AIR_ACTIVATION failed!", DEBUG_ERROR);
        state = JOIN_FAILED;
    }

/**@brief LoRa function for handling HasJoined event.
*/
    void lorawan_has_joined_handler(void)
    {
#if (OVER_THE_AIR_ACTIVATION != 0)
        Serial.println("Network Joined");
#else
        Serial.println("OVER_THE_AIR_ACTIVATION != 0");

#endif
        debug_out("LORA WAN JOINED!", DEBUG_ERROR);
        lmh_class_request(CLASS_A);
        state = STATE_JOINED;
        lmh_getAppSkey(AppSessionKey);
        lmh_getNwSkey(NetworkSessionKey);
        storeKeys();
        printKeys(DEBUG_MED_INFO);

        // need to restore partial data from NVS...
        /*
        MibRequestConfirm_t mibReq;

        mibReq.Type = MIB_UPLINK_COUNTER;
        uplinkSend = appConfig.getULong("uplinks", 0);
        mibReq.Param.UpLinkCounter = uplinkSend;
        LoRaMacMibSetRequestConfirm(&mibReq);
        debug_out("LoRaWAN Uplinks ustawione na: ", DEBUG_MED_INFO, 1);
        debug_out(String(uplinkSend), DEBUG_MED_INFO);

        mibReq.Type = MIB_DOWNLINK_COUNTER;
        downlinkRcv = appConfig.getULong("dwnlinks", 0);
        mibReq.Param.DownLinkCounter = downlinkRcv;
        LoRaMacMibSetRequestConfirm(&mibReq);
        debug_out("LoRaWAN Downlinks ustawione na: ", DEBUG_MED_INFO, 1);
        debug_out(String(downlinkRcv), DEBUG_MED_INFO);
        */

    }

/**@brief Function for handling LoRaWan received data from Gateway

   @param[in] app_data  Pointer to rx data
*/
    void lorawan_rx_handler(lmh_app_data_t *app_data)
    {
        char buff[200];
        snprintf(buff,200,"LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d\n",app_data->port, app_data->buffsize, app_data->rssi, app_data->snr );
        debug_out(String(buff),DEBUG_MIN_INFO);

        switch (app_data->port)
        {
            case 3:
                // Port 3 switches the class
                if (app_data->buffsize == 1)
                {
                    switch (app_data->buffer[0])
                    {
                        case 0:
                            lmh_class_request(CLASS_A);
                            break;

                        case 1:
                            lmh_class_request(CLASS_B);
                            break;

                        case 2:
                            lmh_class_request(CLASS_C);
                            break;

                        default:
                            break;
                    }
                }
                break;

            case LORAWAN_APP_PORT:
                // YOUR_JOB: Take action on received data
                break;

            default:
                break;
        }
    }

    void lorawan_confirm_class_handler(DeviceClass_t Class)
    {
        Serial.printf("switch to class %c done\n", "ABC"[Class]);

        // Informs the server that switch has occurred ASAP
        m_lora_app_data.buffsize = 0;
        m_lora_app_data.port = LORAWAN_APP_PORT;
        lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
    }

    //store airTime for average calculation
    void storeAirTime(uint32_t t) {
        lastAirTime = t;
        airTimeData[airTimeIndex++] = t;
        if (airTimeIndex >= airTimeSamples) airTimeIndex = 0;
        if (airTimeSamples < airTimeTableSize) airTimeSamples++;
    }

    u32_t averageAirTime () {
        if (airTimeSamples == 0 ) { return 0; }
        u64_t sum = 0;
        //we can sum all elements since not used are initialized as 0
        for (u32_t t : airTimeData)
            sum += t;
        return sum/airTimeSamples;
    }


    void sendLoRaWAN(String data)
    {
        static byte count = 0;
        float temp = 20;
        float pm10 = 20;
        float pm25 = 20;
        float h = -0;
        static float sum_temp = 0;
        static float sum_pm10 = 0;
        static float sum_pm25 = 0;
        static float sum_h = 0;
        DynamicJsonBuffer jsonBuffer;

        if (state == JOIN_FAILED ) {
            debug_out(F("Not joined to LoRaWAN!"), DEBUG_ERROR);
            return;
        };

        JsonObject &json = jsonBuffer.parseObject(data);
        if (!json.success()) {
            debug_out(F("Internal JSON (data) parsing failure!"), DEBUG_ERROR);
            return;
        }
        if (json.containsKey("sensordatavalues")){

            JsonArray& items = json["sensordatavalues"];
            for (auto obj : items) {
                String k = obj.as<JsonObject>()["value_type"];

//                debug_out(obj.as<JsonObject>()["value"], DEBUG_ERROR);
                if (k.equals(String(F("SDS_P1")))) {
                    pm10 = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String(F("SDS_P2")))) {
                    pm25 = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String(F("BME280_temperature")))) {
                    temp = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String(F("BME280_humidity")))) {
                    h = obj.as<JsonObject>().get<float>("value");
                }
            }

        }
        count++;
        debug_out("LORAWAN step: ",DEBUG_MIN_INFO, 0);
        debug_out(String(count),DEBUG_MIN_INFO);
        if (count == 4 && state == STATE_OK) {  // we didn't join yet and have correct config
            debug_out("LORAWAN join request",DEBUG_MIN_INFO);
            lmh_join();
        }
        sum_h += h;
        sum_temp += temp;
        sum_pm10 += pm10;
        sum_pm25 += pm25;
        //every 5 measurements send data to LoRaWAN
        if (count >= 5) {
            debug_out("LORAWAN send!.",DEBUG_MIN_INFO);
            lpp.reset();
            lpp.addTemperature(1, sum_pm10/5);
            lpp.addTemperature(2, sum_pm25/5);
            lpp.addTemperature(3, sum_temp/5);
            lpp.addRelativeHumidity(1, sum_h/5);

            //reset counters/sum vars
            count = 0;
            sum_pm25 = sum_pm10 = sum_temp = sum_h = 0;



            m_lora_app_data.port = LORAWAN_APP_PORT;
            m_lora_app_data.buffer = lpp.getBuffer();
            m_lora_app_data.buffsize = lpp.getSize();

            lastSendStatus = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
            debugOut("lmh_send result ", DEBUG_MIN_INFO);
            debugOutLn(String(lastSendStatus), DEBUG_MIN_INFO);
            if (lastSendStatus == LMH_SUCCESS) {
                storeAirTime(RadioTimeOnAir(MODEM_LORA, lpp.getSize()));
            } else { lastAirTime = 0; }
            //as for now we don't have support for persisting all required data, need to join before sending
            //so storing counters is not needed (yet)
            /*
            MibRequestConfirm_t mibReq;
            char tmp[100];
            mibReq.Type = MIB_UPLINK_COUNTER;
            uplinkSend = appConfig.getULong("uplinks", 0);
            if (LoRaMacMibGetRequestConfirm(&mibReq) != LORAMAC_STATUS_SERVICE_UNKNOWN) {
                if (uplinkSend != mibReq.Param.UpLinkCounter) {
                    if (4==appConfig.putULong("uplinks", mibReq.Param.UpLinkCounter)){
                        sprintf(tmp, "Uplink counter: %u\n", mibReq.Param.UpLinkCounter);
                        debug_out(tmp, DEBUG_ERROR);
                        sprintf(tmp, "Uplink counter from prefs: %u\n", appConfig.getULong("uplinks"));
                        debug_out(tmp, DEBUG_ERROR);
                    };

                } else { debug_out("No uplink count change", DEBUG_ERROR);}

            } else {
                debug_out("Failed MibGetRequest",DEBUG_ERROR);
            }

            mibReq.Type = MIB_DOWNLINK_COUNTER;
            downlinkRcv = appConfig.getULong("dwnlinks", 0);
            LoRaMacMibGetRequestConfirm(&mibReq);
            if (downlinkRcv != mibReq.Param.DownLinkCounter)
                if (4==appConfig.putULong("dwnlinks", mibReq.Param.DownLinkCounter)){
                    sprintf(tmp, "Downlink counter: %u\n", mibReq.Param.DownLinkCounter);
                    debug_out(tmp, DEBUG_ERROR);
                    sprintf(tmp, "Downlink counter from prefs: %u\n", appConfig.getULong("dwnlinks"));
                    debug_out(tmp, DEBUG_ERROR);
                };
                */
        }
    persist_lorawan_session();
    }

/**@brief Function for handling a LoRa tx timer timeout event.
*/
    void tx_lora_periodic_handler(void)
    {
//        TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
//        TimerStart(&appTimer);
//        Serial.println("Sending frame");
//        send_lora_frame();
    }

    /**@brief Function for the Timer initialization.

   @details Initializes the timer module. This creates and starts application timers.
*/
    uint32_t timers_init(void)
    {
        appTimer.timerNum = 3;
        TimerInit(&appTimer, tx_lora_periodic_handler);
        return 0;
    }
}
#endif