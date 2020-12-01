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

    int skip_loops = 0;

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
                    skip_loops = 0;
                    debug_out(F("connected"), DEBUG_MIN_INFO, true);
                } else {
                    int connection_state = client.state();
                    debug_out(F("failed, rc="), DEBUG_WARNING, false);
                    debug_out(String(connection_state), DEBUG_WARNING, false);
                    if (connection_state == MQTT_CONNECT_UNAUTHORIZED || connection_state == MQTT_CONNECT_BAD_CREDENTIALS) {
                        debug_out(F(" invalid authentication data"), DEBUG_WARNING, true);
                        skip_loops = 1024;
                        return;
                    }
                    debug_out(F(" will try again in 0.33 seconds"), DEBUG_WARNING, true);
                    retries++;
                    delay(330);
                    yield();
                }
            } else {
                debug_out(F("Failed to (re)connect to MQTT"), DEBUG_ERROR, true);
                return;
            }
        }
    }

    void publish(const String& mqtt_topic, const String &data, bool retained = false) {
        debug_out(F("MQTT publish"), DEBUG_MIN_INFO, true);
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
                if (sensor_value[F("unit")].as<String>() == F("Pa")) {
                    sensor_value.set(F("value"), sensor_value[F("value")].as<double>()/100);
                    sensor_value.set(F("unit"), "hPa");
                } else {
                    sensor_value.set(F("value"), sensor_value[F("value")].as<double>());
                }
                sensor_value.remove(F("value_type"));
                String mqtt_value;
                sensor_value.printTo(mqtt_value);
                debug_out("mqtt data: " + mqtt_type + " ", DEBUG_MIN_INFO, false);
                debug_out(mqtt_value, DEBUG_MIN_INFO, true);
                mqtt::publish(cfg::sensors_topic_mqtt + mqtt_type, mqtt_value);
                yield();
            }
        } else {
            debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, true);
        }
    }

    bool loop() {
        if (skip_loops > 0) {
            skip_loops--;
            return false;
        }
        reconnect(1);
        return client.loop();
    }
}