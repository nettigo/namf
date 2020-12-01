#ifndef NAMF_EXT_DEF_H
#define NAMF_EXT_DEF_H
// Language config
#define CURRENT_LANG INTL_LANG

// Wifi config
#define WLANSSID ""
#define WLANPWD ""

// BasicAuth config
#define WWW_USERNAME "admin"
#define WWW_PASSWORD "admin"
#define WWW_BASICAUTH_ENABLED 0

// Initial name

#define INITIAL_NAME_PART "NAM-"

// Sensor Wifi config (config mode)
#define FS_SSID ""
#define FS_PWD ""

// Wohin gehen die Daten?
#define SEND2DUSTI 1
#define SSL_DUSTI 0
#define SEND2MADAVI 1
#define SSL_MADAVI 0
#define SEND2SENSEMAP 0
#define SEND2FSAPP 0
#define SEND2MQTT 0
#define SEND2INFLUX 0
#define SEND2LORA 0
#define SEND2CSV 0
#define SEND2CUSTOM 0

// NTP Server
#define NTP_SERVER "0.europe.pool.ntp.org"

// OpenSenseMap
#define SENSEBOXID ""

enum LoggerEntry
{
	LoggerDusti,
	LoggerMadavi,
	LoggerSensemap,
	LoggerFSapp,
	LoggerInflux,
	LoggerCustom
};

// IMPORTANT: NO MORE CHANGES TO VARIABLE NAMES NEEDED FOR EXTERNAL APIS

// Definition eigene API
const char HOST_CUSTOM[] PROGMEM = "192.168.234.1";
const char URL_CUSTOM[] PROGMEM = "/data.php";
#define PORT_CUSTOM 80
#define USER_CUSTOM ""
#define PWD_CUSTOM ""

// Definition eigene InfluxDB
const char HOST_INFLUX[] PROGMEM = "influx.server";
const char URL_INFLUX[] PROGMEM = "/write?db=luftdaten";
#define PORT_INFLUX 8086
#define USER_INFLUX ""
#define PWD_INFLUX ""

// Definitions for MQTT
const char HOST_MQTT[] PROGMEM = "mqtt.server";
const char SENSORS_TOPIC_MQTT[] PROGMEM = "/write?db=luftdaten";
#define PORT_MQTT 1883
#define USER_MQTT ""
#define PWD_MQTT ""
#define CLIENT_ID_MQTT ""
#define SENSOR_TOPIC_PREFIX_MQTT ""

// define pins for I2C
#define I2C_PIN_SCL D4
#define I2C_PIN_SDA D3

// define pin for one wire sensors
#if defined(ESP8266)
#define ONEWIRE_PIN D7
#endif
#if defined(ARDUINO_SAMD_ZERO)
#define ONEWIRE_PIN D7
#endif

// define serial interface pins for particle sensors
// Serial confusion: These definitions are based on SoftSerial
// TX (transmitting) pin on one side goes to RX (receiving) pin on other side
// SoftSerial RX PIN is D1 and goes to SDS TX
// SoftSerial TX PIN is D2 and goes to SDS RX
#if defined(ESP8266)
#define PM_SERIAL_RX D1
#define PM_SERIAL_TX D2
#endif

// define serial interface pins for GPS modules
#if defined(ESP8266)
#define GPS_SERIAL_RX D5
#define GPS_SERIAL_TX D6
#endif

// DHT22, temperature, humidity
#define DHT_READ 0
#define DHT_TYPE DHT22
#define DHT_API_PIN 7

// SDS011, der etwas teuerere Feinstaubsensor
#define SDS_READ 1
#define SDS_API_PIN 1

// PMS1003, PMS300, 3PMS5003, PMS6003, PMS7003
#define PMS_READ 0
#define PMS_API_PIN 1

// BMP280, temperature, pressure
#define BMP280_READ 0
#define BMP280_API_PIN 3

// BME280, temperature, humidity, pressure
#define BME280_READ 1
#define BME280_API_PIN 11

// HECA (SHT30), temperature, pressure
#define HECA_READ 1
#define HECA_API_PIN 7

#define WINSEN_MHZ14A_READ 0
#define WINSEN_MHZ14A_API 9

// DS18B20, temperature
#define DS18B20_READ 0
#define DS18B20_API_PIN 13


// GPS, bevorzugt Neo-6M
#define GPS_READ 0
#define GPS_API_PIN 9

// automatic firmware updates
#define AUTO_UPDATE 1

// use beta firmware
#define USE_BETA 0

// OLED Display SSD1306 angeschlossen?
#define HAS_DISPLAY 0

// LCD Display LCD1602 angeschlossen?
#define HAS_LCD1602 0

// LCD Display LCD1602 (0x27) angeschlossen?
#define HAS_LCD1602_27 0

// LCD Display LCD2004 (0x27) angeschlossen?
#define HAS_LCD2004_27 1

// LCD Display LCD2004 (0x3F)
#define HAS_LCD2004_3F 0

// RGB LED BAR (0x32)
#define HAS_LEDBAR_32 0

// show wifi info on LCD display
#define SHOW_WIFI_INFO 1

//default TX trasnmit power
#define TX_OUTPUT_POWER 20.5

//default WiFi PHY MODE
#define PHY_MODE WIFI_PHY_MODE_11N;

// Wieviele Informationen sollen über die serielle Schnittstelle ausgegeben werden?
#define DEBUG 3

// Definition der Debuglevel
#define DEBUG_ERROR 1
#define DEBUG_WARNING 2
#define DEBUG_MIN_INFO 3
#define DEBUG_MED_INFO 4
#define DEBUG_MAX_INFO 5

/*
static const uint16_t suites[] PROGMEM = {
	BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
	BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
	BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
	BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
	BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
	BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
};
*/

// Definition GPIOs for Zero based Arduino Feather M0 LoRaWAN
#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
// Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL
//GPIO Pins
#define D0 0
#define D1 1
#define D2 2
#define D3 3
#define D4 4
#define D5 5
#define D6 6
#define D7 7
#define D8 8
#define D9 9
#define D10 10
#define D11 11
#define D12 12
// LoRa module
#define RFM69_CS 8
#define RFM69_RST 4
#define RFM69_INT 3
#define RF69_FREQ 868.0
#define CLIENT_ADDRESS 2
#define SERVER_ADDRESS 100
#endif

#endif //NAMF_EXT_DEF_H