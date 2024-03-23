#include <Arduino.h>

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/
#include "defines.h"
#include "variables.h"
//#include <Schedule.h>
#include "variables_init.h"
#include "update.h"
#include "helpers.h"
#include "system/scheduler.h"
#include "system/components.h"
#include "wifi.h"

#ifdef NAM_LORAWAN
#include "lora/lorawan.h"
#endif
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
   struct FSInfo {
        size_t totalBytes;
        size_t usedBytes;
        size_t blockSize;
        size_t pageSize;
        size_t maxOpenFiles;
        size_t maxPathLength;
    };
#endif

#include <ArduinoOTA.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <time.h>
//#include <coredecls.h>
#include <assert.h>
#include "system/reporting.h"

#include "html-content.h"
#include "webserver.h"
#include "sending.h"
#include "sensors/sds011/sds011.h"
//#include "sensors/bme280.h"
#include "sensors/dht.h"
#include "display/commons.h"
#include "display/ledbar.h"


SimpleScheduler::NAMFScheduler scheduler;
unsigned long PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS;

/*****************************************************************
 * send Plantower PMS sensor command start, stop, cont. mode     *
 *****************************************************************/
static bool PMS_cmd(PmSensorCmd cmd) {
	static constexpr uint8_t start_cmd[] PROGMEM = {
		0x42, 0x4D, 0xE4, 0x00, 0x01, 0x01, 0x74
	};
	static constexpr uint8_t stop_cmd[] PROGMEM = {
		0x42, 0x4D, 0xE4, 0x00, 0x00, 0x01, 0x73
	};
	static constexpr uint8_t continuous_mode_cmd[] PROGMEM = {
		0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71
	};
	constexpr uint8_t cmd_len = array_num_elements(start_cmd);

	uint8_t buf[cmd_len];
	switch (cmd) {
	case PmSensorCmd::Start:
		memcpy_P(buf, start_cmd, cmd_len);
		break;
	case PmSensorCmd::Stop:
		memcpy_P(buf, stop_cmd, cmd_len);
		break;
	case PmSensorCmd::ContinuousMode:
		memcpy_P(buf, continuous_mode_cmd, cmd_len);
		break;
	case PmSensorCmd::VersionDate:
		assert(false && "not supported by this sensor");
		break;
	}
	serialSDS.write(buf, cmd_len);
	return cmd != PmSensorCmd::Stop;
}



/*****************************************************************
 * disable unneeded NMEA sentences, TinyGPS++ needs GGA, RMC     *
 *****************************************************************/
void disable_unneeded_nmea() {
	serialGPS.println(F("$PUBX,40,GLL,0,0,0,0*5C"));       // Geographic position, latitude / longitude
//	serialGPS.println(F("$PUBX,40,GGA,0,0,0,0*5A"));       // Global Positioning System Fix Data
	serialGPS.println(F("$PUBX,40,GSA,0,0,0,0*4E"));       // GPS DOP and active satellites
//	serialGPS.println(F("$PUBX,40,RMC,0,0,0,0*47"));       // Recommended minimum specific GPS/Transit data
	serialGPS.println(F("$PUBX,40,GSV,0,0,0,0*59"));       // GNSS satellites in view
	serialGPS.println(F("$PUBX,40,VTG,0,0,0,0*5E"));       // Track made good and ground speed
}

//This is meant to be run on first time. Not only sets default values for sensors, but also makes sure
//that pointers are properly inited
void setDefaultConfig(void) {
    //init
    debug_out(F("Set defaults"), DEBUG_MIN_INFO, 1);
    stringToChar(&cfg::www_username, FPSTR(WWW_USERNAME));
    stringToChar(&cfg::www_password, FPSTR(WWW_PASSWORD));
    stringToChar(&cfg::wlanssid, FPSTR(EMPTY_STRING));
    stringToChar(&cfg::wlanpwd, FPSTR(EMPTY_STRING));
    stringToChar(&cfg::fbssid, FPSTR(EMPTY_STRING));
    stringToChar(&cfg::fbpwd, FPSTR(EMPTY_STRING));
    stringToChar(&cfg::user_custom, FPSTR(USER_CUSTOM));
    stringToChar(&cfg::pwd_custom, FPSTR(PWD_CUSTOM));
    stringToChar(&cfg::pwd_influx, FPSTR(PWD_INFLUX));
    stringToChar(&cfg::user_influx, FPSTR(USER_INFLUX));
    SDS011::setDefaults();
    HECA::setDefaults();
//    BMPx80::setDefaults();
    BME280::setDefaults();
//    SHT3x::setDefaults();


}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/
void readConfig() {
    setDefaultConfig();
		if (SPIFFS.exists("/config.json")) {
			//file exists, reading and loading
			debug_out(F("reading config"), DEBUG_MED_INFO, 1);
			File configFile = SPIFFS.open("/config.json", "r");
            if (!readAndParseConfigFile(configFile)){
                //not failed opening & reading
                debug_out(F("Config parsed"), DEBUG_MIN_INFO, true);
            }else{
                debug_out(F("FAILED config parsing and loading"), DEBUG_ERROR, true);
                setDefaultConfig();
            };
		} else {
			debug_out(F("config file not found ..."), DEBUG_ERROR, 1);
		}
}

String tmpl(const String& patt, const String& value) {
	String s = patt;
	s.replace("{v}", value);
	return s;
}

String tmpl(const String& patt, const String& value1, const String& value2) {
	String s = patt;
	s.replace("{v1}", value1);
	s.replace("{v2}", value2);
	return s;
}

String tmpl(const String& patt, const String& value1, const String& value2, const String& value3) {
	String s = patt;
	s.replace("{v1}", value1);
	s.replace("{v2}", value2);
	s.replace("{v3}", value3);
	return s;
}



String add_sensor_type(const String& sensor_text) {
	String s = sensor_text;
	s.replace("{pm}", FPSTR(INTL_PARTICULATE_MATTER));
	s.replace("{t}", FPSTR(INTL_TEMPERATURE));
	s.replace("{h}", FPSTR(INTL_HUMIDITY));
	s.replace("{p}", FPSTR(INTL_PRESSURE));
	return s;
}





/*****************************************************************
 * read DS18B20 sensor values                                    *
 *****************************************************************/
static String sensorDS18B20() {
	double t;
	debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_DS18B20), DEBUG_MED_INFO, 1);

	//it's very unlikely (-127: impossible) to get these temperatures in reality. Most times this means that the sensor is currently faulty
	//try 5 times to read the sensor, otherwise fail
	const int MAX_ATTEMPTS = 5;
	int count = 0;
	do {
		ds18b20.requestTemperatures();
		//for now, we want to read only the first sensor
		t = ds18b20.getTempCByIndex(0);
		++count;
		debug_out(F("DS18B20 trying...."), DEBUG_MIN_INFO, 0);
		debug_out(String(count), DEBUG_MIN_INFO, 1);
	} while (count < MAX_ATTEMPTS && (isnan(t) || t >= 85.0 || t <= (-127.0)));

	String s;
	if (count == MAX_ATTEMPTS) {
		last_value_DS18B20_T = -128.0;
		debug_out(String(FPSTR(SENSORS_DS18B20)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
	} else {
		debug_out(FPSTR(DBG_TXT_TEMPERATURE), DEBUG_MIN_INFO, 0);
		debug_out(Float2String(t) + " C", DEBUG_MIN_INFO, 1);
		last_value_DS18B20_T = t;
		s += Value2Json(F("DS18B20_temperature"), Float2String(last_value_DS18B20_T));
	}
	debug_out("----", DEBUG_MIN_INFO, 1);
	debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_DS18B20), DEBUG_MED_INFO, 1);

	return s;
}
/*****************************************************************
 * read Plantronic PM sensor sensor values                       *
 *****************************************************************/
String sensorPMS() {
	String s = "";
	char buffer;
	int value;
	int len = 0;
	int pm1_serial = 0;
	int pm10_serial = 0;
	int pm25_serial = 0;
	int checksum_is = 0;
	int checksum_should = 0;
	int checksum_ok = 0;
	int frame_len = 24;				// min. frame length

	debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_PMSx003), DEBUG_MED_INFO, 1);
	if (msSince(starttime) < (cfg::sending_intervall_ms - (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS))) {
		if (is_PMS_running) {
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	} else {
		if (! is_PMS_running) {
			is_PMS_running = PMS_cmd(PmSensorCmd::Start);
		}

		while (serialSDS.available() > 0) {
			buffer = serialSDS.read();
			debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", DEBUG_MAX_INFO, 1);
//			"aa" = 170, "ab" = 171, "c0" = 192
			value = int(buffer);
			switch (len) {
			case (0):
				if (value != 66) {
					len = -1;
				};
				break;
			case (1):
				if (value != 77) {
					len = -1;
				};
				break;
			case (2):
				checksum_is = value;
				break;
			case (3):
				frame_len = value + 4;
				break;
			case (10):
				pm1_serial += ( value << 8);
				break;
			case (11):
				pm1_serial += value;
				break;
			case (12):
				pm25_serial = ( value << 8);
				break;
			case (13):
				pm25_serial += value;
				break;
			case (14):
				pm10_serial = ( value << 8);
				break;
			case (15):
				pm10_serial += value;
				break;
			case (22):
				if (frame_len == 24) {
					checksum_should = ( value << 8 );
				};
				break;
			case (23):
				if (frame_len == 24) {
					checksum_should += value;
				};
				break;
			case (30):
				checksum_should = ( value << 8 );
				break;
			case (31):
				checksum_should += value;
				break;
			}
			if ((len > 2) && (len < (frame_len - 2))) { checksum_is += value; }
			len++;
			if (len == frame_len) {
				debug_out(FPSTR(DBG_TXT_CHECKSUM_IS), DEBUG_MED_INFO, 0);
				debug_out(String(checksum_is + 143), DEBUG_MED_INFO, 0);
				debug_out(FPSTR(DBG_TXT_CHECKSUM_SHOULD), DEBUG_MED_INFO, 0);
				debug_out(String(checksum_should), DEBUG_MED_INFO, 1);
				if (checksum_should == (checksum_is + 143)) {
					checksum_ok = 1;
				} else {
					len = 0;
				};
				if (checksum_ok == 1 && (msSince(starttime) > (cfg::sending_intervall_ms - READINGTIME_SDS_MS))) {
					if ((! isnan(pm1_serial)) && (! isnan(pm10_serial)) && (! isnan(pm25_serial))) {
						pms_pm1_sum += pm1_serial;
						pms_pm10_sum += pm10_serial;
						pms_pm25_sum += pm25_serial;
						if (pms_pm1_min > pm10_serial) {
							pms_pm1_min = pm1_serial;
						}
						if (pms_pm1_max < pm10_serial) {
							pms_pm1_max = pm1_serial;
						}
						if (pms_pm10_min > pm10_serial) {
							pms_pm10_min = pm10_serial;
						}
						if (pms_pm10_max < pm10_serial) {
							pms_pm10_max = pm10_serial;
						}
						if (pms_pm25_min > pm25_serial) {
							pms_pm25_min = pm25_serial;
						}
						if (pms_pm25_max < pm25_serial) {
							pms_pm25_max = pm25_serial;
						}
						debug_out(F("PM1 (sec.): "), DEBUG_MED_INFO, 0);
						debug_out(Float2String(double(pm1_serial)), DEBUG_MED_INFO, 1);
						debug_out(F("PM2.5 (sec.): "), DEBUG_MED_INFO, 0);
						debug_out(Float2String(double(pm25_serial)), DEBUG_MED_INFO, 1);
						debug_out(F("PM10 (sec.) : "), DEBUG_MED_INFO, 0);
						debug_out(Float2String(double(pm10_serial)), DEBUG_MED_INFO, 1);
						pms_val_count++;
					}
					len = 0;
					checksum_ok = 0;
					pm1_serial = 0.0;
					pm10_serial = 0.0;
					pm25_serial = 0.0;
					checksum_is = 0;
				}
			}
			yield();
		}

	}
	if (send_now) {
		last_value_PMS_P0 = -1;
		last_value_PMS_P1 = -1;
		last_value_PMS_P2 = -1;
		if (pms_val_count > 2) {
			pms_pm1_sum = pms_pm1_sum - pms_pm1_min - pms_pm1_max;
			pms_pm10_sum = pms_pm10_sum - pms_pm10_min - pms_pm10_max;
			pms_pm25_sum = pms_pm25_sum - pms_pm25_min - pms_pm25_max;
			pms_val_count = pms_val_count - 2;
		}
		if (pms_val_count > 0) {
			last_value_PMS_P0 = double(pms_pm1_sum) / (pms_val_count * 1.0);
			last_value_PMS_P1 = double(pms_pm10_sum) / (pms_val_count * 1.0);
			last_value_PMS_P2 = double(pms_pm25_sum) / (pms_val_count * 1.0);
			debug_out("PM1:   " + Float2String(last_value_PMS_P0), DEBUG_MIN_INFO, 1);
			debug_out("PM2.5: " + Float2String(last_value_PMS_P2), DEBUG_MIN_INFO, 1);
			debug_out("PM10:  " + Float2String(last_value_PMS_P1), DEBUG_MIN_INFO, 1);
			debug_out("-------", DEBUG_MIN_INFO, 1);
			s += Value2Json("PMS_P0", Float2String(last_value_PMS_P0));
			s += Value2Json("PMS_P1", Float2String(last_value_PMS_P1));
			s += Value2Json("PMS_P2", Float2String(last_value_PMS_P2));
		}
		pms_pm1_sum = 0;
		pms_pm10_sum = 0;
		pms_pm25_sum = 0;
		pms_val_count = 0;
		pms_pm1_max = 0;
		pms_pm1_min = 20000;
		pms_pm10_max = 0;
		pms_pm10_min = 20000;
		pms_pm25_max = 0;
		pms_pm25_min = 20000;
		if (cfg::sending_intervall_ms > (WARMUPTIME_SDS_MS + READINGTIME_SDS_MS)) {
			is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
		}
	}

	debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_PMSx003), DEBUG_MED_INFO, 1);

	return s;
}

/*****************************************************************
 * read GPS sensor values                                        *
 *****************************************************************/
String sensorGPS() {
	String s = "";
	String gps_lat = "";
	String gps_lon = "";

	debug_out(String(FPSTR(DBG_TXT_START_READING)) + F("GPS"), DEBUG_MED_INFO, 1);

	while (serialGPS.available() > 0) {
		if (gps.encode(serialGPS.read())) {
			if (gps.location.isValid()) {
				last_value_GPS_lat = gps.location.lat();
				last_value_GPS_lon = gps.location.lng();
				gps_lat = Float2String(last_value_GPS_lat, 6);
				gps_lon = Float2String(last_value_GPS_lon, 6);
			} else {
				last_value_GPS_lat = -200;
				last_value_GPS_lon = -200;
				debug_out(F("Lat/Lng INVALID"), DEBUG_MAX_INFO, 1);
			}
			if (gps.altitude.isValid()) {
				last_value_GPS_alt = gps.altitude.meters();
				String gps_alt = Float2String(last_value_GPS_lat, 2);
			} else {
				last_value_GPS_alt = -1000;
				debug_out(F("Altitude INVALID"), DEBUG_MAX_INFO, 1);
			}
			if (gps.date.isValid()) {
				String gps_date = "";
				if (gps.date.month() < 10) {
					gps_date += "0";
				}
				gps_date += String(gps.date.month());
				gps_date += "/";
				if (gps.date.day() < 10) {
					gps_date += "0";
				}
				gps_date += String(gps.date.day());
				gps_date += "/";
				gps_date += String(gps.date.year());
				last_value_GPS_date = gps_date;
			} else {
				debug_out(F("Date INVALID"), DEBUG_MAX_INFO, 1);
			}
			if (gps.time.isValid()) {
				String gps_time = "";
				if (gps.time.hour() < 10) {
					gps_time += "0";
				}
				gps_time += String(gps.time.hour());
				gps_time += ":";
				if (gps.time.minute() < 10) {
					gps_time += "0";
				}
				gps_time += String(gps.time.minute());
				gps_time += ":";
				if (gps.time.second() < 10) {
					gps_time += "0";
				}
				gps_time += String(gps.time.second());
				gps_time += ".";
				if (gps.time.centisecond() < 10) {
					gps_time += "0";
				}
				gps_time += String(gps.time.centisecond());
				last_value_GPS_time = gps_time;
			} else {
				debug_out(F("Time: INVALID"), DEBUG_MAX_INFO, 1);
			}
		}
	}

	if (send_now) {
		debug_out("Lat/Lng: " + Float2String(last_value_GPS_lat, 6) + "," + Float2String(last_value_GPS_lon, 6), DEBUG_MIN_INFO, 1);
		debug_out("Altitude: " + Float2String(last_value_GPS_alt, 2), DEBUG_MIN_INFO, 1);
		debug_out("Date: " + last_value_GPS_date, DEBUG_MIN_INFO, 1);
		debug_out("Time " + last_value_GPS_time, DEBUG_MIN_INFO, 1);
		debug_out("----", DEBUG_MIN_INFO, 1);
		s += Value2Json(F("GPS_lat"), Float2String(last_value_GPS_lat, 6));
		s += Value2Json(F("GPS_lon"), Float2String(last_value_GPS_lon, 6));
		s += Value2Json(F("GPS_height"), Float2String(last_value_GPS_alt, 2));
		s += Value2Json(F("GPS_date"), last_value_GPS_date);
		s += Value2Json(F("GPS_time"), last_value_GPS_time);
	}

	if ( gps.charsProcessed() < 10) {
		debug_out(F("No GPS data received: check wiring"), DEBUG_ERROR, 1);
	}

	debug_out(String(FPSTR(DBG_TXT_END_READING)) + F("GPS"), DEBUG_MED_INFO, 1);

	return s;
}


/*****************************************************************
 * Init OLED display                                             *
 *****************************************************************/
void init_display() {
    if (cfg::has_display) {
        display = new SSD1306(0x3c, I2C_PIN_SDA, I2C_PIN_SCL);
        display->init();
    }
}

/*****************************************************************
 * Init LCD display                                              *
 *****************************************************************/
void init_lcd() {
    byte addr = getLCDaddr();
    if (addr == 0) return;
    byte cols = getLCDCols();
    byte lines = getLCDRows();
	if (cfg::has_lcd1602 || cfg::has_lcd2004) {
		char_lcd = new LiquidCrystal_I2C(addr, cols, lines);
}
	//LCD is set? Configure it!
    if (char_lcd) {
        char_lcd->init();
        char_lcd->backlight();
        initCustomChars();
    }
}

static void powerOnTestSensors() {
     debug_out(F("PowerOnTest"),0,1);
    cfg::debug = DEBUG_MED_INFO;


    if (cfg::pms_read) {
		debug_out(F("Read PMS(1,3,5,6,7)003..."), DEBUG_MIN_INFO, 1);
		PMS_cmd(PmSensorCmd::Start);
		delay(100);
		PMS_cmd(PmSensorCmd::ContinuousMode);
		delay(100);
		debug_out(F("Stopping PMS..."), DEBUG_MIN_INFO, 1);
		is_PMS_running = PMS_cmd(PmSensorCmd::Stop);
	}

	if (cfg::dht_read) {
		dht.begin();                                        // Start DHT
		debug_out(F("Read DHT..."), DEBUG_MIN_INFO, 1);
	}



	if (cfg::ds18b20_read) {
		ds18b20.begin();                                    // Start DS18B20
		debug_out(F("Read DS18B20..."), DEBUG_MIN_INFO, 1);
	}

    cfg::debug = DEBUG;
}

static void logEnabledAPIs() {
	debug_out(F("Send to :"), DEBUG_MIN_INFO, 1);
	if (cfg::send2dusti) {
		debug_out(F("luftdaten.info"), DEBUG_MIN_INFO, 1);
	}

	if (cfg::send2madavi) {
		debug_out(F("Madavi.de"), DEBUG_MIN_INFO, 1);
	}

	if (cfg::send2lora) {
		debug_out(F("LoRa gateway"), DEBUG_MIN_INFO, 1);
	}

	if (cfg::send2csv) {
		debug_out(F("Serial as CSV"), DEBUG_MIN_INFO, 1);
	}

	if (cfg::send2custom) {
		debug_out(F("custom API"), DEBUG_MIN_INFO, 1);
	}

	if (cfg::send2influx) {
		debug_out(F("custom influx DB"), DEBUG_MIN_INFO, 1);
	}
	debug_out("", DEBUG_MIN_INFO, 1);
	if (cfg::auto_update) {
		debug_out(F("Auto-Update active..."), DEBUG_MIN_INFO, 1);
		debug_out("", DEBUG_MIN_INFO, 1);
	}
}

static void logEnabledDisplays() {
	if (cfg::has_display) {
		debug_out(F("Show on OLED..."), DEBUG_MIN_INFO, 1);
	}
	if (cfg::has_lcd1602) {
		debug_out(F("Show on LCD 1602 ..."), DEBUG_MIN_INFO, 1);
	}
	if (cfg::has_lcd2004) {
		debug_out(F("Show on LCD 2004 ..."), DEBUG_MIN_INFO, 1);
	}
	if (cfg::has_ledbar_32) {
		debug_out(F("Show on LED Bar..."), DEBUG_MIN_INFO, 1);
	}

}


void initNonTrivials(const char *id) {
    strcpy(cfg::current_lang, CURRENT_LANG);
    if (cfg::fs_ssid == nullptr || cfg::fs_ssid[0] == '\0') {
        String ssid = F("NAM-");
        ssid.concat(id);
        stringToChar(&cfg::fs_ssid, ssid);

    }
}

#include "arch_dependend/factory_reset.h"
//check if factory reset conditions are met

/*****************************************************************
 * The Setup                                                     *
 *****************************************************************/
void setup() {
    Debug.begin(115200);
    debug_out(F("\nNAMF ver: "), DEBUG_ERROR, false);
    debug_out(SOFTWARE_VERSION, DEBUG_ERROR, false);
    debug_out(F("/"), DEBUG_ERROR, false);
    debug_out(FPSTR(INTL_LANG), DEBUG_ERROR);
    debug_out(F("Chip ID: "), DEBUG_ERROR, false);
    debug_out(esp_chipid(), DEBUG_ERROR);

    checkFactoryReset();
#ifdef ARDUINO_ARCH_ESP8266
    serialSDS.begin(9600, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX, false, SDS_SERIAL_BUFF_SIZE);
    serialGPS.begin(9600, SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 64);
#else
    serialSDS.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX, false, SDS_SERIAL_BUFF_SIZE);
    serialGPS.begin(9600, EspSoftwareSerial::SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 64);
#endif
//    schedule_recurrent_function_us([]() {
//        serialSDS.perform_work();
//        serialGPS.perform_work();
//        return true;
//    }, 50);
    serialSDS.enableIntTx(false);

    Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
    Wire.setClock(100000); // Force bus speed 100 Khz


    initNonTrivials(esp_chipid().c_str());



    FSInfo fs_info;
#ifdef ARDUINO_ARCH_ESP8266
    SPIFFS.info(fs_info);
#else
    fs_info.totalBytes = SPIFFS.totalBytes();
    fs_info.usedBytes = SPIFFS.usedBytes();
    fs_info.blockSize = 0;
    fs_info.pageSize = 0;
    fs_info.maxOpenFiles = 0;
    fs_info.maxPathLength = 0;
#endif
    debug_out(F("SPIFFS (kB): "), DEBUG_ERROR, false);
    debug_out(String(fs_info.totalBytes/(1024)), DEBUG_ERROR);

    debug_out(F("Free sketch space (kB): "), DEBUG_ERROR, false);
    debug_out(String(ESP.getFreeSketchSpace()/1024), DEBUG_ERROR);
    debug_out(F("CPU freq (MHz): "), DEBUG_ERROR, false);
    debug_out(String(ESP.getCpuFreqMHz()), DEBUG_ERROR);

    readConfig();
    debug_out(getConfigString(), DEBUG_MED_INFO);
    resetMemoryStats();
    Reporting::setupHomePhone();

    init_display();
    init_lcd();

    powerOnTestSensors();

    //set update check interval

    switch (cfg::update_channel) {
        case UPDATE_CHANNEL_ALFA:
            PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS = 3600 * 1000;
            break;
        case UPDATE_CHANNEL_BETA:
            PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS = 3 * 3600 * 1000;
            break;
        default:
            PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS = ONE_DAY_IN_MS;
    }


    configNetwork();
    if (cfg::internet) {//we are connected to internet
        Reporting::reportBoot();
        Serial.println(F(" After Report boot"));
    }
    //in AP mode (no internet) we still want webserver
    setup_webserver();

    if (cfg::gps_read) {
        serialGPS.begin(9600);
        debug_out(F("Read GPS..."), DEBUG_MIN_INFO, 1);
        disable_unneeded_nmea();
    }

    logEnabledAPIs();
    logEnabledDisplays();
#if defined(NAM_LORAWAN)
    LoRaWan::setup();
#endif

    //String server_name = F("NAM-");
    //server_name += esp_chipid();
    //if (MDNS.begin(server_name.c_str())) {
		
	if (MDNS.begin(cfg::fs_ssid)) {
        char buff[20];
#ifdef ARDUINO_ARCH_ESP8266
        MDNSResponder::hMDNSService hService = MDNS.addService( cfg::fs_ssid, "http", "tcp", 80);
        sprintf(buff, "NAM-%u", ESP.getChipId());
        MDNS.addServiceTxt(hService, "id", buff);
        MDNS.addServiceTxt(hService, "manufacturer", "Nettigo");
#else
        MDNS.addService("http", "tcp", 80);
        sprintf(buff, "NAM-%u", ESP.getEfuseMac());
        MDNS.addServiceTxt((char *)"http", (char *)"tcp", (char *)"id", (char *)buff);
        MDNS.addServiceTxt((char *)"http", (char *)"tcp", (char *)"manufacturer", (char *)"Nettigo");
#endif
    } else {
        debug_out(F("\nmDNS failure!"), DEBUG_ERROR, 1);
    }

    delay(50);

	serialSDS.flush();  //drop any packets sent on startup...

	starttime = millis();                                   // store the start time
	time_point_device_start_ms = starttime;
	starttime_SDS = starttime;
//	next_display_millis = starttime + DISPLAY_UPDATE_INTERVAL_MS;

}

static void checkForceRestart() {
	if (msSince(time_point_device_start_ms) > DURATION_BEFORE_FORCED_RESTART_MS) {
		ESP.restart();
	}
}

static unsigned long sendDataToOptionalApis(const String &data) {
	unsigned long start_send = 0;
	unsigned long sum_send_time = 0;
#if defined(NAM_LORAWAN)
    if (cfg::lw_en) {
        debug_out("\n\nLORAWAN leci!",DEBUG_ERROR);
        LoRaWan::send_lora_frame(data);
    }
#endif
    if (cfg::internet) {    //send data to API only if we have access to network
        if (cfg::send2madavi) {
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("madavi.de: "), DEBUG_MIN_INFO, 1);
            start_send = millis();
            sendData(LoggerMadavi, data, 0, HOST_MADAVI, (cfg::ssl_madavi ? 443 : 80), URL_MADAVI, true);
            sum_send_time += millis() - start_send;
            server.handleClient();
        }

        if (cfg::send2sensemap && (cfg::senseboxid[0] != '\0')) {
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("opensensemap: "), DEBUG_MIN_INFO, 1);
            start_send = millis();
            String sensemap_path = URL_SENSEMAP;
            sensemap_path.replace("BOXID", cfg::senseboxid);
            sendData(LoggerSensemap, data, 0, HOST_SENSEMAP, PORT_SENSEMAP, sensemap_path.c_str(), false);
            sum_send_time += millis() - start_send;
            server.handleClient();
        }
        if (cfg::send2fsapp) {
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("Server FS App: "), DEBUG_MIN_INFO, 1);
            start_send = millis();
            sendData(LoggerFSapp, data, 0, HOST_FSAPP, PORT_FSAPP, URL_FSAPP, false);
            sum_send_time += millis() - start_send;
            server.handleClient();

        }

        if (cfg::send2influx) {
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("custom influx db: "), DEBUG_MIN_INFO, 1);
            start_send = millis();
            const String data_4_influxdb = create_influxdb_string(data);

            sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::port_influx, cfg::url_influx, false);
            sum_send_time += millis() - start_send;
            server.handleClient();

        }

        if (cfg::send2custom) {
            String data_4_custom = data;
            data_4_custom.remove(0, 1);
#if defined(ARDUINO_ARCH_ESP8266)
            data_4_custom = "{\"esp8266id\": \"" + esp_chipid() + "\", " + data_4_custom;
#else
            data_4_custom = "{\"esp32id\": \"" + esp_chipid() + "\", " + data_4_custom;
#endif
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("custom api: "), DEBUG_MIN_INFO, 1);
            start_send = millis();
            sendData(LoggerCustom, data_4_custom, 0, cfg::host_custom, cfg::port_custom, cfg::url_custom, false);
            sum_send_time += millis() - start_send;
            server.handleClient();

        }

        if (cfg::send2aqi) {
            String data_4_custom = data;
            data_4_custom.remove(0, 1);
#if defined(ARDUINO_ARCH_ESP8266)
            data_4_custom = "{\"esp8266id\": \"" + esp_chipid() + "\", " + data_4_custom;
#else
            data_4_custom = "{\"esp32id\": \"" + esp_chipid() + "\", " + data_4_custom;
#endif
            debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("AQI.eco api: "), DEBUG_MIN_INFO, 1);
            String path = F("/update/");
            path.concat(cfg::token_AQI);
            start_send = millis();
            sendData(LoggerAQI, data_4_custom, 0, F("api.aqi.eco"), 443, path, false);
            sum_send_time += millis() - start_send;
            server.handleClient();

        }
    }


	if (cfg::send2csv) {
		debug_out(F("## Sending as csv: "), DEBUG_MIN_INFO, 1);
		send_csv(data);
        server.handleClient();

    }


    return sum_send_time;
}

/*****************************************************************
 * And action                                                    *
 *****************************************************************/
void loop() {
	String result_SDS = "";
	String result_PMS = "";
	String result_DHT = "";
	String result_BME280 = "";
	String result_HECA = "";
	String result_MHZ14 = "";
	String result_DS18B20 = "";
	String result_GPS = "";

	unsigned long sum_send_time = 0;
	unsigned long start_send;

	act_micro = micros();
	act_milli = millis();
    send_now = msSince(starttime) > cfg::sending_intervall_ms;
    sample_count++;
    if (cfg::in_factory_reset_window && millis() > 5000) {
        cfg::in_factory_reset_window = false;
        clearFactoryResetMarkers();
    }
    NAMWiFi::process();
#ifdef ARDUINO_ARCH_ESP8266
    MDNS.update();
#endif
    if (enable_ota_time > act_milli) {
        ArduinoOTA.handle();
    }

#ifdef ARDUINO_ARCH_ESP8266
    wdt_reset(); // nodemcu is alive
#endif

//    if (wificonfig_loop && millis() - wificonfig_loop_update  > 15000) {
//        debug_out(F("Updating WiFi SSID list...."),DEBUG_ERROR);
//
//        delete []wifiInfo;
//        wifiInfo = NAMWiFi::collectWiFiInfo(count_wifiInfo);
//        wificonfig_loop_update = millis();
//    }
	scheduler.process();

	if (last_micro != 0) {
		unsigned long diff_micro = act_micro - last_micro;
		if (max_micro < diff_micro) {
			max_micro = diff_micro;
		}
		if (min_micro > diff_micro) {
			min_micro = diff_micro;
		}
	}
	last_micro = act_micro;

    server.handleClient();
    yield();
#if !defined(BOOT_FW)
	if ((msSince(starttime_SDS) > SAMPLETIME_SDS_MS) || send_now) {

		if (cfg::pms_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + "PMS", DEBUG_MAX_INFO, 1);
			result_PMS = sensorPMS();
			starttime_SDS = act_milli;
		}

	}

	server.handleClient();
    yield();
	if (send_now) {
        debug_out(String(F("****************** Upload data to APIs*****************************")),DEBUG_MED_INFO);
        if (cfg::dht_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_DHT22), DEBUG_MAX_INFO, 1);
			result_DHT = sensorDHT();                       // getting temperature and humidity (optional)
		}

		if (cfg::ds18b20_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_DS18B20), DEBUG_MAX_INFO, 1);
			result_DS18B20 = sensorDS18B20();               // getting temperature (optional)
		}
	}

	if (cfg::gps_read && ((msSince(starttime_GPS) > SAMPLETIME_GPS_MS) || send_now)) {
		debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + F("GPS"), DEBUG_MAX_INFO, 1);
		result_GPS = sensorGPS();                           // getting GPS coordinates
		starttime_GPS = act_milli;
	}

    cycleDisplay();
	yield();

	if ((cfg::has_ledbar_32) && (send_now)) {
		displayLEDBar();
	}

	if (send_now) {
        displaySendSignal();
		debug_out(F("Creating data string:"), DEBUG_MIN_INFO, 1);
		String data = FPSTR(data_first_part);
		data.replace("{v}", String(SOFTWARE_VERSION));
		String data_sample_times  = Value2Json(F("samples"), String(sample_count));
		data_sample_times.concat(Value2Json(String(F("min_micro")), String(min_micro)));
		data_sample_times.concat(Value2Json(String(F("max_micro")), String(max_micro)));

		String signal_strength = String(WiFi.RSSI());
		debug_out(F("WLAN signal strength: "), DEBUG_MIN_INFO, 0);
		debug_out(signal_strength, DEBUG_MIN_INFO, 0);
		debug_out(" dBm", DEBUG_MIN_INFO, 1);
		debug_out("----", DEBUG_MIN_INFO, 1);

		server.handleClient();
		yield();
//		server.stop();
        if (cfg::internet) {
            const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);

            if (cfg::pms_read) {
                data.concat(result_PMS);
                if (cfg::send2dusti) {
                    debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(PMS): "), DEBUG_MIN_INFO, 1);
                    start_send = millis();
                    sendLuftdaten(result_PMS, PMS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "PMS_");
                    sum_send_time += millis() - start_send;
                }
            }
            server.handleClient();
            if (cfg::dht_read) {
                data.concat(result_DHT);
                if (cfg::send2dusti) {
                    debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DHT): "), DEBUG_MIN_INFO, 1);
                    start_send = millis();
                    sendLuftdaten(result_DHT, DHT_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DHT_");
                    sum_send_time += millis() - start_send;
                }
            }
            server.handleClient();


            if (cfg::ds18b20_read) {
                data.concat(result_DS18B20);
                if (cfg::send2dusti) {
                    debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DS18B20): "), DEBUG_MIN_INFO, 1);
                    start_send = millis();
                    sendLuftdaten(result_DS18B20, DS18B20_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DS18B20_");
                    sum_send_time += millis() - start_send;
                }
            }
            server.handleClient();

            if (cfg::gps_read) {
                data.concat(result_GPS);
                if (cfg::send2dusti) {
                    debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(GPS): "), DEBUG_MIN_INFO, 1);
                    start_send = millis();
                    sendLuftdaten(result_GPS, GPS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "GPS_");
                    sum_send_time += millis() - start_send;
                }
            }
            server.handleClient();

        }


        //add results from new scheduler
        SimpleScheduler::getResults(data);

        // reconnect to WiFi if disconnected
        NAMWiFi::tryToReconnect();

        if (cfg::internet && cfg::send2dusti) {
		    SimpleScheduler::sendToSC();
		}
		data_sample_times.concat(Value2Json("signal", signal_strength));
        data_sample_times.remove(data_sample_times.length()-1);
		data.concat(data_sample_times)  ;

		data.concat(F("]}"));

		sum_send_time += sendDataToOptionalApis(data);

		//as for now we do not collect sending status, so we assume sending was successful
		SimpleScheduler::afterSendData(true);

//		server.client().setNoDelay(true);

		checkForceRestart();

		if (cfg::auto_update && msSince(last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS) {
			updateFW();
		}

		sending_time = (4 * sending_time + sum_send_time) / 5;
		debug_out(F("Time for sending data (ms): "), DEBUG_MIN_INFO, 0);
		debug_out(String(sending_time), DEBUG_MIN_INFO, 1);
        server.handleClient();



        // Resetting for next sampling
		last_data_string = data;
		sample_count = 0;
		last_micro = 0;
		min_micro = 1000000000;
		max_micro = 0;
		sum_send_time = 0;
		starttime = millis();                               // store the start time
		first_cycle = false;
		count_sends += 1;

		resetMemoryStats();
	}
	delay(1);
    collectMemStats();
    Reporting::homePhone();
#endif
//	if (sample_count % 500 == 0) { Serial.println(ESP.getFreeHeap(),DEC); }
}
