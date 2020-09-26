//
// Created by Julian Szulc on 08/08/2020.
//

#ifndef NAMF_MQTT_H
#define NAMF_MQTT_H

#include "PubSubClient.h"
#include "ESP8266WiFi.h"

namespace mqtt {
    extern PubSubClient client;

    void setup(WiFiClient &);
    void reconnect(int maxRetries = 3);

    void publish(const String& mqtt_topic, const String &data, bool retained);

    void send_mqtt(const String& data);
}

#endif //NAMF_MQTT_H
