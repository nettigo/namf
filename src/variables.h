//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_VARIABLES_H
#define AIRROHR_FIRMWARE_VARIABLES_H

#if defined(BOOT_FW)
#define SOFTWARE_VERSION  "NAMF-2020-boot"
#define SOFTWARE_VERSION_SHORT "boot"
#else
#define SOFTWARE_VERSION  "NAMF-47rc1"
#define SOFTWARE_VERSION_SHORT "47rc1"
// undefine SOFTWARE_BETA in production releases
#define SOFTWARE_BETA  1
#endif
#include "defines.h"
#include "system/scheduler.h"
#include <SoftwareSerial.h>
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include "./oledfont.h"				// avoids including the default Arial font, needs to be included before SSD1306.h
#include <SSD1306.h>
#include <LiquidCrystal_I2C.h>

#include "ClosedCube_SHT31D.h" // support for Nettigo Air Monitor HECA
#include <DallasTemperature.h>
#include <TinyGPS++.h>

#include "ext_def.h"
#include "lang/select_lang.h"
extern SimpleScheduler::NAMFScheduler scheduler;
struct apiTimeStat{
    uint8_t id;
    int16_t status;
    uint32_t time;
};

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
    extern char *wlanssid;
    extern char *wlanpwd;

    //fallback SSID & pwd
    extern char *fbssid;
    extern char *fbpwd;

    extern bool in_factory_reset_window;

    extern char current_lang[3];
    extern char *www_username;
    extern char *www_password;
    extern bool www_basicauth_enabled;

    extern char *fs_ssid;
    extern char *fs_pwd;

    //do we have connectivity over internet?
    extern bool internet;

#ifdef NAM_LORAWAN
    extern bool lw_en;
    extern String lw_d_eui;
    extern String lw_a_eui;
    extern String lw_app_key;
//    extern String lw_nws_key;
//    extern String lw_apps_key;
//    extern String lw_dev_addr;
#endif

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
    extern bool send2aqi;
    extern bool send2lora;
    extern bool send2influx;
    extern bool send2csv;
    extern byte apiCount;
    extern apiTimeStat *apiStats;
    extern bool auto_update;
    extern u8_t update_channel;
    extern bool has_display;
    extern bool has_lcd1602;
    extern bool has_lcd2004;
    extern bool show_wifi_info;
    extern bool sh_dev_inf;
    extern bool has_ledbar_32;
    extern float outputPower;
    extern int phyMode; 
    extern int  debug;

    extern bool send_diag;

    extern bool ssl_madavi;
    extern bool ssl_dusti ;
    extern char senseboxid[30];

    extern int port_influx;
    extern char *user_influx;
    extern char *pwd_influx;

    extern String host_custom;
    extern String url_custom;
    extern int port_custom;
    extern char *user_custom;
    extern char *pwd_custom;

    extern String token_AQI;
    extern String assign_token_AQI;

    extern String host_influx;
    extern String url_influx;

    extern String UUID;
    extern unsigned long time_for_wifi_config;
    extern unsigned long sending_intervall_ms;

    extern void initNonTrivials(const char* id);
}




typedef enum  {
    Start,
    Stop,
    ContinuousMode,
    ContinuousMode2,
    VersionDate,
    None
}PmSensorCmd;

extern char *basic_auth_influx ;
extern char *basic_auth_custom ;

extern long int sample_count ;
extern bool bme280_init_failed ;
extern bool heca_init_failed ;
#if defined(ARDUINO_ARCH_ESP8266)
extern ESP8266WebServer server;
#else
extern WebServer server;
#endif
extern int TimeZone ;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
extern SSD1306 * display;
extern LiquidCrystal_I2C * char_lcd;
extern byte backlight_stop;
extern byte backlight_start;
/*****************************************************************
 * SDS011 declarations                                           *
 *****************************************************************/
extern EspSoftwareSerial::UART serialSDS;
extern EspSoftwareSerial::UART serialGPS;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
extern OneWire oneWire;
extern DallasTemperature ds18b20;

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
extern TinyGPSPlus gps;

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
extern unsigned int last_value_WINSEN_CO2 ;
extern double last_value_DS18B20_T ;
extern double last_value_GPS_lat ;
extern double last_value_GPS_lon ;
extern double last_value_GPS_alt ;
extern String last_value_GPS_date ;
extern String last_value_GPS_time ;
extern String last_data_string ;

extern String esp_chipid();

extern long last_page_load ;

extern bool wificonfig_loop ;
extern unsigned long wificonfig_loop_update ;

extern bool first_cycle ;

extern bool sntp_time_is_set ;

extern bool got_ntp ;

extern unsigned long enable_ota_time ;

extern unsigned long count_sends ;

struct struct_wifiInfo {
    char ssid[35];
    uint8_t encryptionType;
    int32_t RSSI;
    int32_t channel;
    bool isHidden;
};
extern struct struct_wifiInfo *wifiInfo;
extern int count_wifiInfo;



extern const char data_first_part[] ;
extern memory_stat_t memoryStatsMax;
extern memory_stat_t memoryStatsMin;


class LoggingSerial : public HardwareSerial {

public:
    LoggingSerial();
    size_t write(uint8_t c) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    String popLines();
    void stopWebCopy(void);    //stop copying data to buffer available via network (passwords)
    void resumeWebCopy(void);   // enable copying data

private:
    std::unique_ptr<circular_queue<uint8_t> > m_buffer;
    bool skipBuffer;

};

extern class LoggingSerial Debug;
#endif //AIRROHR_FIRMWARE_VARIABLES_H
