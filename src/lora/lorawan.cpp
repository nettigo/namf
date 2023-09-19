//
// Created by viciu on 9/13/23.
//
#include "lorawan.h"
#include "ttn_key.h"
#include <ArduinoJson.h>
#include "helpers.h"

namespace LoRaWan {
    hw_config hwConfig;
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

/**@brief Structure containing LoRaWan parameters, needed for lmh_init()
*/
    lmh_param_t lora_param_init = {LORAWAN_ADR_ON, LORAWAN_DEFAULT_DATARATE, LORAWAN_PUBLIC_NETWORK,
                                   JOINREQ_NBTRIALS, LORAWAN_DEFAULT_TX_POWER, LORAWAN_DUTYCYCLE_OFF};

/**@brief Structure containing LoRaWan callback functions, needed for lmh_init()
*/
    lmh_callback_t lora_callbacks = {BoardGetBatteryLevel, BoardGetUniqueId, BoardGetRandomSeed,
                                     lorawan_rx_handler, lorawan_has_joined_handler,
                                     lorawan_confirm_class_handler, lorawan_join_failed_handler};


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

        // Setup the EUIs and Keys
        lmh_setDevEui(nodeDeviceEUI);
        lmh_setAppEui(nodeAppEUI);
        lmh_setAppKey(nodeAppKey);
//        lmh_setNwkSKey(nodeNwsKey);
//        lmh_setAppSKey(nodeAppsKey);
//        lmh_setDevAddr(nodeDevAddr);

        // Initialize LoRaWan
        err_code = lmh_init(&lora_callbacks, lora_param_init, true, CLASS_A, LORAMAC_REGION_EU868);
        if (err_code != 0)
        {
            Serial.printf("lmh_init failed - %d\n", err_code);
        }
        if (!lmh_setSubBandChannels(1))
        {
            Serial.println("lmh_setSubBandChannels failed. Wrong sub band requested?");
        }

        // Start Join procedure
        lmh_join();

    }

    void lorawan_join_failed_handler(void)
    {
        Serial.println("OVER_THE_AIR_ACTIVATION failed!");
        Serial.println("Check your EUI's and Keys's!");
        Serial.println("Check if a Gateway is in range!");
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

        TimerSetValue(&appTimer, LORAWAN_APP_TX_DUTYCYCLE);
        TimerStart(&appTimer);
    }

/**@brief Function for handling LoRaWan received data from Gateway

   @param[in] app_data  Pointer to rx data
*/
    void lorawan_rx_handler(lmh_app_data_t *app_data)
    {
        Serial.printf("LoRa Packet received on port %d, size:%d, rssi:%d, snr:%d\n",
                      app_data->port, app_data->buffsize, app_data->rssi, app_data->snr);

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

    CayenneLPP lpp(20);

    void send_lora_frame(String data)
    {
        static float temp = 20;
        float pm10 = 20;
        float pm25 = 20;
        float h = -0;
        DynamicJsonBuffer jsonBuffer;

        JsonObject &json = jsonBuffer.parseObject(data);
        if (!json.success()) {
            debug_out(F("Internal JSON (data) parsing failure!"), DEBUG_ERROR);
            return;
        }
        if (json.containsKey("sensordatavalues")){

            JsonArray& items = json["sensordatavalues"];
            for (auto obj : items) {
                String k = obj.as<JsonObject>()["value_type"];

                debug_out("JSON KEY: ", DEBUG_ERROR, 0);
                debug_out(k, DEBUG_ERROR);
                debug_out("JSON VAL: ", DEBUG_ERROR, 0);
                debug_out(obj.as<JsonObject>()["value"], DEBUG_ERROR);
                if (k.equals(String("SDS_P1"))) {
                    pm10 = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String("SDS_P2"))) {
                    pm25 = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String("BME280_temperature"))) {
                    temp = obj.as<JsonObject>().get<float>("value");
                }
                if (k.equals(String("BME280_humidity"))) {
                    h = obj.as<JsonObject>().get<float>("value");
                }
            }

        }
        if (lmh_join_status_get() != LMH_SET)
        {
            //Not joined, try again later
            Serial.println("Did not join network, skip sending frame");
            return;
        }
        lpp.reset();
        lpp.addTemperature(1,pm10);
        lpp.addTemperature(2,pm25);
        lpp.addTemperature(3,temp);
        lpp.addRelativeHumidity(1,h);

        m_lora_app_data.port = LORAWAN_APP_PORT;
        m_lora_app_data.buffer = lpp.getBuffer();
        m_lora_app_data.buffsize = lpp.getSize();

        lmh_error_status error = lmh_send(&m_lora_app_data, LMH_UNCONFIRMED_MSG);
        Serial.printf("lmh_send result %d\n", error);
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