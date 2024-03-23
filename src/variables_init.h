//
// Created by viciu on 28.01.2020.
//

#ifndef NAMF_VARIABLES_INIT_H
#define NAMF_VARIABLES_INIT_H
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
    char *wlanssid = nullptr;
    char *wlanpwd = nullptr;

    char *fbssid = nullptr;
    char *fbpwd = nullptr;

    bool in_factory_reset_window = true;

    char current_lang[3] = "PL";
    char *www_username = nullptr;
    char *www_password = nullptr;
    bool www_basicauth_enabled = WWW_BASICAUTH_ENABLED;

    char *fs_ssid = nullptr;
    char *fs_pwd = nullptr;

    bool internet = false;

#ifdef NAM_LORAWAN
    bool lw_en = false;
    String lw_d_eui = F("");
    String lw_a_eui = F("");
    String lw_app_key = F("");
//     String lw_nws_key = F("");
//     String lw_apps_key = F("");
//     String lw_dev_addr = F("");
#endif

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
    bool send2aqi = SEND2AQI;
    bool send2lora = SEND2LORA;
    bool send2influx = SEND2INFLUX;
    bool send2csv = SEND2CSV;
    bool auto_update = AUTO_UPDATE;
    bool has_display = HAS_DISPLAY;
    bool has_lcd1602 = HAS_LCD1602;
    bool has_lcd2004 = HAS_LCD2004_27;
    bool show_wifi_info = SHOW_WIFI_INFO;
    bool sh_dev_inf = SHOW_DEVICE_INFO;
    bool has_ledbar_32 = HAS_LEDBAR_32;
    float outputPower = TX_OUTPUT_POWER;
    int phyMode = PHY_MODE;
    int debug = DEBUG;
    bool send_diag = true;

    bool ssl_madavi = SSL_MADAVI;
    bool ssl_dusti = SSL_DUSTI;
    bool ssl_influx = SSL_INFLUX;
    char senseboxid[30] = SENSEBOXID;

    int port_influx = PORT_INFLUX;
    char *user_influx = nullptr;
    char *pwd_influx = nullptr;

    String host_custom = FPSTR(HOST_CUSTOM);
    String url_custom = FPSTR(URL_CUSTOM);
    int port_custom = PORT_CUSTOM;
    char *user_custom = nullptr;
    char *pwd_custom = nullptr;

    String token_AQI = FPSTR(EMPTY_STRING);
    String assign_token_AQI = FPSTR(EMPTY_STRING);


    String host_influx = FPSTR(HOST_INFLUX);
    String url_influx = FPSTR(URL_INFLUX);

    String UUID;


    unsigned long time_for_wifi_config = 600000;
    unsigned long sending_intervall_ms = 145000;

    byte update_channel = UPDATE_CHANNEL_STABLE;
}

//enum class PmSensorCmd {
//    Start,
//    Stop,
//    ContinuousMode,
//    VersionDate
//};

long int sample_count = 0;
bool bme280_init_failed = false;
bool heca_init_failed = false;

#if defined(ARDUINO_ARCH_ESP8266)
ESP8266WebServer server(80);
#else
WebServer server(80);
#endif
int TimeZone = 1;

/*****************************************************************
 * Display definitions                                           *
 *****************************************************************/
SSD1306 *display = nullptr;//(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
LiquidCrystal_I2C *char_lcd = nullptr;//(0x27, 16, 2);
//backlight enabled
byte backlight_stop = 25;
byte backlight_start = 25;

/*****************************************************************
 * SDS011 declarations                                           *
 *****************************************************************/
EspSoftwareSerial::UART serialSDS;//(PM_SERIAL_RX, PM_SERIAL_TX, false);
EspSoftwareSerial::UART serialGPS;//(GPS_SERIAL_RX, GPS_SERIAL_TX, false);

/*****************************************************************
 * DS18B20 declaration                                            *
 *****************************************************************/
OneWire oneWire(ONEWIRE_PIN);
DallasTemperature ds18b20(&oneWire);

/*****************************************************************
 * GPS declaration                                               *
 *****************************************************************/
TinyGPSPlus gps;


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
unsigned int last_value_WINSEN_CO2 = 0;
double last_value_DS18B20_T = -1.0;
double last_value_GPS_lat = -200.0;
double last_value_GPS_lon = -200.0;
double last_value_GPS_alt = -1000.0;
String last_value_GPS_date = "";
String last_value_GPS_time = "";
String last_data_string = "";

#if defined(ARDUINO_ARCH_ESP8266)
String esp_chipid() {return String(ESP.getChipId());};
#else
String esp_chipid() {return String(ESP.getEfuseMac());}

#endif
long last_page_load = millis();

bool wificonfig_loop = false;
unsigned long wificonfig_loop_update = 0;

bool first_cycle = true;

bool sntp_time_is_set = false;

bool got_ntp = false;

unsigned long enable_ota_time = 0;

unsigned long count_sends = 0;

struct struct_wifiInfo *wifiInfo;
int count_wifiInfo = -1;

template<typename T, std::size_t N> constexpr std::size_t array_num_elements(const T(&)[N]) {
    return N;
}



const char data_first_part[] PROGMEM = "{\"software_version\": \"{v}\", \"sensordatavalues\":[";

memory_stat_t memoryStatsMax;
memory_stat_t memoryStatsMin;

#endif //NAMF_VARIABLES_INIT_H
