//
// Created by viciu on 22.02.2020.
//


#include "winsen-mhz.h"
#include <stdint.h>
#include <stdbool.h>

namespace MHZ14A {
    const char KEY[] PROGMEM = "MHZ14A";
    bool enabled = false;
    bool printOnLCD = false;

#define CMD_SIZE 9

    typedef enum {
        START_BYTE,
        COMMAND,
        DATA,
        CHECK
    } state_t;

    //for internal use - register/deregister display
    void registerDisplayMHZ() {
        if (enabled && printOnLCD)
            scheduler.registerDisplay(SimpleScheduler::MHZ14A, 1);  // one screen
        else
            scheduler.registerDisplay(SimpleScheduler::MHZ14A, 0);  // disable
    }

    JsonObject &parseHTTPRequest() {
        setBoolVariableFromHTTP(String(F("enabled")), enabled, SimpleScheduler::MHZ14A);
        setBoolVariableFromHTTP(String(F("display")), printOnLCD, SimpleScheduler::MHZ14A);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &ret = jsonBuffer.createObject();
        ret[F("e")] = enabled;
        ret[F("d")] = printOnLCD;
        registerDisplayMHZ();
        ret.printTo(Serial);
        return ret;

    };

    //send data to LD API...
    void sendToLD(){

    };

    void getResults(String &str) {
        if (!enabled) return;
        str.concat(sensorMHZ());
    };

    void resultsAsHTML(String &page_content) {
        if (!enabled) { return; }
        page_content.concat(FPSTR(EMPTY_ROW));
        page_content.concat(table_row_from_value(FPSTR(INTL_MHZ14A_VAL), F("CO2"), String(last_value_WINSEN_CO2),
                                             F("ppm")));
    }

    void readConfigJSON(JsonObject &json) {
        enabled = json.get<bool>(F("e"));
        printOnLCD = json.get<bool>(F("d"));
        scheduler.enableSubsystem(SimpleScheduler::MHZ14A, enabled, MHZ14A::process, FPSTR(MHZ14A::KEY));
        registerDisplayMHZ();

    };

    String getConfigJSON(void) {
        String ret = F("");
        ret += Var2JsonInt(F("e"), enabled);
        if (printOnLCD) ret += Var2JsonInt(F("d"), printOnLCD);
        return ret;
    }

    void display(byte cols, byte minor, String lines[]) {

        lines[0] = (F("CO2 (MHZ14A)"));
        lines[1] = String(last_value_WINSEN_CO2);
        lines[1] += (F(" ppm"));

        return;
    };

    bool getDisplaySetting() {
        return printOnLCD;
    };


    unsigned long process(SimpleScheduler::LoopEventType event) {
        switch (event) {
            case SimpleScheduler::INIT:
                setupWinsenMHZ(serialGPS);
                return 1000;
            case SimpleScheduler::RUN:
                readWinsenMHZ(serialGPS);
                return 1000;
            default:
                return 1000;
        }
    };

/**
    Prepares a command buffer to send to an mhz19.
    @param data tx data
    @param buffer the tx buffer to fill
    @param size the size of the tx buffer
    @return number of bytes in buffer
*/
    int prepare_tx(uint8_t cmd, const uint8_t *data, uint8_t buffer[], int size) {
        if (size < CMD_SIZE) {
            return 0;
        }

        // create command buffer
        buffer[0] = 0xFF;
        buffer[1] = 0x01;
        buffer[2] = cmd;
        for (int i = 3; i < 8; i++) {
            buffer[i] = *data++;
        }

        // calculate checksum
        uint8_t check = 0;
        for (int i = 0; i < 8; i++) {
            check += buffer[i];
        }
        buffer[8] = 255 - check;

        return CMD_SIZE;
    }

/**
    Processes one received byte.
    @param b the byte
    @param cmd the command code
    @param data the buffer to contain a received message
    @return true if a full message was received, false otherwise
 */
    bool process_rx(uint8_t b, uint8_t cmd, uint8_t data[]) {
        static uint8_t check = 0;
        static int idx = 0;
        static int len = 0;
        static state_t state = START_BYTE;

        // update checksum
        check += b;

        switch (state) {
            case START_BYTE:
                if (b == 0xFF) {
                    check = 0;
                    state = COMMAND;
                }
                break;
            case COMMAND:
                if (b == cmd) {
                    idx = 0;
                    len = 6;
                    state = DATA;
                } else {
                    state = START_BYTE;
                    process_rx(b, cmd, data);
                }
                break;
            case DATA:
                data[idx++] = b;
                if (idx == len) {
                    state = CHECK;
                }
                break;
            case CHECK:
                state = START_BYTE;
                return (check == 0);
            default:
                state = START_BYTE;
                break;
        }

        return false;
    }

    bool exchange_command(SoftwareSerial &sensor, uint8_t cmd, uint8_t data[], unsigned int timeout) {
        // create command buffer
        uint8_t buf[9];
        int len = prepare_tx(cmd, data, buf, sizeof(buf));

        // send the command
        sensor.write(buf, len);

        // wait for response
        unsigned long start = millis();
        while ((millis() - start) < timeout) {
            if (sensor.available() > 0) {
                uint8_t b = sensor.read();
                if (process_rx(b, cmd, data)) {
                    return true;
                }
            }
            yield();
        }

        return false;
    }

    bool set_range(SoftwareSerial &serial, range_t r) {
        uint8_t data[6];
        uint8_t *p = data;
        switch (r) {
            case RANGE_2K:
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x07;
                *p++ = 0xD0;
                *p++ = 0x8F;
                break;
            case RANGE_10K:
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x00;
                *p++ = 0x27;
                *p++ = 0x10;
                *p++ = 0x2F;
                break;
        }
        return exchange_command(serial, 0x99, p, 3000);
    }


    bool read_temp_co2(SoftwareSerial &serial, unsigned int *co2, unsigned int *temp) {
        uint8_t data[] = {0, 0, 0, 0, 0, 0};
        bool result = exchange_command(serial, 0x86, data, 3000);
        if (result) {
            *temp = data[2] - 40;
            *co2 = (data[0] << 8) + data[1];
#if 0
            char raw[32];
            sprintf(raw, "RAW: %02X %02X %02X %02X %02X %02X", data[0], data[1], data[2], data[3],
                    data[4], data[5]);
            Serial.println(raw);
            Serial.print(*temp);
            Serial.print(F(" "));
            Serial.println(*co2);
#endif
        }
        return result;
    }


#define WINSEN_AVG_SAMPLE   10
    unsigned int *samples = nullptr;
    unsigned int samplesCount;

    void setupWinsenMHZ(SoftwareSerial &serial) {
        serial.begin(9600);
        set_range(serial, RANGE_2K);
        samples = new unsigned int[WINSEN_AVG_SAMPLE];
        if (samples == nullptr) {
            debug_out(F("MHZ14A - no array for data"), DEBUG_ERROR);
        }
        samplesCount = 0;
    }

    void readWinsenMHZ(SoftwareSerial &serial) {
        unsigned int co2, temp;
        static unsigned long lastRead = millis();
        static unsigned long interval = cfg::sending_intervall_ms / (WINSEN_AVG_SAMPLE + 2);
//        Serial.println(samplesCount);
//        Serial.println(millis() - lastRead);
//        Serial.println(interval);
        if (samplesCount<WINSEN_AVG_SAMPLE && millis() - lastRead > interval)
            if (read_temp_co2(serial, &co2, &temp)) {
//            debug_out(String("read Winsen"), DEBUG_MIN_INFO, true);
                lastRead = millis();
                samples[samplesCount++] = co2;
                last_value_WINSEN_CO2 = co2;

            } //else debug_out(String("**** NO read Winsen"), DEBUG_MIN_INFO, true);
    }

    //reset counter after sending
    void afterSend(bool status) {
        samplesCount = 0;
    }

    float currentReading() {
        unsigned long sum = 0;
        if (samplesCount == 0) return 0;

        for (byte i = 0; i < samplesCount; i++)
            sum += samples[i];
        return sum /(float)samplesCount;
    }

    String sensorMHZ() {
        String s;
        if (!samples || samplesCount == 0)
        return s;
        if (currentReading() > 0) {
            s += Value2Json(F("conc_co2_ppm"), String(currentReading()));
        }
        return s;
    }
}