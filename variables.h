//
// Created by viciu on 13.01.2020.
//

#ifndef AIRROHR_FIRMWARE_VARIABLES_H
#define AIRROHR_FIRMWARE_VARIABLES_H

#include <SoftwareSerial.h>

/******************************************************************
 * Constants                                                      *
 ******************************************************************/
const unsigned long SAMPLETIME_MS = 30000;
const unsigned long SAMPLETIME_SDS_MS = 1000;
const unsigned long WARMUPTIME_SDS_MS = 15000;
const unsigned long READINGTIME_SDS_MS = 5000;
const unsigned long SAMPLETIME_GPS_MS = 50;
const unsigned long DISPLAY_UPDATE_INTERVAL_MS = 5000;
const unsigned long ONE_DAY_IN_MS = 24 * 60 * 60 * 1000;
const unsigned long PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS = ONE_DAY_IN_MS;        // check for firmware updates once a day
const unsigned long DURATION_BEFORE_FORCED_RESTART_MS = ONE_DAY_IN_MS * 28;  // force a reboot every ~4 weeks

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
    char wlanssid[35] = WLANSSID;
    char wlanpwd[65] = WLANPWD;

    char current_lang[3] = "PL";
    char www_username[65] = WWW_USERNAME;
    char www_password[65] = WWW_PASSWORD;
    bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;

    char fs_ssid[33] = FS_SSID;
    char fs_pwd[65] = FS_PWD;

    char version_from_local_config[20] = "";

    bool dht_read = DHT_READ;
    bool sds_read = SDS_READ;
    bool pms_read = PMS_READ;
    bool bmp280_read = BMP280_READ;
    bool bme280_read = BME280_READ;
    bool heca_read = HECA_READ;
    bool ds18b20_read = DS18B20_READ;
    bool gps_read = GPS_READ;
    bool send2dusti = SEND2DUSTI;
    bool send2madavi = SEND2MADAVI;
    bool send2sensemap = SEND2SENSEMAP;
    bool send2fsapp = SEND2FSAPP;
    bool send2custom = SEND2CUSTOM;
    bool send2lora = SEND2LORA;
    bool send2influx = SEND2INFLUX;
    bool send2csv = SEND2CSV;
    bool auto_update = AUTO_UPDATE;
    bool use_beta = USE_BETA;
    bool has_display = HAS_DISPLAY;
    bool has_lcd1602 = HAS_LCD1602;
    bool has_lcd1602_27 = HAS_LCD1602_27;
    bool has_lcd2004_27 = HAS_LCD2004_27;
    bool has_lcd2004_3f = HAS_LCD2004_3F;
    int  debug = DEBUG;

    bool ssl_madavi = SSL_MADAVI;
    bool ssl_dusti = SSL_DUSTI;
    char senseboxid[30] = SENSEBOXID;

    int port_influx = PORT_INFLUX;
    char user_influx[65] = USER_INFLUX;
    char pwd_influx[65] = PWD_INFLUX;

    char host_custom[100] = HOST_CUSTOM;
    char url_custom[100] = URL_CUSTOM;
    int port_custom = PORT_CUSTOM;
    char user_custom[65] = USER_CUSTOM;
    char pwd_custom[65] = PWD_CUSTOM;

    char host_influx[100] = HOST_INFLUX;
    char url_influx[100] = URL_INFLUX;

    unsigned long time_for_wifi_config = 600000;
    unsigned long sending_intervall_ms = 145000;

    void initNonTrivials(const char* id) {
        strcpy(cfg::current_lang, CURRENT_LANG);
        if (fs_ssid[0] == '\0') {
            strcpy(fs_ssid, "NAM-");
            strcat(fs_ssid, id);
        }
    }
}

#define HOST_MADAVI "api-rrd.madavi.de"
#define URL_MADAVI "/data.php"
#define PORT_MADAVI 80

#define HOST_DUSTI "api.luftdaten.info"
#define URL_DUSTI "/v1/push-sensor-data/"
#define PORT_DUSTI 80

// IMPORTANT: NO MORE CHANGES TO VARIABLE NAMES NEEDED FOR EXTERNAL APIS

#define HOST_SENSEMAP "ingress.opensensemap.org"
#define URL_SENSEMAP "/boxes/BOXID/data?luftdaten=1"
#define PORT_SENSEMAP 443

#define HOST_FSAPP "www.h2801469.stratoserver.net"
#define URL_FSAPP "/data.php"
#define PORT_FSAPP 80

#define UPDATE_HOST "fw.air.nettigo.pl"
#define UPDATE_URL "/update/index.php"
#define UPDATE_PORT 80

#define JSON_BUFFER_SIZE 2000



enum class PmSensorCmd {
    Start,
    Stop,
    ContinuousMode,
    VersionDate
};

String basic_auth_influx = "";
String basic_auth_custom = "";

long int sample_count = 0;
bool bmp280_init_failed = false;
bool bme280_init_failed = false;
bool heca_init_failed = false;

ESP8266WebServer server(80);
int TimeZone = 1;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
SSD1306 display(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
LiquidCrystal_I2C lcd_1602_27(0x27, 16, 2);
LiquidCrystal_I2C lcd_1602_3f(0x3F, 16, 2);
LiquidCrystal_I2C lcd_2004_27(0x27, 20, 4);
LiquidCrystal_I2C lcd_2004_3f(0x3F, 20, 4);

/*****************************************************************
 * SDS011 declarations                                           *
 *****************************************************************/
SoftwareSerial serialSDS(PM_SERIAL_RX, PM_SERIAL_TX, false);
SoftwareSerial serialGPS(GPS_SERIAL_RX, GPS_SERIAL_TX, false);

/*****************************************************************
 * DHT declaration                                               *
 *****************************************************************/
DHT dht(ONEWIRE_PIN, DHT_TYPE);

/*****************************************************************
 * BMP280 declaration                                               *
 *****************************************************************/
Adafruit_BMP280 bmp280;

/*****************************************************************
 * BME280 declaration                                            *
 *****************************************************************/
Adafruit_BME280 bme280;

/*****************************************************************
 * HECA (SHT30) declaration                                            *
 *****************************************************************/
ClosedCube_SHT31D heca;

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
OneWire oneWire(ONEWIRE_PIN);
DallasTemperature ds18b20(&oneWire);

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
TinyGPSPlus gps;

boolean trigP1 = false;
boolean trigP2 = false;
unsigned long trigOnP1;
unsigned long trigOnP2;

unsigned long lowpulseoccupancyP1 = 0;
unsigned long lowpulseoccupancyP2 = 0;

bool send_now = false;
unsigned long starttime;
unsigned long time_point_device_start_ms;
unsigned long starttime_SDS;
unsigned long starttime_GPS;
unsigned long act_micro;
unsigned long act_milli;
unsigned long last_micro = 0;
unsigned long min_micro = 1000000000;
unsigned long max_micro = 0;

bool is_SDS_running = true;
bool is_PMS_running = true;

unsigned long sending_time = 0;
unsigned long last_update_attempt;

int sds_pm10_sum = 0;
int sds_pm25_sum = 0;
int sds_val_count = 0;
int sds_pm10_max = 0;
int sds_pm10_min = 20000;
int sds_pm25_max = 0;
int sds_pm25_min = 20000;

int pms_pm1_sum = 0;
int pms_pm10_sum = 0;
int pms_pm25_sum = 0;
int pms_val_count = 0;
int pms_pm1_max = 0;
int pms_pm1_min = 20000;
int pms_pm10_max = 0;
int pms_pm10_min = 20000;
int pms_pm25_max = 0;
int pms_pm25_min = 20000;

double last_value_SDS_P1 = -1.0;
double last_value_SDS_P2 = -1.0;
double last_value_PMS_P0 = -1.0;
double last_value_PMS_P1 = -1.0;
double last_value_PMS_P2 = -1.0;
double last_value_DHT_T = -128.0;
double last_value_DHT_H = -1.0;
double last_value_BMP280_T = -128.0;
double last_value_BMP280_P = -1.0;
double last_value_BME280_T = -128.0;
double last_value_BME280_H = -1.0;
double last_value_BME280_P = -1.0;
double last_value_HECA_T = -128.0;
double last_value_HECA_H = -1.0;
double last_value_DS18B20_T = -1.0;
double last_value_GPS_lat = -200.0;
double last_value_GPS_lon = -200.0;
double last_value_GPS_alt = -1000.0;
String last_value_GPS_date = "";
String last_value_GPS_time = "";
String last_data_string = "";

String esp_chipid;

long last_page_load = millis();

bool wificonfig_loop = false;

bool first_cycle = true;

bool sntp_time_is_set = false;

bool got_ntp = false;

unsigned long count_sends = 0;
unsigned long next_display_millis = 0;
unsigned long next_display_count = 0;

struct struct_wifiInfo {
    char ssid[35];
    uint8_t encryptionType;
    int32_t RSSI;
    int32_t channel;
    bool isHidden;
};

struct struct_wifiInfo *wifiInfo;
uint8_t count_wifiInfo;

template<typename T, std::size_t N> constexpr std::size_t array_num_elements(const T(&)[N]) {
    return N;
}

template<typename T, std::size_t N> constexpr std::size_t capacity_null_terminated_char_array(const T(&)[N]) {
    return N - 1;
}

#define msSince(timestamp_before) (act_milli - (timestamp_before))

const char data_first_part[] PROGMEM = "{\"software_version\": \"{v}\", \"sensordatavalues\":[";


#endif //AIRROHR_FIRMWARE_VARIABLES_H
