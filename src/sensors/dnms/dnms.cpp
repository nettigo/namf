//
// Created by viciu on 2/8/25.
//
#include "dnms.h"

namespace DNMS {
    const char KEY[] PROGMEM = "DNMS";
    bool enabled = false;
    bool printOnLCD = false;
    unsigned long lastStatChange = 0;

    float last_value_dnms_laeq = -1.0;
    float last_value_dnms_la_min = -1.0;
    float last_value_dnms_la_max = -1.0;

    constexpr uint8_t I2C_ADDRESS = 0x55;
    constexpr uint8_t MAX_VERSION_LEN = 18;
    constexpr uint8_t DNMS_CRC8_INIT = 0xFF;
    constexpr uint8_t DNMS_CRC8_LEN = 1;
    constexpr uint8_t DNMS_CRC8_POLYNOMIAL = 0x31;
    constexpr uint8_t DNMS_STATUS_OK = 0;
    constexpr int8_t DNMS_STATUS_FAIL = -1;
    constexpr uint8_t WORD_SIZE = 2;
    constexpr uint8_t COMMAND_SIZE = 2;
    constexpr uint8_t MAX_BUFFER_WORDS = 32;

#define DNMS_NUM_WORDS(x)               (sizeof(x) / WORD_SIZE)


#define DNMS_CMD_RESET                  0x0001
constexpr uint8_t DNMS_CMD_READ_VERSION = 0x0002;
#define DNMS_CMD_CALCULATE_LEQ          0x0003
#define DNMS_CMD_READ_DATA_READY        0x0004
#define DNMS_CMD_READ_LEQ               0x0005


    struct dnms_measurements {
        float leq_a;
        float leq_a_min;
        float leq_a_max;
    };


    typedef enum {
        UNKNOWN,
        FAILED,
        INITIALIZED,
        READ,
        WAITING,
        READING_AVAILABLE,
        READY_TO_SEND
    } DNMSState;
    DNMSState state = UNKNOWN;

    int16_t dnms_i2c_write_cmd(uint8_t address, uint16_t command);
    uint16_t dnms_fill_cmd_send_buf(uint8_t * buf, uint16_t cmd, const uint16_t *args, uint8_t num_args);
    int8_t dnms_i2c_write(uint8_t address, const uint8_t *data, uint16_t count);
    int16_t dnms_reset();
    bool init();
    int16_t dnms_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words);
    int16_t dnms_read_leq(struct dnms_measurements *leq);

    //for internal use - register/deregister display
    void registerDisplay() {
        if (enabled && printOnLCD)
            scheduler.registerDisplay(SimpleScheduler::DNMS, 1);  // one screen
        else
            scheduler.registerDisplay(SimpleScheduler::DNMS, 0);  // disable
    }

    JsonDocument parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::DNMS);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::DNMS);
        JsonDocument ret;
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        registerDisplay();
        return ret;
    };

    void resultsAsHTML(String &page_content){
        if (!enabled) return;
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(KEY),F("LAeq"),
                                                 String(last_value_dnms_laeq), F("dB")));
        page_content.concat(table_row_from_value(FPSTR(KEY),F("LA min"),
                                                 check_display_value(last_value_dnms_la_min, -1, 1, 0), F("dB")));
        page_content.concat(table_row_from_value(FPSTR(KEY),F("LA max"),
                                                 check_display_value(last_value_dnms_la_max, -1, 1, 0), F("dB")));
    }


    void updateState (DNMSState newState) {
        state = newState;
        lastStatChange = millis();
    }

    bool stateTimeout(unsigned long timeout) {
        if (millis() - lastStatChange > timeout)
            return true;
        return false;
    }

    unsigned long processState() {
        switch (state) {
            case UNKNOWN:
                init();
                return 1000;
            case FAILED:
                return 0;
            case INITIALIZED:
                if (time2Measure() < millis() - 10000) {
                    state = READ;
                    return 100;
                }
                return 500;
            case READ:
                dnms_i2c_write_cmd(I2C_ADDRESS, DNMS_CMD_CALCULATE_LEQ);
                updateState(WAITING);
                return 30;
            case WAITING:
                uint16_t data_ready;
                if (
                        dnms_i2c_read_cmd(I2C_ADDRESS, DNMS_CMD_READ_DATA_READY, &data_ready,
                                          DNMS_NUM_WORDS(data_ready)) == 0 &&
                        data_ready != 0
                        ) {
                    updateState(READING_AVAILABLE);
                    return 10;
                }
                if (stateTimeout(10000)) {
                    updateState(UNKNOWN);
                    debug_out(F("DNMS did not answer for 10 seconds"), DEBUG_ERROR);
                    return 1000;
                }
                return 30;
            case READING_AVAILABLE:
                struct dnms_measurements dnms_values;

                if (dnms_read_leq(&dnms_values) == 0)
                {
//                    float dnms_corr_value = readCorrectionOffset(cfg::dnms_correction);
                    last_value_dnms_laeq = dnms_values.leq_a;
                    last_value_dnms_la_min = dnms_values.leq_a_min;
                    last_value_dnms_la_max = dnms_values.leq_a_max;
                } else {
                    last_value_dnms_laeq = -1.0;
                    last_value_dnms_la_min = -1.0;
                    last_value_dnms_la_max = -1.0;
                }
                updateState(READY_TO_SEND);
                return 100;
            case READY_TO_SEND:
                return 500;

        }
        return 1000;
    }

    void afterSend(bool status) {
        updateState(INITIALIZED);
    }

    void results(String &s) {
        if (!enabled) return;
        if (state != READY_TO_SEND) return;

        s.concat(Value2Json(F("DNMS_noise_LAeq"), String(last_value_dnms_laeq)));
        s.concat(Value2Json(F("DNMS_noise_LA_min"), String(last_value_dnms_la_min)));
        s.concat(Value2Json(F("DNMS_noise_LA_max"), String(last_value_dnms_la_max)));
    }

    unsigned long process(SimpleScheduler::LoopEventType e) {
        switch (e) {
            case SimpleScheduler::INIT:
                if (init())
                    return 10000;
                else {
                    debug_out(F("!!!! DNMS sensor init failed...."), DEBUG_ERROR, 1);
                }
                return 0;
            case SimpleScheduler::RUN:
                return processState();
            default:
                return 0;
        };
    }

    void getStatusReport(String &res) {
        if (!enabled) return;
        res.concat(FPSTR(EMPTY_ROW));
        char dnms_version[MAX_VERSION_LEN + 1];
//        dnms_reset();
//        delay(1000);
        int16_t ret = readVersion(dnms_version);
        if ( 0 == ret ) {
            res.concat(table_row_from_value(F("DNMS"), F("ver"), String(dnms_version), ""));
        }
        else {
            res.concat(table_row_from_value(F("DNMS"), F("ver"), F("Read failed"), ""));
        }
        res.concat(table_row_from_value(F("DNMS"), F("state"), String(state), ""));
        res.concat(table_row_from_value(F("DNMS"), F("Last state change"), String(millis()-lastStatChange), ""));


    }

    void readConfigJSON(JsonDocument json) {
        enabled = json[F("e")].as<bool>();
        printOnLCD = json[F("d")].as<bool>();

        //register/deregister sensor
        if (enabled && !scheduler.isRegistered(SimpleScheduler::DNMS)) {
            scheduler.registerSensor(SimpleScheduler::DNMS, DNMS::process, FPSTR(DNMS::KEY));
            scheduler.init(SimpleScheduler::DNMS);
            enabled = true;
            debug_out(F("DNMS: start"), DEBUG_MIN_INFO, 1);
        } else if (!enabled && scheduler.isRegistered(SimpleScheduler::DNMS)) {   //de
            debug_out(F("DNMS: stop"), DEBUG_MIN_INFO, 1);
            scheduler.unregisterSensor(SimpleScheduler::DNMS);
        }
        //register display - separate check to allow enable/disable display not only when turning DNMS on/off

        if (enabled && printOnLCD) {
            scheduler.registerDisplay(SimpleScheduler::DNMS, 1);
        }
    }

    void display(byte cols, byte minor, String lines[]) {

        if (getLCDRows() == 2) {    //16x2
            lines[0] = F("DNMS:");
            if (last_value_dnms_laeq == -1) {
                lines[0].concat( String(F(" --")) );
            } else {
                lines[0].concat(String(last_value_dnms_laeq, 1));
                lines[0].concat(String(F(" dB")));
                lines[1] = String(last_value_dnms_la_min,1);
                lines[1].concat(F("/"));
                lines[1].concat(String(last_value_dnms_la_max,1));
                lines[1].concat(String(F(" dB")));
            }
        } else {
            lines[0] = F("DNMS");
            if (last_value_dnms_laeq == -1) {
                lines[1].concat( String(F("--")) );
            } else {
                lines[1] = F("LAeq: ");
                lines[1].concat(String(last_value_dnms_laeq, 1));
                lines[1].concat(String(F(" dB")));

                lines[2] = F("min: ");
                lines[2].concat(String(last_value_dnms_la_min,1));
                lines[2].concat(String(F(" dB")));

                lines[3] = F("max: ");
                lines[3].concat(String(last_value_dnms_la_max,1));
                lines[3].concat(String(F(" dB")));
            }

        }

    };


    String getConfigJSON() {
        String ret = F("");
        ret.concat(Var2JsonInt(F("e"), enabled));
        if (printOnLCD) ret.concat(Var2JsonInt(F("d"), printOnLCD));
        return ret;
    };

#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (uint16_t *) castable)
   @w:  number of word-sized elements in the array (DNMS_NUM_WORDS(a)).
*/
#define DNMS_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }


    int16_t dnms_read_leq(struct dnms_measurements *leq) {
        int16_t ret;
        uint16_t idx;
        union {
            uint16_t uu[2];
            uint32_t u;
            float f;
        } val, data[3];

        ret = dnms_i2c_read_cmd(I2C_ADDRESS, DNMS_CMD_READ_LEQ, data->uu, DNMS_NUM_WORDS(data));
        if (ret != DNMS_STATUS_OK)
            return ret;

        DNMS_WORDS_TO_BYTES(data->uu, DNMS_NUM_WORDS(data));

        idx = 0;
        val.u = be32_to_cpu(data[idx].u);
        leq->leq_a = val.f;
        ++idx;
        val.u = be32_to_cpu(data[idx].u);
        leq->leq_a_min = val.f;
        ++idx;
        val.u = be32_to_cpu(data[idx].u);
        leq->leq_a_max = val.f;

        return 0;
    }


    bool init() {
        char dnms_version[MAX_VERSION_LEN + 1];

        debug_out(F("Trying DNMS sensor on 0x55H "), DEBUG_MIN_INFO);
        dnms_reset();
        delay(1000);    //TODO switch to loop in NAMF
        if (readVersion(dnms_version) != 0)
        {
            debug_out(F("No DNMS sensor"), DEBUG_ERROR);
            state = FAILED;
            return false;
//            dnms_init_failed = true;
        }
        else
        {
            dnms_version[MAX_VERSION_LEN] = 0;
            debug_out(String(F("DNMS ver:")) + String(dnms_version), DEBUG_ERROR);
            state = INITIALIZED;
            return true;
        }
    }


    int16_t dnms_reset() {
        return dnms_i2c_write_cmd(I2C_ADDRESS, DNMS_CMD_RESET);
    }


    int16_t dnms_i2c_write_cmd(uint8_t address, uint16_t command) {
        uint8_t buf[COMMAND_SIZE];

        dnms_fill_cmd_send_buf(buf, command, NULL, 0);
        return dnms_i2c_write(address, buf, COMMAND_SIZE);
    }


    uint8_t dnms_common_generate_crc(uint8_t * data, uint16_t count) {
        uint16_t current_byte;
uint8_t crc = DNMS_CRC8_INIT;
        uint8_t crc_bit;

        /* calculates 8-Bit checksum with given polynomial */
        for ( current_byte = 0; current_byte<count; ++current_byte) {
            crc ^= (data[current_byte]);
            for (crc_bit = 8; crc_bit > 0; --crc_bit) {
                if (crc & 0x80)
crc = (crc << 1) ^ DNMS_CRC8_POLYNOMIAL;
                else
                    crc = (crc << 1);
            }
        }
        return crc;
    }

    int8_t dnms_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum) {
        uint8_t crc;
        crc = dnms_common_generate_crc(data, count);
        if (crc != checksum) {
return
DNMS_STATUS_FAIL;
        }
return
DNMS_STATUS_OK;
    }

    uint16_t dnms_fill_cmd_send_buf(uint8_t * buf, uint16_t cmd, const uint16_t *args, uint8_t num_args) {
        uint8_t crc;
        uint8_t i;
        uint16_t idx = 0;

        buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
        buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

        for (i = 0; i<num_args; ++i) {
            buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
            buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

            crc = dnms_common_generate_crc((uint8_t *) &buf[idx - 2], WORD_SIZE);
            buf[idx++] = crc;
        }
        return idx;
    }

    int8_t dnms_i2c_write(uint8_t address, const uint8_t *data, uint16_t count) {
        Wire.beginTransmission(address);
        Wire.write(data, count);
        Wire.endTransmission();
        return 0;
    }


    int8_t dnms_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
        uint8_t rxByteCount = 0;

        // 2 bytes RH, 1 CRC, 2 bytes T, 1 CRC
        Wire.requestFrom(address, (uint8_t)count);
        while (Wire.available()) { // wait till all arrive
            data[rxByteCount++] = Wire.read();
            if (rxByteCount >= count) {
                break;
            }
        }
        return 0;
    }


    int16_t dnms_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words) {
        int16_t ret;
        uint16_t i, j;
        uint16_t size = num_words * (WORD_SIZE + DNMS_CRC8_LEN);
        uint16_t word_buf[MAX_BUFFER_WORDS];
        uint8_t * const buf8 = (uint8_t *)word_buf;

        ret = dnms_i2c_read(address, buf8, size);
        if (ret != DNMS_STATUS_OK) {
            return ret;
        }
        /* check the CRC for each word */
        for (i = 0, j = 0; i < size; i += WORD_SIZE + DNMS_CRC8_LEN) {
            ret = dnms_common_check_crc(&buf8[i], WORD_SIZE, buf8[i + WORD_SIZE]);
            if (ret != DNMS_STATUS_OK) {
                return ret;
            }
            data[j++] = buf8[i];
            data[j++] = buf8[i + 1];
        }

        return DNMS_STATUS_OK;
    }


    int16_t dnms_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words) {
        int16_t ret;
        uint8_t i;

        ret = dnms_i2c_read_bytes(address, (uint8_t *)data_words, num_words);
        if (ret != DNMS_STATUS_OK) {
            return ret;
        }
        for (i = 0; i < num_words; ++i) {
            data_words[i] = be16_to_cpu(data_words[i]);
        }
        return DNMS_STATUS_OK;
    }


    int16_t dnms_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words) {
        int16_t ret;
        uint8_t buf[COMMAND_SIZE];

        dnms_fill_cmd_send_buf(buf, cmd, NULL, 0);
        ret = dnms_i2c_write(address, buf, COMMAND_SIZE);
        if (ret != DNMS_STATUS_OK) {
            return ret;
        }
        return dnms_i2c_read_words(address, data_words, num_words);
    }


    int16_t readVersion(char *dnms_version) {
        uint16_t i;
        int16_t ret;
        union {
            char dnms_version[MAX_VERSION_LEN];
            uint16_t __enforce_alignment;
        } buffer;

        ret = dnms_i2c_read_cmd(I2C_ADDRESS, DNMS_CMD_READ_VERSION, (uint16_t *) buffer.dnms_version,
                                DNMS_NUM_WORDS(buffer.dnms_version));
        if (ret != DNMS_STATUS_OK) {
            return ret;
        }
        DNMS_WORDS_TO_BYTES(buffer.dnms_version, DNMS_NUM_WORDS(buffer.dnms_version));
        for (i = 0; i < MAX_VERSION_LEN; ++i) {
            dnms_version[i] = buffer.dnms_version[i];
        }
        if ((dnms_version[0] == 'D') && (dnms_version[1] == 'N') && (dnms_version[2] == 'M') && (dnms_version[3] == 'S')) {
            return 0;
        }
        return 1;  // error
    }


}