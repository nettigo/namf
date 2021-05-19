//
// Created by viciu on 16.04.2020.
//

#include "commons.h"
//used internally no need to declare outside
enum class DisplayPages {
    PagePM,
    PageTemp,
    PageHECA,
    PageGPS,
    PageWIFI,
    PageInfo
};

unsigned long next_display_millis = 0;
unsigned long next_display_count = 0;
unsigned static_screen_count = 0;


/*****************************************************************
 * check display values, return '-' if undefined                 *
 *****************************************************************/
String check_display_value(double value, double undef, uint8_t len, uint8_t str_len) {
    String s = (value != undef ? Float2String(value, len) : "-");
    while (s.length() < str_len) {
        s = " " + s;
    }
    return s;
}


String displayGenerateFooter(unsigned int screen_count) {
    String display_footer;
    for (unsigned int i = 0; i < screen_count; ++i) {
        display_footer += (i != (next_display_count % screen_count)) ? " . " : " o ";
    }
    return display_footer;
}



/*****************************************************************
 * display values                                                *
 *****************************************************************/
void display_values() {
    double t_value = -128.0;
    double h_value = -1.0;
    double t_hc_value = -128.0; // temperature inside heating chamber
    double h_hc_value = -1.0; //  humidity inside heating chamber
    double p_value = -1.0;
    String t_sensor = "";
    String h_sensor = "";
    String p_sensor = "";
    double pm10_value = -1.0;
    double pm25_value = -1.0;
    String pm10_sensor = "";
    String pm25_sensor = "";
    double lat_value = -200.0;
    double lon_value = -200.0;
    double alt_value = -1000.0;
    String gps_sensor = "";
    String display_header = "";
    String display_lines[3] = { "", "", ""};
    DisplayPages screens[5];
    int line_count = 0;
    static_screen_count = 0;
//	debug_out(F("output values to display..."), DEBUG_MAX_INFO, 1);
    if (cfg::pms_read) {
        pm10_value = last_value_PMS_P1;
        pm10_sensor = FPSTR(SENSORS_PMSx003);
        pm25_value = last_value_PMS_P2;
        pm25_sensor = FPSTR(SENSORS_PMSx003);
    }
    if (cfg::dht_read) {
        t_value = last_value_DHT_T;
        t_sensor = FPSTR(SENSORS_DHT22);
        h_value = last_value_DHT_H;
        h_sensor = FPSTR(SENSORS_DHT22);
    }
    if (cfg::ds18b20_read) {
        t_value = last_value_DS18B20_T;
        t_sensor = FPSTR(SENSORS_DS18B20);
    }
    if (cfg::bmp280_read) {
        t_value = last_value_BMP280_T;
        t_sensor = FPSTR(SENSORS_BMP280);
        p_value = last_value_BMP280_P;
        p_sensor = FPSTR(SENSORS_BMP280);
    }
    if (cfg::bme280_read) {
        t_value = last_value_BME280_T;
        t_sensor = FPSTR(SENSORS_BME280);
        h_value = last_value_BME280_H;
        h_sensor = FPSTR(SENSORS_BME280);
        p_value = last_value_BME280_P;
        p_sensor = FPSTR(SENSORS_BME280);
    }
    if (cfg::gps_read) {
        lat_value = last_value_GPS_lat;
        lon_value = last_value_GPS_lon;
        alt_value = last_value_GPS_alt;
        gps_sensor = "NEO6M";
    }
    if (cfg::pms_read) {
        screens[static_screen_count++] = DisplayPages::PagePM;
    }
    if (cfg::dht_read || cfg::ds18b20_read || cfg::bmp280_read || cfg::bme280_read) {
        screens[static_screen_count++] = DisplayPages::PageTemp;
    }
    if (cfg::gps_read) {
        screens[static_screen_count++] = DisplayPages::PageGPS;
    }
    if (cfg::show_wifi_info) {
        screens[static_screen_count++] = DisplayPages::PageWIFI;    // Wifi info
    }
    screens[static_screen_count++] = DisplayPages::PageInfo;    // chipID, firmware and count of measurements
    bool skipOldDisplay = false;
    if (next_display_count+1 > static_screen_count) {
        byte diff = next_display_count - static_screen_count;
        byte minor;
        SimpleScheduler::LoopEntryType sensor = scheduler.selectSensorToDisplay(diff, minor);
        if (sensor == SimpleScheduler::EMPTY) {
            next_display_count = 0;
        } else {
            String lines[] = {"","","",""};
            SimpleScheduler::displaySensor(sensor, lines, 20, 4, minor);
            if (display) {
                display->clear();
                display->displayOn();
//                display->setTextAlignment(TEXT_ALIGN_CENTER);
//                display->drawString(64, 1, display_header);
                display->setTextAlignment(TEXT_ALIGN_LEFT);
                display->drawString(0, 1, lines[0]);
                display->drawString(0, 16, lines[1]);
                display->drawString(0, 28, lines[2]);
                display->drawString(0, 40, lines[3]);
                display->setTextAlignment(TEXT_ALIGN_CENTER);
                display->drawString(64, 52, displayGenerateFooter(static_screen_count+scheduler.countScreens()));
                display->display();
            }
            next_display_count++;
            skipOldDisplay = true;
        }


    }
    if (!skipOldDisplay) {

        if (cfg::has_display || cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) {
            switch (screens[next_display_count % static_screen_count]) {
                case (DisplayPages::PagePM):
                    display_header = pm25_sensor;
                    if (pm25_sensor != pm10_sensor) {
                        display_header += " / " + pm25_sensor;
                    }
                    display_lines[0] = "PM2.5: " + check_display_value(pm25_value, -1, 1, 6) + " µg/m³";
                    display_lines[1] = "PM10:  " + check_display_value(pm10_value, -1, 1, 6) + " µg/m³";
                    display_lines[2] = "";
                    break;
                case (DisplayPages::PageTemp):
                    display_header = t_sensor;
                    if (h_sensor != "" && t_sensor != h_sensor) {
                        display_header += " / " + h_sensor;
                    }
                    if ((h_sensor != "" && p_sensor != "" && (h_sensor != p_sensor)) ||
                        (h_sensor == "" && p_sensor != "" && (t_sensor != p_sensor))) {
                        display_header += " / " + p_sensor;
                    }
                    if (t_sensor != "") {
                        display_lines[line_count++] = "Temp.: " + check_display_value(t_value, -128, 1, 6) + " °C";
                    }
                    if (h_sensor != "") {
                        display_lines[line_count++] = "Hum.:  " + check_display_value(h_value, -1, 1, 6) + " %";
                    }
                    if (p_sensor != "") {
                        display_lines[line_count++] =
                                "Pres.: " + check_display_value(p_value / 100, (-1 / 100.0), 1, 6) + " hPa";
                    }
                    while (line_count < 3) { display_lines[line_count++] = ""; }
                    break;
                case (DisplayPages::PageHECA):
                    display_header = F("NAM HECA (SHT30)");
                    display_lines[0] = "Temp.: " + check_display_value(t_hc_value, -128, 1, 6) + " °C";
                    display_lines[1] = "Hum.:  " + check_display_value(h_hc_value, -1, 1, 6) + " %";
                    display_lines[2] = "PTC HE: Soon :)"; // PTC heater status
                    break;
                case (DisplayPages::PageGPS):
                    display_header = gps_sensor;
                    display_lines[0] = "Lat: " + check_display_value(lat_value, -200.0, 6, 10);
                    display_lines[1] = "Lon: " + check_display_value(lon_value, -200.0, 6, 10);
                    display_lines[2] = "Alt: " + check_display_value(alt_value, -1000.0, 2, 10);
                    break;
                case (DisplayPages::PageWIFI):
                    display_header = F("Wifi info");
                    display_lines[0] = "IP: " + WiFi.localIP().toString();
                    display_lines[1] = "SSID:" + WiFi.SSID();
                    display_lines[2] = "Signal: " + String(calcWiFiSignalQuality(WiFi.RSSI())) + "%";
                    break;
                case (DisplayPages::PageInfo):
                    display_header = F("Device Info");
                    display_lines[0] = "ID: " + esp_chipid();
                    display_lines[1] = "FW: " + String(SOFTWARE_VERSION);
                    display_lines[2] = "Measurements: " + String(count_sends);
                    break;
            }

            if (display) {
                display->clear();
                display->displayOn();
                display->setTextAlignment(TEXT_ALIGN_CENTER);
                display->drawString(64, 1, display_header);
                display->setTextAlignment(TEXT_ALIGN_LEFT);
                display->drawString(0, 16, display_lines[0]);
                display->drawString(0, 28, display_lines[1]);
                display->drawString(0, 40, display_lines[2]);
                display->setTextAlignment(TEXT_ALIGN_CENTER);
                display->drawString(64, 52, displayGenerateFooter(static_screen_count));
                display->display();
            }
            if (cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) {
                display_header = getLCDHeader() + " " + display_header;
                display_lines[0].replace(" µg/m³", "");
                display_lines[0].replace("°", String(char(223)));
                display_lines[1].replace(" µg/m³", "");
                char_lcd->clear();
                char_lcd->setCursor(0, 0);
                char_lcd->print(display_header);
                char_lcd->setCursor(0, 1);
                char_lcd->print(display_lines[0]);
                char_lcd->setCursor(0, 2);
                char_lcd->print(display_lines[1]);
                char_lcd->setCursor(0, 3);
                char_lcd->print(display_lines[2]);
            }
        }


        if (cfg::has_lcd1602_27 || cfg::has_lcd1602) {
            switch (screens[next_display_count % static_screen_count]) {
                case (DisplayPages::PagePM):
                    display_lines[0] = "PM2.5: " + check_display_value(pm25_value, -1, 1, 6);
                    display_lines[1] = "PM10:  " + check_display_value(pm10_value, -1, 1, 6);
                    break;
                case (DisplayPages::PageTemp):
                    display_lines[0] = "T: " + check_display_value(t_value, -128, 1, 6) + char(223) + "C";
                    display_lines[1] = "H: " + check_display_value(h_value, -1, 1, 6) + "%";
                    break;
                case (DisplayPages::PageHECA):
                    display_lines[0] = "Th: " + check_display_value(t_hc_value, -128, 1, 6) + char(223) + "C";
                    display_lines[1] = "Hh: " + check_display_value(h_hc_value, -1, 1, 6) + "%";
                    break;
                case (DisplayPages::PageGPS):
                    display_lines[0] = "Lat: " + check_display_value(lat_value, -200.0, 6, 11);
                    display_lines[1] = "Lon: " + check_display_value(lon_value, -200.0, 6, 11);
                    break;
                case (DisplayPages::PageWIFI):
                    display_lines[0] = WiFi.localIP().toString();
                    display_lines[1] = WiFi.SSID();
                    break;
                case (DisplayPages::PageInfo):
                    display_lines[0] = esp_chipid() + " " + String(count_sends);
                    display_lines[1] = String(SOFTWARE_VERSION);
                    break;
            }

            char_lcd->clear();
            char_lcd->setCursor(0, 0);
            char_lcd->print(display_lines[0]);
            char_lcd->setCursor(0, 1);
            char_lcd->print(display_lines[1]);
        }
    next_display_count += 1;
    }
    yield();
    next_display_millis = millis() + DISPLAY_UPDATE_INTERVAL_MS;
}

//get LCD screen sizes. returns 0 if no LCD or graphical one (SSD1306)
byte getLCDCols(){
    if (cfg::has_lcd1602 || cfg::has_lcd1602_27) return 16;
    if (cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) return 20;
    return 0;
};

byte getLCDRows(){
    if (cfg::has_lcd1602 || cfg::has_lcd1602_27) return 2;
    if (cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) return 4;
    return 0;

};

String getLCDHeader(){
    String ret = String(next_display_count+1)+F("/")+String(static_screen_count+scheduler.countScreens());
    return ret;
};

void cycleDisplay(void){
    if ((cfg::has_display || cfg::has_lcd2004_27 || cfg::has_lcd2004_3f || cfg::has_lcd1602 ||
         cfg::has_lcd1602_27) && (act_milli > next_display_millis)) {
        display_values();
    }

}