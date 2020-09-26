//
// Created by Julian Szulc on 08/08/2020.
//
#include "mqtt.h"
#include "helpers.h"
#include "ext_def.h"
#include "variables.h"
#include <ArduinoJson.h>
#include "html-content.h"

#undef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 2048

namespace mqtt {

    PubSubClient client;

    void setup(WiFiClient &wifiClient) {
        client.setClient(wifiClient);
        client.setServer(cfg::host_mqtt.c_str(), cfg::port_mqtt);
    }


    void reconnect(int maxRetries) {
        int retries = 0;
        while (!(client.connected())) {
            if (retries < maxRetries) {
                debug_out(F("Attempting MQTT connection..."), DEBUG_MIN_INFO, false);
                if (client.connect(cfg::client_id_mqtt, cfg::user_mqtt.c_str(), cfg::pwd_mqtt.c_str())) {
                    debug_out(F("connected"), DEBUG_MIN_INFO, true);
                } else {
                    debug_out(F("failed, rc="), DEBUG_WARNING, false);
                    debug_out(String(client.state()), DEBUG_WARNING, false);
                    debug_out(F(" try again in 0.5 second"), DEBUG_WARNING, true);
                    retries++;
                    yield();
                    delay(500);
                }
            } else {
                debug_out(F("Failed to (re)connect to MQTT"), DEBUG_ERROR, true);
                break;
            }
        }
    }

    void publish(const String& mqtt_topic, const String &data, bool retained = false) {
        reconnect(3);
        if (!client.publish(mqtt_topic.c_str(), data.c_str(), retained)) {
            debug_out(F("Failed to publish data to mqtt:"), DEBUG_WARNING, false);
            debug_out(data, DEBUG_WARNING, true);
        }
    }

    void send_mqtt(const String& data) {
        StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
        JsonObject& json2data = jsonBuffer.parseObject(data);
        time_t now = time(nullptr);
        String timestamp=(ctime(&now));
        timestamp.trim();
        if (json2data.success()) {
            for (uint8_t i = 0; i < json2data["sensordatavalues"].size(); i++) {
                JsonObject& sensor_value = json2data["sensordatavalues"][i];
                sensor_value.set(F("timestamp"), timestamp);
                String mqtt_type = sensor_value[F("value_type")].as<String>();
                sensor_value.remove(F("value_type"));
                String mqtt_value;
                sensor_value.printTo(mqtt_value);
                mqtt::publish(cfg::sensors_topic_mqtt + mqtt_type, mqtt_value);
            }
            mqtt::publish(cfg::sensors_topic_mqtt + "last_data_time", String(now, 10), true);
        } else {
            debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, true);
        }
    }
}