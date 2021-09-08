#include <Arduino.h>

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/
#include "defines.h"
#include "variables.h"
#include <Schedule.h>
#include "variables_init.h"
#include "update.h"
#include "helpers.h"
#include "system/scheduler.h"
#include "system/components.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <time.h>
#include <coredecls.h>
#include <assert.h>

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

void setDefaultConfig(void) {
    SDS011::setDefaults();
    HECA::setDefaults();
    BMPx80::setDefaults();
    SHT3x::setDefaults();


}

/*****************************************************************
 * read config from spiffs                                       *
 *****************************************************************/
void readConfig() {
	debug_out(F("mounting FS..."), DEBUG_MIN_INFO, 1);

	if (SPIFFS.begin()) {
		debug_out(F("mounted file system..."), DEBUG_MIN_INFO, 1);
		if (SPIFFS.exists("/config.json")) {
			//file exists, reading and loading
			debug_out(F("reading config file..."), DEBUG_MIN_INFO, 1);
			File configFile = SPIFFS.open("/config.json", "r");
            if (!readAndParseConfigFile(configFile)){
                //not failed opening & reading
                debug_out(F("Config parsed and loaded"), DEBUG_MIN_INFO, true);
            }else{
                debug_out(F("FAILED config parsing and loading"), DEBUG_ERROR, true);
                setDefaultConfig();
            };
		} else {
			debug_out(F("config file not found ..."), DEBUG_ERROR, 1);
			setDefaultConfig();
		}
	} else {
		debug_out(F("failed to mount FS"), DEBUG_ERROR, 1);
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




static int selectChannelForAp(struct struct_wifiInfo *info, int count) {
	std::array<int, 14> channels_rssi;
	std::fill(channels_rssi.begin(), channels_rssi.end(), -100);

	for (int i = 0; i < count; i++) {
		if (info[i].RSSI > channels_rssi[info[i].channel]) {
			channels_rssi[info[i].channel] = info[i].RSSI;
		}
	}

	if ((channels_rssi[1] < channels_rssi[6]) && (channels_rssi[1] < channels_rssi[11])) {
		return 1;
	} else if ((channels_rssi[6] < channels_rssi[1]) && (channels_rssi[6] < channels_rssi[11])) {
		return 6;
	} else {
		return 11;
	}
}

/*****************************************************************
 * WifiConfig                                                    *
 *****************************************************************/
void wifiConfig() {
	debug_out(F("Starting WiFiManager"), DEBUG_MIN_INFO, 1);
	debug_out(F("AP ID: "), DEBUG_MIN_INFO, 0);
	debug_out(cfg::fs_ssid, DEBUG_MIN_INFO, 1);
	debug_out(F("Password: "), DEBUG_MIN_INFO, 0);
	debug_out(cfg::fs_pwd, DEBUG_MIN_INFO, 1);

	wificonfig_loop = true;

	WiFi.disconnect(true);
	debug_out(F("scan for wifi networks..."), DEBUG_MIN_INFO, 1);
	count_wifiInfo = WiFi.scanNetworks(false, true);
	wifiInfo = new struct_wifiInfo[count_wifiInfo];
	for (int i = 0; i < count_wifiInfo; i++) {
		uint8_t* BSSID;
		String SSID;
		WiFi.getNetworkInfo(i, SSID, wifiInfo[i].encryptionType, wifiInfo[i].RSSI, BSSID, wifiInfo[i].channel, wifiInfo[i].isHidden);
		SSID.toCharArray(wifiInfo[i].ssid, 35);
	}

	WiFi.mode(WIFI_AP);
	const IPAddress apIP(192, 168, 4, 1);
	WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
	WiFi.softAP(cfg::fs_ssid, cfg::fs_pwd, selectChannelForAp(wifiInfo, count_wifiInfo));
	debug_out(String(WLANPWD), DEBUG_MIN_INFO, 1);

	DNSServer dnsServer;
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(53, "*", apIP);							// 53 is port for DNS server

	// 10 minutes timeout for wifi config
	last_page_load = millis();
	while (((millis() - last_page_load) < cfg::time_for_wifi_config)) {
		dnsServer.processNextRequest();
		server.handleClient();
		wdt_reset(); // nodemcu is alive
		yield();
	}

	// half second to answer last requests
	last_page_load = millis();
	while ((millis() - last_page_load) < 500) {
		dnsServer.processNextRequest();
		server.handleClient();
		yield();
	}

	WiFi.disconnect(true);
	WiFi.softAPdisconnect(true);
	WiFi.mode(WIFI_STA);

	delete []wifiInfo;

	dnsServer.stop();

	delay(100);

	debug_out(F("Connecting to "), DEBUG_MIN_INFO, 0);
	debug_out(cfg::wlanssid, DEBUG_MIN_INFO, 1);

	// we could set hostname also here
	WiFi.begin(cfg::wlanssid, cfg::wlanpwd);

	debug_out(F("---- Result Webconfig ----"), DEBUG_MIN_INFO, 1);
	debug_out(F("WLANSSID: "), DEBUG_MIN_INFO, 0);
	debug_out(cfg::wlanssid, DEBUG_MIN_INFO, 1);
	debug_out(F("----\nReading ..."), DEBUG_MIN_INFO, 1);
	debug_out(F("SDS: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::sds_read), DEBUG_MIN_INFO, 1);
	debug_out(F("PMS: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::pms_read), DEBUG_MIN_INFO, 1);
	debug_out(F("DHT: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::dht_read), DEBUG_MIN_INFO, 1);
	debug_out(F("DS18B20: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::ds18b20_read), DEBUG_MIN_INFO, 1);
	debug_out(F("----\nSend to ..."), DEBUG_MIN_INFO, 1);
	debug_out(F("Dusti: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::send2dusti), DEBUG_MIN_INFO, 1);
	debug_out(F("Madavi: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::send2madavi), DEBUG_MIN_INFO, 1);
	debug_out(F("CSV: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::send2csv), DEBUG_MIN_INFO, 1);
	debug_out("----", DEBUG_MIN_INFO, 1);
	debug_out(F("Autoupdate: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::auto_update), DEBUG_MIN_INFO, 1);
	debug_out(F("Display: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::has_display), DEBUG_MIN_INFO, 1);
	debug_out(F("LCD 1602: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::has_lcd1602), DEBUG_MIN_INFO, 1);
	debug_out(F("Debug: "), DEBUG_MIN_INFO, 0);
	debug_out(String(cfg::debug), DEBUG_MIN_INFO, 1);
	wificonfig_loop = false;
}

static void waitForWifiToConnect(int maxRetries) {
	int retryCount = 0;
	while ((WiFi.status() != WL_CONNECTED) && (retryCount <  maxRetries)) {
		delay(500);
		debug_out(".", DEBUG_MIN_INFO, 0);
		++retryCount;
	}
}

/*****************************************************************
 * Start WiFi in AP mode (for configuration)
 *****************************************************************/

void    startAP(void) {
		String fss = String(cfg::fs_ssid);
		display_debug(fss.substring(0, 16), fss.substring(16));
		wifiConfig();
		if (WiFi.status() != WL_CONNECTED) {
			waitForWifiToConnect(20);
			debug_out("", DEBUG_MIN_INFO, 1);
		}

}

/*****************************************************************
 * WiFi auto connecting script                                   *
 *****************************************************************/
void connectWifi() {
	debug_out(String(WiFi.status()), DEBUG_MIN_INFO, 1);
	WiFi.disconnect();
	WiFi.setOutputPower(cfg::outputPower);
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);
	WiFi.setPhyMode((WiFiPhyMode_t)cfg::phyMode);
	WiFi.mode(WIFI_STA);
	
	//String hostname = F("NAM-");
    //hostname += esp_chipid();
	//WiFi.hostname(hostname.c_str()); // Hostname for DHCP Server.

	WiFi.hostname(cfg::fs_ssid); // Hostname for DHCP Server
	WiFi.begin(cfg::wlanssid, cfg::wlanpwd); // Start WiFI

	debug_out(F("Connecting to "), DEBUG_MIN_INFO, 0);
	debug_out(cfg::wlanssid, DEBUG_MIN_INFO, 1);

	waitForWifiToConnect(40);
	debug_out("", DEBUG_MIN_INFO, 1);
	if (WiFi.status() != WL_CONNECTED) {
		startAP();
	}
	debug_out(F("WiFi connected\nIP address: "), DEBUG_MIN_INFO, 0);
	debug_out(WiFi.localIP().toString(), DEBUG_MIN_INFO, 1);
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
	if (cfg::has_lcd1602_27) {
		char_lcd = new LiquidCrystal_I2C(0x27, 16, 2);
    } else if (cfg::has_lcd1602) {
        char_lcd = new LiquidCrystal_I2C(0x3F, 16, 2);
    } else if (cfg::has_lcd2004_27) {
        char_lcd = new LiquidCrystal_I2C(0x27, 20, 4);
    } else if (cfg::has_lcd2004_3f) {
        char_lcd = new LiquidCrystal_I2C(0x3F, 20, 4);
	}

	//LCD is set? Configure it!
    if (char_lcd) {
        char_lcd->init();
        char_lcd->backlight();
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
	if (cfg::has_lcd1602 || cfg::has_lcd1602_27) {
		debug_out(F("Show on LCD 1602 ..."), DEBUG_MIN_INFO, 1);
	}
	if (cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) {
		debug_out(F("Show on LCD 2004 ..."), DEBUG_MIN_INFO, 1);
	}
	if (cfg::has_ledbar_32) {
		debug_out(F("Show on LED Bar..."), DEBUG_MIN_INFO, 1);
	}

}

void time_is_set (void) {
	sntp_time_is_set = true;
}

static bool acquireNetworkTime() {
	int retryCount = 0;
	debug_out(F("Setting time using SNTP"), DEBUG_MIN_INFO, 1);
	time_t now = time(nullptr);
	debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
	debug_out(NTP_SERVER,DEBUG_MIN_INFO,1);
	settimeofday_cb(time_is_set);
	configTime("GMT", NTP_SERVER);
	while (retryCount++ < 20) {
		// later than 2000/01/01:00:00:00
		if (sntp_time_is_set) {
			now = time(nullptr);
			debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
			return true;
		}
		delay(500);
		debug_out(".",DEBUG_MIN_INFO,0);
	}
	debug_out(F("\nrouter/gateway:"),DEBUG_MIN_INFO,1);
	retryCount = 0;
	configTime(0, 0, WiFi.gatewayIP().toString().c_str());
	while (retryCount++ < 20) {
		// later than 2000/01/01:00:00:00
		if (sntp_time_is_set) {
			now = time(nullptr);
			debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
			return true;
		}
		delay(500);
		debug_out(".",DEBUG_MIN_INFO,0);
	}
	return false;
}

/*****************************************************************
 * The Setup                                                     *
 *****************************************************************/
void setup() {
    Serial.begin(115200);                    // Output to Serial at 9600 baud
    serialSDS.begin(9600, SWSERIAL_8N1, PM_SERIAL_RX, PM_SERIAL_TX, false, SDS_SERIAL_BUFF_SIZE);
    serialGPS.begin(9600, SWSERIAL_8N1, GPS_SERIAL_RX, GPS_SERIAL_TX, false, 64);

    schedule_recurrent_function_us([]() {
        serialSDS.perform_work();
        serialGPS.perform_work();
        return true;
    }, 500);
    serialSDS.enableIntTx(false);

    Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
    Wire.setClock(100000); // Force bus speed 100 Khz


    cfg::initNonTrivials(esp_chipid().c_str());

    Serial.print(F("\nNAMF ver: "));
    Serial.print(SOFTWARE_VERSION);
    Serial.print(F("/"));
    Serial.println(INTL_LANG);
    Serial.print(F("Chip ID: "));
    Serial.println(esp_chipid());


    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.print(F("SPIFFS size (kB): "));
    Serial.println(fs_info.totalBytes/(1024));

    Serial.print(F("Free sketch space (kB): "));
    Serial.println(ESP.getFreeSketchSpace()/1024);
    Serial.print(F("CPU freq (MHz): "));
    Serial.println(ESP.getCpuFreqMHz());

    readConfig();
    resetMemoryStats();

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


    setup_webserver();
    display_debug(F("Connecting to"), String(cfg::wlanssid));
    debug_out(F("SSID: '"), DEBUG_ERROR, 0);
    debug_out(cfg::wlanssid, DEBUG_ERROR, 0);
    debug_out(F("'"), DEBUG_ERROR, 1);

    if (strlen(cfg::wlanssid) > 0) {
        connectWifi();
        got_ntp = acquireNetworkTime();
        debug_out(F("NTP time "), DEBUG_MIN_INFO, 0);
        debug_out(String(got_ntp ? "" : "not ") + F("received"), DEBUG_MIN_INFO, 1);
        if(cfg::auto_update) {
            updateFW();
        }
    } else {
        startAP();
    }

    if (cfg::gps_read) {
        serialGPS.begin(9600);
        debug_out(F("Read GPS..."), DEBUG_MIN_INFO, 1);
        disable_unneeded_nmea();
    }

    logEnabledAPIs();
    logEnabledDisplays();

    //String server_name = F("NAM-");
    //server_name += esp_chipid();
    //if (MDNS.begin(server_name.c_str())) {
		
	if (MDNS.begin(cfg::fs_ssid)) {
        MDNSResponder::hMDNSService hService = MDNS.addService( cfg::fs_ssid, "http", "tcp", 80);
//        MDNS.addService("http", "tcp", 80);
        char buff[20];
        sprintf(buff, "NAM-%u", ESP.getChipId());
        MDNS.addServiceTxt(hService, "id", buff);
        MDNS.addServiceTxt(hService, "manufacturer", "Nettigo");
    } else {
        debug_out(F("\nmDNS failure!"), DEBUG_ERROR, 1);
    }

    delay(50);

	// sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
	wdt_disable();
	wdt_enable(30000);

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

	if (cfg::send2madavi) {
		debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("madavi.de: "), DEBUG_MIN_INFO, 1);
		start_send = millis();
		sendData(LoggerMadavi, data, 0, HOST_MADAVI, (cfg::ssl_madavi ? 443 : 80), URL_MADAVI, true);
		sum_send_time += millis() - start_send;
	}

    if (cfg::send2sensemap && (cfg::senseboxid[0] != '\0')) {
		debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("opensensemap: "), DEBUG_MIN_INFO, 1);
		start_send = millis();
		String sensemap_path = URL_SENSEMAP;
		sensemap_path.replace("BOXID", cfg::senseboxid);
		sendData(LoggerSensemap, data, 0, HOST_SENSEMAP, PORT_SENSEMAP, sensemap_path.c_str(), false);
		sum_send_time += millis() - start_send;
	}

	if (cfg::send2fsapp) {
		debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("Server FS App: "), DEBUG_MIN_INFO, 1);
		start_send = millis();
		sendData(LoggerFSapp, data, 0, HOST_FSAPP, PORT_FSAPP, URL_FSAPP, false);
		sum_send_time += millis() - start_send;
	}

    if (cfg::send2influx) {
        debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("custom influx db: "), DEBUG_MIN_INFO, 1);
        start_send = millis();
        const String data_4_influxdb = create_influxdb_string(data);

        sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::port_influx, cfg::url_influx, false);
		sum_send_time += millis() - start_send;
    }

	/*		if (send2lora) {
				debug_out(F("## Sending to LoRa gateway: "), DEBUG_MIN_INFO, 1);
				send_lora(data);
			}
	*/
	if (cfg::send2csv) {
		debug_out(F("## Sending as csv: "), DEBUG_MIN_INFO, 1);
		send_csv(data);
	}

	if (cfg::send2custom) {
		String data_4_custom = data;
		data_4_custom.remove(0, 1);
		data_4_custom = "{\"esp8266id\": \"" + esp_chipid() + "\", " + data_4_custom;
		debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("custom api: "), DEBUG_MIN_INFO, 1);
		start_send = millis();
		sendData(LoggerCustom, data_4_custom, 0, cfg::host_custom, cfg::port_custom, cfg::url_custom, false);
		sum_send_time += millis() - start_send;
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

    MDNS.update();
    if (enable_ota_time > act_milli) {
        ArduinoOTA.handle();
    }
	wdt_reset(); // nodemcu is alive

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
		server.stop();
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
		if (cfg::dht_read) {
			data.concat(result_DHT);
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DHT): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_DHT, DHT_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DHT_");
				sum_send_time += millis() - start_send;
			}
		}


		if (cfg::ds18b20_read) {
			data.concat(result_DS18B20);
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DS18B20): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_DS18B20, DS18B20_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DS18B20_");
				sum_send_time += millis() - start_send;
			}
		}

		if (cfg::gps_read) {
			data.concat(result_GPS);
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(GPS): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_GPS, GPS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "GPS_");
				sum_send_time += millis() - start_send;
			}
		}


        //add results from new scheduler
        SimpleScheduler::getResults(data);

		if (cfg::send2dusti) {
		    SimpleScheduler::sendToSC();
		}
		data_sample_times.concat(Value2Json("signal", signal_strength));
        data_sample_times.remove(data_sample_times.length()-1);
		data.concat(data_sample_times)  ;

		data.concat(F("]}"));

		sum_send_time += sendDataToOptionalApis(data);

		//as for now we do not collect sending status, so we assume sending was successful
		SimpleScheduler::afterSendData(true);

		server.begin();
		server.client().setNoDelay(true);

		checkForceRestart();

		if (cfg::auto_update && msSince(last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS) {
			updateFW();
		}

		sending_time = (4 * sending_time + sum_send_time) / 5;
		debug_out(F("Time for sending data (ms): "), DEBUG_MIN_INFO, 0);
		debug_out(String(sending_time), DEBUG_MIN_INFO, 1);


		// reconnect to WiFi if disconnected
		if (WiFi.status() != WL_CONNECTED) {
			debug_out(F("Connection lost, reconnecting "), DEBUG_MIN_INFO, 0);
			WiFi.reconnect();
			waitForWifiToConnect(20);
			debug_out("", DEBUG_MIN_INFO, 1);
		}

		// Resetting for next sampling
		last_data_string = data;
		lowpulseoccupancyP1 = 0;
		lowpulseoccupancyP2 = 0;
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
#endif
//	if (sample_count % 500 == 0) { Serial.println(ESP.getFreeHeap(),DEC); }
}
