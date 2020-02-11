//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_VARIABLES_H
#define AIRROHR_FIRMWARE_VARIABLES_H

#if defined(BOOT_FW)
#define SOFTWARE_VERSION  "NAMF-2020-boot"
#else
#define SOFTWARE_VERSION  "NAMF-2020-7"
#endif
#include "defines.h"
#include <SoftwareSerial.h>
#include <ESP8266WebServer.h>
#include "./oledfont.h"				// avoids including the default Arial font, needs to be included before SSD1306.h
#include <SSD1306.h>
#include <LiquidCrystal_I2C.h>

#include "ClosedCube_SHT31D.h" // support for Nettigo Air Monitor HECA
#include <DallasTemperature.h>
#include <TinyGPS++.h>

#include "ext_def.h"
#if defined(INTL_EN)
#include "lang/intl_en.h"
#elif defined(INTL_PL)
#include "lang/intl_pl.h"
#elif defined(INTL_HU)
#include "lang/intl_hu.h"
#else
#include "lang/intl_en.h"
#endif

/******************************************************************
 * The variables inside the cfg namespace are persistent          *
 * configuration values. They have defaults which can be          *
 * configured at compile-time via the ext_def.h file              *
 * They can be changed by the user via the web interface, the     *
 * changes are persisted to the flash and read back after reboot. *
 * Note that the names of these variables can't be easily changed *
 * as they are part of the json format used to persist the data.  *
 ******************************************************************/
namespace cfg {
    extern char wlanssid[35];
    extern char wlanpwd[65];

    extern char current_lang[3];
    extern char www_username[65];
    extern char www_password[65];
    extern bool www_basicauth_enabled;

    extern char fs_ssid[33];
    extern char fs_pwd[65];

    extern char version_from_local_config[20];

    extern bool dht_read;
    extern bool sds_read;
    extern bool pms_read;
    extern bool bmp280_read;
    extern bool bme280_read;
    extern bool heca_read;
    extern bool ds18b20_read;
    extern bool gps_read;
    extern bool send2dusti;
    extern bool send2madavi;
    extern bool send2sensemap;
    extern bool send2fsapp;
    extern bool send2custom;
    extern bool send2lora;
    extern bool send2influx;
    extern bool send2csv;
    extern bool auto_update;
    extern bool use_beta;
    extern bool has_display;
    extern bool has_lcd1602;
    extern bool has_lcd1602_27;
    extern bool has_lcd2004_27;
    extern bool has_lcd2004_3f;
    extern int  debug;

    extern bool ssl_madavi;
    extern bool ssl_dusti ;
    extern char senseboxid[30];

    extern int port_influx;
    extern char user_influx[65];
    extern char pwd_influx[65];

    extern char host_custom[100];
    extern char url_custom[100];
    extern int port_custom;
    extern char user_custom[65];
    extern char pwd_custom[65];

    extern char host_influx[100];
    extern char url_influx[100];

    extern unsigned long time_for_wifi_config;
    extern unsigned long sending_intervall_ms;

    extern void initNonTrivials(const char* id);
}




enum class PmSensorCmd;
extern String basic_auth_influx ;
extern String basic_auth_custom ;

extern long int sample_count ;
extern bool bmp280_init_failed ;
extern bool bme280_init_failed ;
extern bool heca_init_failed ;

extern ESP8266WebServer server;
extern int TimeZone ;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
extern SSD1306 display;
extern LiquidCrystal_I2C lcd_1602_27;
extern LiquidCrystal_I2C lcd_1602_3f;
extern LiquidCrystal_I2C lcd_2004_27;
extern LiquidCrystal_I2C lcd_2004_3f;

/*****************************************************************
 * SDS011 declarations                                           *
 *****************************************************************/
extern SoftwareSerial serialSDS;
extern SoftwareSerial serialGPS;

/*****************************************************************
 * HECA (SHT30) declaration                                            *
 *****************************************************************/
extern ClosedCube_SHT31D heca;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
extern OneWire oneWire;
extern DallasTemperature ds18b20;

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
extern TinyGPSPlus gps;

extern boolean trigP1 ;
extern boolean trigP2 ;
extern unsigned long trigOnP1;
extern unsigned long trigOnP2;

extern unsigned long lowpulseoccupancyP1 ;
extern unsigned long lowpulseoccupancyP2 ;

extern bool send_now ;
extern unsigned long starttime;
extern unsigned long time_point_device_start_ms;
extern unsigned long starttime_SDS;
extern unsigned long starttime_GPS;
extern unsigned long act_micro;
extern unsigned long act_milli;
extern unsigned long last_micro ;
extern unsigned long min_micro ;
extern unsigned long max_micro ;

extern bool is_SDS_running ;
extern bool is_PMS_running ;

extern unsigned long sending_time ;
extern unsigned long last_update_attempt;

extern int sds_pm10_sum ;
extern int sds_pm25_sum ;
extern int sds_val_count ;
extern int sds_pm10_max ;
extern int sds_pm10_min ;
extern int sds_pm25_max ;
extern int sds_pm25_min ;

extern int pms_pm1_sum ;
extern int pms_pm10_sum ;
extern int pms_pm25_sum ;
extern int pms_val_count ;
extern int pms_pm1_max ;
extern int pms_pm1_min ;
extern int pms_pm10_max ;
extern int pms_pm10_min ;
extern int pms_pm25_max ;
extern int pms_pm25_min ;

extern double last_value_SDS_P1 ;
extern double last_value_SDS_P2 ;
extern double last_value_PMS_P0 ;
extern double last_value_PMS_P1 ;
extern double last_value_PMS_P2 ;
extern double last_value_DHT_T ;
extern double last_value_DHT_H ;
extern double last_value_BMP280_T ;
extern double last_value_BMP280_P ;
extern double last_value_BME280_T ;
extern double last_value_BME280_H ;
extern double last_value_BME280_P ;
extern double last_value_HECA_T ;
extern double last_value_HECA_H ;
extern double last_value_DS18B20_T ;
extern double last_value_GPS_lat ;
extern double last_value_GPS_lon ;
extern double last_value_GPS_alt ;
extern String last_value_GPS_date ;
extern String last_value_GPS_time ;
extern String last_data_string ;

extern String esp_chipid;

extern long last_page_load ;

extern bool wificonfig_loop ;

extern bool first_cycle ;

extern bool sntp_time_is_set ;

extern bool got_ntp ;

extern unsigned long count_sends ;
extern unsigned long next_display_millis ;
extern unsigned long next_display_count ;

extern struct struct_wifiInfo *wifiInfo;
extern uint8_t count_wifiInfo;



extern const char data_first_part[] ;


#endif //AIRROHR_FIRMWARE_VARIABLES_H
