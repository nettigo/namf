/* in case You need to crash your ESP :) some handy functions
 * use setup_webserver() to add few urls /crash... to generate crashes on request */

#include <Arduino.h>

void crashme(){
    int* i = nullptr;
    *i = 80;
}

void crashme2(){
    char *svptr = nullptr;
    static char* str_input = nullptr;
    const char delim[] = " ";
    char input[] = "bla";
    size_t malloc_amount = (sizeof(char) * 0) & (~3);
    str_input = (char *)malloc(malloc_amount);
    memset(str_input, '\0', 0);
    strcpy(str_input, input);
}

/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
void setup_webserver(ESP8266WebServer server) {
    server.on("/crash", []() {
        server.send(200, "text/plain", "ESP8266 is going to crash with EXCEPTION..");
        char linea[] = "0x123456", **ap;
        int num;
        num = strtol(linea, ap, 0);
        printf("%d\n%s", num, *ap);
        int k;
    });
    server.on("/crash2", []() {
        server.send(200, "text/plain", "ESP8266 is going to crash with SOFT_WDT..");
        while (true) {
            Serial.println("Crashing...");
        }
    });
    server.on("/crash3", []() {
        server.send(200, "text/plain", "ESP8266 is goign to crash with WDT..");
        ESP.wdtDisable();
        while (true) {
            Serial.println("Crashing...");
        }
    });
    server.on("/crash4", []() {
        server.send(200, "text/plain", "ESP8266 is going to crash with EXCEPTION..");
        crashme();
    });
    server.on("/crash5", []() {
        server.send(200, "text/plain", "ESP8266 is going to crash with EXCEPTION..");
        crashme2();
    });
}