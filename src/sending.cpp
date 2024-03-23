#include "sending.h"

#if defined(ARDUINO_ARCH_ESP8266)
BearSSL::X509List x509_dst_root_ca(dst_root_ca_x1);

void configureCACertTrustAnchor(WiFiClientSecure* client) {
    constexpr time_t fw_built_year = (__DATE__[ 7] - '0') * 1000 + \
							  (__DATE__[ 8] - '0') *  100 + \
							  (__DATE__[ 9] - '0') *   10 + \
							  (__DATE__[10] - '0');
    if (time(nullptr) < (fw_built_year - 1970) * 365 * 24 * 3600) {
        debug_out(F("Time incorrect; Disabling CA verification."), DEBUG_MIN_INFO,1);
        client->setInsecure();
    }
    else {
        client->setTrustAnchors(&x509_dst_root_ca);
    }
}
#else
void configureCACertTrustAnchor(WiFiClientSecure* client) {
    constexpr time_t fw_built_year = (__DATE__[ 7] - '0') * 1000 + \
							  (__DATE__[ 8] - '0') *  100 + \
							  (__DATE__[ 9] - '0') *   10 + \
							  (__DATE__[10] - '0');
    if (time(nullptr) < (fw_built_year - 1970) * 365 * 24 * 3600) {
        debug_out(F("Time incorrect; Disabling CA verification."), DEBUG_MIN_INFO,1);
        client->setInsecure();
    }
    else {
        client->setCACert(dst_root_ca_x3);
    }
}
#endif

/*****************************************************************
 * send data to rest api                                         *
 *****************************************************************/
void sendData(const LoggerEntry logger, const String &data, const int pin, const String &host, const int httpPort, const String &url, const bool verify) {
    WiFiClient *client;
    const __FlashStringHelper *contentType;
    bool ssl = false;
    if (httpPort == 443 || cfg::ssl_influx == 1) {
        client = new WiFiClientSecure;
        ssl = true;
        configureCACertTrustAnchor(static_cast<WiFiClientSecure *>(client));
#ifdef ARDUINO_ARCH_ESP8266
        static_cast<WiFiClientSecure *>(client)->setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
#endif
    } else {
        client = new WiFiClient;
    }
    client->setTimeout(20000);
    int result = 0;

    switch (logger)
    {
        case LoggerInflux:
            contentType = FPSTR(TXT_CONTENT_TYPE_INFLUXDB);
            break;
        default:
            contentType = FPSTR(TXT_CONTENT_TYPE_JSON);
            break;
    }

    debug_out(F("Start connecting to "), DEBUG_MED_INFO, 0);
    debug_out(host, DEBUG_MED_INFO, 1);

    HTTPClient *http;
    http = new HTTPClient;
    http->setTimeout(20 * 1000);
    http->setUserAgent(String(SOFTWARE_VERSION) + "/" + esp_chipid());
    http->setReuse(false);
    debug_out(String(host), DEBUG_MAX_INFO, 1);
    debug_out(String(httpPort), DEBUG_MAX_INFO, 1);
    debug_out(String(url), DEBUG_MAX_INFO, 1);
    if (logger == LoggerInflux && (
            (cfg::user_influx != NULL && strlen(cfg::user_influx) > 0) ||
            (cfg::pwd_influx != NULL && strlen(cfg::pwd_influx) > 0)
            )) {
        http->setAuthorization(cfg::user_influx, cfg::pwd_influx);
    }
    if (http->begin(*client, host, httpPort, url, ssl)) {
        if (logger == LoggerCustom && (*cfg::user_custom || *cfg::pwd_custom))
        {
            http->setAuthorization(cfg::user_custom, cfg::pwd_custom);
        }
        if (logger == LoggerInflux && (*cfg::user_influx || *cfg::pwd_influx))
        {
            http->setAuthorization(cfg::user_influx, cfg::pwd_influx);
        }

        http->addHeader(F("Content-Type"), contentType);
        http->addHeader(F("X-Sensor"), String(F(PROCESSOR_ARCH)) + F("-") + esp_chipid());
        if (pin) {
            http->addHeader(F("X-PIN"), String(pin));
        }
        debug_out(data, DEBUG_MAX_INFO);
        result = http->POST(data);

        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
            debug_out(F("Succeeded - "), DEBUG_MIN_INFO);
        } else {
            debug_out(F("Not succeeded. HTTP status code: "), DEBUG_MIN_INFO, false);
            debug_out(String(result), DEBUG_MIN_INFO);
        }
        debug_out(F("Request result: "), DEBUG_MED_INFO, 0);
        debug_out(String(result), DEBUG_MED_INFO, 1);
        if (result != 204 && http->getString().length() > 0) {
            debug_out(F("Details:"), DEBUG_MED_INFO, 1);
            debug_out(http->getString(), DEBUG_MED_INFO, 1);
        }


    } else {
        debug_out(F("Failed connecting"), DEBUG_MIN_INFO, 1);
    }
    http->end();
    debug_out(F("End connecting to "), DEBUG_MED_INFO, 0);
    debug_out(host, DEBUG_MED_INFO);
    delete (http);
    delete (client);
#ifdef ARDUINO_ARCH_ESP8266
    wdt_reset(); // nodemcu is alive
#endif
    yield();
}

/*****************************************************************
 * send single sensor data to luftdaten.info api                 *
 *****************************************************************/
void sendLuftdaten(const String& data, const int pin, const char* host, const int httpPort, const char* url, const bool verify, const char* replace_str) {
    String data_4_dusti = FPSTR(data_first_part);
    data_4_dusti.replace(String(F("{v}")), String(SOFTWARE_VERSION));
    data_4_dusti.concat(data);
    data_4_dusti.remove(data_4_dusti.length() - 1);
    data_4_dusti.replace(replace_str, String(F("")));
    data_4_dusti.concat(String(F("]}")));
    if (data != "") {
        sendData(LoggerDusti, data_4_dusti, pin, host, httpPort, url, verify);
    } else {
        debug_out(F("No data sent..."), DEBUG_MED_INFO, 1);
    }
//    debugData(data_4_dusti,F("sendLuftdaten data4dusti:"));
//    debugData(data,F("sendLuftdaten data out:"));
}

/*****************************************************************
 * send data to LoRa gateway                                     *
 *****************************************************************/
// void send_lora(const String& data) {
// }

/*****************************************************************
 * send data to mqtt api                                         *
 *****************************************************************/
// rejected (see issue #33)

/*****************************************************************
 * send data to influxdb                                         *
 *****************************************************************/
String create_influxdb_string(const String& data) {
    String data_4_influxdb = "";
    debug_out(F("Parse JSON for influx DB"), DEBUG_MIN_INFO, 1);
    debug_out(data, DEBUG_MIN_INFO, 1);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json2data = jsonBuffer.parseObject(data);
    if (json2data.success()) {
        bool first_line = true;
        data_4_influxdb.concat(F("feinstaub,node="));
        data_4_influxdb.concat(String(F(PROCESSOR_ARCH)) + String(F("-")));
        data_4_influxdb.concat(esp_chipid() + F(","));
        data_4_influxdb.concat(F("fw="));
        data_4_influxdb.concat(F(SOFTWARE_VERSION));
        data_4_influxdb.concat(F(","));
        data_4_influxdb.concat(F("hostname="));
        data_4_influxdb.concat(cfg::fs_ssid);
        data_4_influxdb.concat(F(","));
        data_4_influxdb.concat(F("chann="));
        switch (cfg::update_channel) {
            case UPDATE_CHANNEL_ALFA:
                data_4_influxdb.concat(F("alfa"));
                break;
            case UPDATE_CHANNEL_BETA:
                data_4_influxdb.concat(F("beta"));
                break;
            case UPDATE_CHANNEL_STABLE:
                data_4_influxdb.concat(F("stable"));
                break;
            default:
                data_4_influxdb.concat(F("unknown"));
                break;


        }
        data_4_influxdb.concat(F(" "));
        for (uint8_t i = 0; i < json2data["sensordatavalues"].size(); i++) {
            String tmp_str = "";
            if (first_line)
                first_line = false;
            else
                tmp_str.concat(F(","));
            tmp_str.concat(json2data["sensordatavalues"][i]["value_type"].as<char *>());
            tmp_str.concat(F("="));
            if (
                    json2data["sensordatavalues"][i]["value_type"] == String(F("GPS_date")) ||
                    json2data["sensordatavalues"][i]["value_type"] == String(F("GPS_time"))
                    )
                tmp_str.concat(String(F("\"")) + json2data["sensordatavalues"][i]["value"].as<char *>() + String(F("\"")));
            else
                tmp_str.concat(json2data["sensordatavalues"][i]["value"].as<char *>());
            data_4_influxdb.concat(tmp_str);
        }
#ifdef DBG_NAMF_SDS_NO_DATA
        if (SDS011::enabled && (last_value_SDS_P1 == -1 || last_value_SDS_P2 == -1)) {
            data_4_influxdb.concat(F(",SDS_P1="));
            data_4_influxdb.concat(String(last_value_SDS_P1));
            data_4_influxdb.concat(F(",SDS_P2="));
            data_4_influxdb.concat(String(last_value_SDS_P2));
        }
#endif
        data_4_influxdb.concat(F(",measurements="));
        data_4_influxdb.concat(String(count_sends+1));
        data_4_influxdb.concat(F(",free="));
        data_4_influxdb.concat(String(memoryStatsMin.freeHeap));
        data_4_influxdb.concat(F(",frag="));
        data_4_influxdb.concat(String(memoryStatsMax.frag));
        data_4_influxdb.concat(F(",max_block="));
        data_4_influxdb.concat(String(memoryStatsMin.maxFreeBlock));
        data_4_influxdb.concat(F(",cont_stack="));
        data_4_influxdb.concat(String(memoryStatsMin.freeContStack));
        data_4_influxdb.concat("\n");
    } else {
        debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
    }
    debug_out(data_4_influxdb,DEBUG_MIN_INFO,1);
    return data_4_influxdb;
}

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
void send_csv(const String& data) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json2data = jsonBuffer.parseObject(data);
    debug_out(F("CSV Output"), DEBUG_MIN_INFO, 1);
    debug_out(data, DEBUG_MIN_INFO, 1);
    if (json2data.success()) {
        String headline = F("Timestamp_ms;");
        String valueline = String(act_milli) + ";";
        for (uint8_t i = 0; i < json2data["sensordatavalues"].size(); i++) {
            String tmp_str = json2data["sensordatavalues"][i]["value_type"].as<char*>();
            headline += tmp_str + ";";
            tmp_str = json2data["sensordatavalues"][i]["value"].as<char*>();
            valueline += tmp_str + ";";
        }
        static bool first_csv_line = true;
        if (first_csv_line) {
            if (headline.length() > 0) {
                headline.remove(headline.length() - 1);
            }
            Serial.println(headline);
            first_csv_line = false;
        }
        if (valueline.length() > 0) {
            valueline.remove(valueline.length() - 1);
        }
        Serial.println(valueline);
    } else {
        debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
    }
}
