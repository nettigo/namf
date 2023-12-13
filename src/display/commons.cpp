//
// Created by viciu on 16.04.2020.
//

#include "commons.h"
#include "../lib/testable/testable.h"
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

void initCustomChars() {
    byte chr1[8] =  {
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B11111
    };
    byte chr2[8] =  {
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B11111,
            B11111
    };
    byte chr3[8] =  {
            B00000,
            B00000,
            B00000,
            B00000,
            B00000,
            B11111,
            B11111,
            B11111
    };
    byte chr4[8] =  {
            B00000,
            B00000,
            B00000,
            B00000,
            B11111,
            B11111,
            B11111,
            B11111
    };
    byte chr5[8] =  {
            B00000,
            B00000,
            B00000,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111
    };
    byte chr6[8] =  {
            B00000,
            B00000,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111
    };
    byte chr7[8] =  {
            B00000,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111
    };
    byte chr8[8] =  {
            B00000,
            B01110,
            B10001,
            B00100,
            B01010,
            B00000,
            B00100,
            B00000
    };
    if (char_lcd) {
        char_lcd->createChar(1,chr1);
        char_lcd->createChar(2,chr2);
        char_lcd->createChar(3,chr3);
        char_lcd->createChar(4,chr4);
        char_lcd->createChar(5,chr5);
        char_lcd->createChar(6,chr6);
        char_lcd->createChar(7,chr7);
        char_lcd->createChar(8,chr8);
    }
}

void displayProgressBar() {
    if (char_lcd && getLCDRows() > 2) {
        char_lcd->setCursor(19,3);
        byte x = 7 - (byte)((float)time2Measure()/cfg::sending_intervall_ms * 7) ;
        char_lcd->write(x);
        debug_out(F("Progress bar: "),DEBUG_ERROR,0);
        debug_out(String(x),DEBUG_ERROR);

    }
}

void displaySendSignal() {
    if (char_lcd && getLCDRows() > 2) {
        char_lcd->setCursor(19, 3);
        char_lcd->write(8);
    }
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
    bool backlight = true;
    if (sntp_time_is_set && backlight_start < 25 && backlight_stop < 25) {
        time_t rawtime;
        struct tm *timeinfo;
        time (&rawtime);
        timeinfo = localtime(&rawtime);


        backlight = hourIsInRange(timeinfo->tm_hour, backlight_start, backlight_stop);

    }
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
    if (BME280::isEnabled()) {
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
    if (cfg::dht_read || cfg::ds18b20_read) {
        screens[static_screen_count++] = DisplayPages::PageTemp;
    }
    if (cfg::gps_read) {
        screens[static_screen_count++] = DisplayPages::PageGPS;
    }
    if (cfg::show_wifi_info) {
        screens[static_screen_count++] = DisplayPages::PageWIFI;    // Wifi info
    }
    if (cfg::sh_dev_inf) {
        screens[static_screen_count++] = DisplayPages::PageInfo;    // chipID, firmware and count of measurements
    }
    bool skipOldDisplay = (static_screen_count == 0);
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
                display->drawString(64, 52, displayGenerateFooter(static_screen_count + scheduler.countScreens()));
                display->display();
            }
            if (char_lcd) {
                char_lcd->clear();
                char_lcd->setBacklight(backlight);
                for (byte i = 0; i < 4; i++) {
                    char_lcd->setCursor(0,i);
                    if (i==0) char_lcd->print(getLCDHeader(getLCDRows()==4));
                    char_lcd->print(lines[i]);
                }
                displayProgressBar();
            }
            next_display_count++;
            skipOldDisplay = true;
        }


    }
    if (!skipOldDisplay) {

        if (cfg::has_display || cfg::has_lcd2004) {
            switch (screens[next_display_count % static_screen_count]) {
                case (DisplayPages::PagePM):
                    display_header = pm25_sensor;
                    if (pm25_sensor != pm10_sensor) {
                        display_header.concat(F(" / "));
                        display_header.concat(pm25_sensor);
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
                display->drawString(64, 52, displayGenerateFooter(static_screen_count + scheduler.countScreens()));
                display->display();
            }
            if (cfg::has_lcd2004) {
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
                displayProgressBar();
            }
        }


        if (cfg::has_lcd1602) {
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
    if (cfg::has_lcd1602) return 16;
    if (cfg::has_lcd2004) return 20;
    return 0;
};

byte getLCDRows(){
    if (cfg::has_lcd1602) return 2;
    if (cfg::has_lcd2004 || display) return 4;
    return 0;
};

byte getLCDaddr() {
    Wire.beginTransmission(0x27);
    if (Wire.endTransmission() == 0) {
        return 0x27;
    }
    Wire.beginTransmission(0x3F);
    if (Wire.endTransmission() == 0) {
        return 0x3F;
    }
    return 0;   //no I2C LCD?
}

String getLCDHeader(bool longDisp) {
    String ret = String(next_display_count + 1);
    if (longDisp)
        ret += F("/");
    ret += String(static_screen_count + scheduler.countScreens());
    ret += F(" ");
    return ret;
};

void cycleDisplay(void){
    if ((cfg::has_display || cfg::has_lcd2004 || cfg::has_lcd1602) && (act_milli > next_display_millis)) {
        display_values();
    }

}