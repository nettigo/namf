#include <Arduino.h>

/*****************************************************************
 * Includes                                                      *
 *****************************************************************/
#include "defines.h"
#include "variables.h"
#include "variables_init.h"
#include "update.h"
#include "helpers.h"
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LiquidCrystal_I2C.h>
#include <base64.h>
#include <ArduinoJson.h>
#include <time.h>
#include <coredecls.h>
#include <assert.h>

#include "html-content.h"
#include "webserver.h"
#include "sensors/sds011.h"
#include "sensors/bme280.h"
#include "sensors/dht.h"
#include "sensors/heca.h"
#include "sensors/winsen-mhz.h"
#include "display/ledbar.h"

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
            };
		} else {
			debug_out(F("config file not found ..."), DEBUG_ERROR, 1);
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

String table_row_from_value(const String& sensor, const String& param, const String& value, const String& unit) {
	String s = F(	"<tr>"
					"<td>{s}</td>"
					"<td>{p}</td>"
					"<td class='r'>{v}&nbsp;{u}</td>"
					"</tr>");
	s.replace("{s}", sensor);
	s.replace("{p}", param);
	s.replace("{v}", value);
	s.replace("{u}", unit);
	return s;
}

static int32_t calcWiFiSignalQuality(int32_t rssi) {
	if (rssi > -50) {
		rssi = -50;
	}
	if (rssi < -100) {
		rssi = -100;
	}
	return (rssi + 100) * 2;
}

String wlan_ssid_to_table_row(const String& ssid, const String& encryption, int32_t rssi) {
	String s = F(	"<tr>"
					"<td>"
					"<a href='#wlanpwd' onclick='setSSID(this)' class='wifi'>{n}</a>&nbsp;{e}"
					"</td>"
					"<td style='width:80%;vertical-align:middle;'>"
					"{v}%"
					"</td>"
					"</tr>");
	s.replace("{n}", ssid);
	s.replace("{e}", encryption);
	s.replace("{v}", String(calcWiFiSignalQuality(rssi)));
	return s;
}

String warning_first_cycle() {
	String s = FPSTR(INTL_TIME_TO_FIRST_MEASUREMENT);
	unsigned long time_to_first = cfg::sending_intervall_ms - msSince(starttime);
	if (time_to_first > cfg::sending_intervall_ms) {
		time_to_first = 0;
	}
	s.replace("{v}", String((long)((time_to_first + 500) / 1000)));
	return s;
}

String age_last_values() {
	String s = "<b>";
	unsigned long time_since_last = msSince(starttime);
	if (time_since_last > cfg::sending_intervall_ms) {
		time_since_last = 0;
	}
	s += String((long)((time_since_last + 500) / 1000));
	s += FPSTR(INTL_TIME_SINCE_LAST_MEASUREMENT);
	s += F("</b><br/><br/>");
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
 * Webserver wifi: show available wifi networks                  *
 *****************************************************************/
void webserver_wifi() {
	debug_out(F("wifi networks found: "), DEBUG_MIN_INFO, 0);
	debug_out(String(count_wifiInfo), DEBUG_MIN_INFO, 1);
	String page_content = "";
	if (count_wifiInfo == 0) {
		page_content += BR_TAG;
		page_content += FPSTR(INTL_NO_NETWORKS);
		page_content += BR_TAG;
	} else {
		std::unique_ptr<int[]> indices(new int[count_wifiInfo]);
		debug_out(F("output config page 2"), DEBUG_MIN_INFO, 1);
		for (int i = 0; i < count_wifiInfo; i++) {
			indices[i] = i;
		}
		for (int i = 0; i < count_wifiInfo; i++) {
			for (int j = i + 1; j < count_wifiInfo; j++) {
				if (wifiInfo[indices[j]].RSSI > wifiInfo[indices[i]].RSSI) {
					std::swap(indices[i], indices[j]);
				}
			}
		}
		debug_out(F("output config page 3"), DEBUG_MIN_INFO, 1);
		int duplicateSsids = 0;
		for (int i = 0; i < count_wifiInfo; i++) {
			if (indices[i] == -1) {
				continue;
			}
			for (int j = i + 1; j < count_wifiInfo; j++) {
				if (strncmp(wifiInfo[indices[i]].ssid, wifiInfo[indices[j]].ssid, 35) == 0) {
					indices[j] = -1; // set dup aps to index -1
					++duplicateSsids;
				}
			}
		}

		page_content += FPSTR(INTL_NETWORKS_FOUND);
		page_content += String(count_wifiInfo - duplicateSsids);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(BR_TAG);
		page_content += FPSTR(TABLE_TAG_OPEN);
		//if(n > 30) n=30;
		for (int i = 0; i < count_wifiInfo; ++i) {
			if (indices[i] == -1 || wifiInfo[indices[i]].isHidden) {
				continue;
			}
			// Print SSID and RSSI for each network found
			page_content += wlan_ssid_to_table_row(wifiInfo[indices[i]].ssid, ((wifiInfo[indices[i]].encryptionType == ENC_TYPE_NONE) ? " " : u8"🔒"), wifiInfo[indices[i]].RSSI);
		}
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
		page_content += FPSTR(BR_TAG);
	}
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
void webserver_values() {
	if (WiFi.status() != WL_CONNECTED) {
		sendHttpRedirect(server);
	} else {
		String page_content = make_header(FPSTR(INTL_CURRENT_DATA));
		const String unit_PM = "µg/m³";
		const String unit_T = "°C";
		const String unit_H = "%";
		const String unit_P = "hPa";
		last_page_load = millis();

		const int signal_quality = calcWiFiSignalQuality(WiFi.RSSI());
		debug_out(F("output values to web page..."), DEBUG_MIN_INFO, 1);
		if (first_cycle) {
			page_content += F("<b style='color:red'>");
			page_content += warning_first_cycle();
			page_content += F("</b><br/><br/>");
		} else {
			page_content += age_last_values();
		}
		page_content += F("<table cellspacing='0' border='1' cellpadding='5'>");
		page_content += tmpl(F("<tr><th>{v1}</th><th>{v2}</th><th>{v3}</th>"), FPSTR(INTL_SENSOR), FPSTR(INTL_PARAMETER), FPSTR(INTL_VALUE));
		if (cfg::sds_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM2.5", check_display_value(last_value_SDS_P2, -1, 1, 0), unit_PM);
			page_content += table_row_from_value(FPSTR(SENSORS_SDS011), "PM10", check_display_value(last_value_SDS_P1, -1, 1, 0), unit_PM);
		}
        if (cfg::winsen_mhz14a_read) {
            page_content += FPSTR(EMPTY_ROW);
            page_content += table_row_from_value(FPSTR(INTL_MHZ14A_VAL), F("CO2"), String(last_value_WINSEN_CO2),
                                                 F("ppm"));
        }
        if (cfg::pms_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM1", check_display_value(last_value_PMS_P0, -1, 1, 0), unit_PM);
			page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM2.5", check_display_value(last_value_PMS_P2, -1, 1, 0), unit_PM);
			page_content += table_row_from_value(FPSTR(SENSORS_PMSx003), "PM10", check_display_value(last_value_PMS_P1, -1, 1, 0), unit_PM);
		}
		if (cfg::dht_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_DHT_T, -128, 1, 0), unit_T);
			page_content += table_row_from_value(FPSTR(SENSORS_DHT22), FPSTR(INTL_HUMIDITY), check_display_value(last_value_DHT_H, -1, 1, 0), unit_H);
		}
		if (cfg::bmp280_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_BMP280), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_BMP280_T, -128, 1, 0), unit_T);
			page_content += table_row_from_value(FPSTR(SENSORS_BMP280), FPSTR(INTL_PRESSURE), check_display_value(last_value_BMP280_P / 100.0, (-1 / 100.0), 2, 0), unit_P);
		}
		if (cfg::bme280_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_BME280_T, -128, 1, 0), unit_T);
			page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_HUMIDITY), check_display_value(last_value_BME280_H, -1, 1, 0), unit_H);
			page_content += table_row_from_value(FPSTR(SENSORS_BME280), FPSTR(INTL_PRESSURE),  check_display_value(last_value_BME280_P / 100.0, (-1 / 100.0), 2, 0), unit_P);
		}
		if (cfg::heca_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_HECA_T, -128, 1, 0), unit_T);
			page_content += table_row_from_value(FPSTR(SENSORS_HECA), FPSTR(INTL_HUMIDITY), check_display_value(last_value_HECA_H, -1, 1, 0), unit_H);
		}
		if (cfg::ds18b20_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(FPSTR(SENSORS_DS18B20), FPSTR(INTL_TEMPERATURE), check_display_value(last_value_DS18B20_T, -128, 1, 0), unit_T);
		}
		if (cfg::gps_read) {
			page_content += FPSTR(EMPTY_ROW);
			page_content += table_row_from_value(F("GPS"), FPSTR(INTL_LATITUDE), check_display_value(last_value_GPS_lat, -200.0, 6, 0), "°");
			page_content += table_row_from_value(F("GPS"), FPSTR(INTL_LONGITUDE), check_display_value(last_value_GPS_lon, -200.0, 6, 0), "°");
			page_content += table_row_from_value(F("GPS"), FPSTR(INTL_ALTITUDE),  check_display_value(last_value_GPS_alt, -1000.0, 2, 0), "m");
			page_content += table_row_from_value(F("GPS"), FPSTR(INTL_DATE), last_value_GPS_date, "");
			page_content += table_row_from_value(F("GPS"), FPSTR(INTL_TIME), last_value_GPS_time, "");
		}

		page_content += FPSTR(EMPTY_ROW);
		page_content += table_row_from_value(F("WiFi"), FPSTR(INTL_SIGNAL_STRENGTH),  String(WiFi.RSSI()), "dBm");
		page_content += table_row_from_value(F("WiFi"), FPSTR(INTL_SIGNAL_QUALITY), String(signal_quality), "%");
		page_content += FPSTR(EMPTY_ROW);
		page_content += table_row_from_value(F("NAM"),FPSTR(INTL_NUMBER_OF_MEASUREMENTS),String(count_sends),"");
		page_content += table_row_from_value(F("NAM"),F("Uptime"), millisToTime(millis()),"");
		page_content += FPSTR(EMPTY_ROW);
		page_content += table_row_from_value(F("ESP"),F("Reset Reason"), String(ESP.getResetReason()),"");
		String tmp = String(memoryStatsMin.maxFreeBlock) + String("/") + String(memoryStatsMax.maxFreeBlock);
		page_content += table_row_from_value(F("ESP"),F("Max Free Block Size"), tmp,"B");
        tmp = String(memoryStatsMin.frag) + String("/") + String(memoryStatsMax.frag);
        page_content += table_row_from_value(F("ESP"),F("Heap Fragmentation"), tmp,"%");
        tmp = String(memoryStatsMin.freeContStack) + String("/") + String(memoryStatsMax.freeContStack);
        page_content += table_row_from_value(F("ESP"),F("Free Cont Stack"), tmp,"B");
        tmp = String(memoryStatsMin.freeHeap) + String("/") + String(memoryStatsMax.freeHeap);
        page_content += table_row_from_value(F("ESP"),F("Free Memory"), tmp,"B");
        page_content += table_row_from_value(F("ESP"),F("Flash ID"), String(ESP.getFlashChipId()),"");
        page_content += table_row_from_value(F("ESP"),F("Flash Vendor ID"), String(ESP.getFlashChipVendorId()),"");
        page_content += table_row_from_value(F("ESP"),F("Flash Speed"), String(ESP.getFlashChipSpeed()/1000000.0),"MHz");
        page_content += table_row_from_value(F("ESP"),F("Flash Mode"), String(ESP.getFlashChipMode()),"");
		page_content += FPSTR(EMPTY_ROW);
		page_content += table_row_from_value(F("ENV"),F("Core version"), String(ESP.getCoreVersion()),"");
		page_content += table_row_from_value(F("ENV"),F("SDK version"), String(ESP.getSdkVersion()),"");
		page_content += FPSTR(TABLE_TAG_CLOSE_BR);
		page_content += make_footer();
		server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
	}
}

/*****************************************************************
 * Webserver set debug level                                     *
 *****************************************************************/
void webserver_debug_level() {
	if (!webserver_request_auth())
	{ return; }

	String page_content = make_header(FPSTR(INTL_DEBUG_LEVEL));
	last_page_load = millis();
	debug_out(F("output change debug level page..."), DEBUG_MIN_INFO, 1);

	if (server.hasArg("lvl")) {
		const int lvl = server.arg("lvl").toInt();
		if (lvl >= 0 && lvl <= 5) {
			cfg::debug = lvl;
			page_content += F("<h3>");
			page_content += FPSTR(INTL_DEBUG_SETTING_TO);
			page_content += F(" ");

			static constexpr std::array<const char *, 6> lvlText PROGMEM = {
				INTL_NONE, INTL_ERROR, INTL_WARNING, INTL_MIN_INFO, INTL_MED_INFO, INTL_MAX_INFO
			};

			page_content += FPSTR(lvlText[lvl]);
			page_content += F(".</h3>");
		}
	}
	page_content += make_footer();
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
void webserver_removeConfig() {
	if (!webserver_request_auth())
	{ return; }

	String page_content = make_header(FPSTR(INTL_DELETE_CONFIG));
	String message_string = F("<h3>{v}.</h3>");
	last_page_load = millis();
	debug_out(F("output remove config page..."), DEBUG_MIN_INFO, 1);

	if (server.method() == HTTP_GET) {
		page_content += FPSTR(WEB_REMOVE_CONFIG_CONTENT);
		page_content.replace("{t}", FPSTR(INTL_CONFIGURATION_REALLY_DELETE));
		page_content.replace("{b}", FPSTR(INTL_DELETE));
		page_content.replace("{c}", FPSTR(INTL_CANCEL));

	} else {
		if (SPIFFS.exists("/config.json")) {	//file exists
			debug_out(F("removing config.json..."), DEBUG_MIN_INFO, 1);
			if (SPIFFS.remove("/config.json")) {
				page_content += tmpl(message_string, FPSTR(INTL_CONFIG_DELETED));
			} else {
				page_content += tmpl(message_string, FPSTR(INTL_CONFIG_CAN_NOT_BE_DELETED));
			}
		} else {
			page_content += tmpl(message_string, FPSTR(INTL_CONFIG_NOT_FOUND));
		}
	}
	page_content += make_footer();
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver reset NodeMCU                                       *
 *****************************************************************/
void webserver_reset() {
	if (!webserver_request_auth())
	{ return; }

	String page_content = make_header(FPSTR(INTL_RESTART_SENSOR));
	last_page_load = millis();
	debug_out(F("output reset NodeMCU page..."), DEBUG_MIN_INFO, 1);

	if (server.method() == HTTP_GET) {
		page_content += FPSTR(WEB_RESET_CONTENT);
		page_content.replace("{t}", FPSTR(INTL_REALLY_RESTART_SENSOR));
		page_content.replace("{b}", FPSTR(INTL_RESTART));
		page_content.replace("{c}", FPSTR(INTL_CANCEL));
	} else {
		ESP.restart();
	}
	page_content += make_footer();
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_HTML), page_content);
}

/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
void webserver_data_json() {
	String s1 = "";
	unsigned long age = 0;
	debug_out(F("output data json..."), DEBUG_MIN_INFO, 1);
	if (first_cycle) {
		s1 = FPSTR(data_first_part);
		s1.replace("{v}", String(SOFTWARE_VERSION));
		s1 += "]}";
		age = cfg::sending_intervall_ms - msSince(starttime);
		if (age > cfg::sending_intervall_ms) {
			age = 0;
		}
		age = 0 - age;
	} else {
		s1 = last_data_string;
		debug_out(F("last data: "), DEBUG_MIN_INFO, 0);
		debug_out(s1, DEBUG_MIN_INFO, 1);
		age = msSince(starttime);
		if (age > cfg::sending_intervall_ms) {
			age = 0;
		}
	}
	String s2 = F(", \"age\":\"");
	s2 += String((long)((age + 500) / 1000));
	s2 += String(F("\", \"measurements\":\""));
	s2 += String((unsigned long)(count_sends));
	s2 += F("\", \"sensordatavalues\"");
	debug_out(F("replace with: "), DEBUG_MIN_INFO, 0);
	debug_out(s2, DEBUG_MIN_INFO, 1);
	s1.replace(F(", \"sensordatavalues\""), s2);
	debug_out(F("replaced: "), DEBUG_MIN_INFO, 0);
	debug_out(s1, DEBUG_MIN_INFO, 1);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_JSON), s1);
}

/*****************************************************************
 * Webserver prometheus metrics endpoint                         *
 *****************************************************************/
void webserver_prometheus_endpoint() {
	debug_out(F("output prometheus endpoint..."), DEBUG_MIN_INFO, 1);
	String data_4_prometheus = F("software_version{version=\"{ver}\",{id}} 1\nuptime_ms{{id}} {up}\nsending_intervall_ms{{id}} {si}\nnumber_of_measurements{{id}} {cs}\n");
	String id = F("node=\"esp8266-");
	id += esp_chipid() + "\"";
	debug_out(F("Parse JSON for Prometheus"), DEBUG_MIN_INFO, 1);
	debug_out(last_data_string, DEBUG_MED_INFO, 1);
	data_4_prometheus.replace("{id}", id);
	data_4_prometheus.replace("{ver}", String(SOFTWARE_VERSION));
	data_4_prometheus.replace("{up}", String(msSince(time_point_device_start_ms)));
	data_4_prometheus.replace("{si}", String(cfg::sending_intervall_ms));
	data_4_prometheus.replace("{cs}", String(count_sends));
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json2data = jsonBuffer.parseObject(last_data_string);
	if (json2data.success()) {
		for (uint8_t i = 0; i < json2data["sensordatavalues"].size() - 1; i++) {
			String tmp_str = json2data["sensordatavalues"][i]["value_type"].as<char*>();
			data_4_prometheus += tmp_str + "{" + id + "} ";
			tmp_str = json2data["sensordatavalues"][i]["value"].as<char*>();
			data_4_prometheus += tmp_str + "\n";
		}
		data_4_prometheus += F("last_sample_age_ms{");
		data_4_prometheus += id + "} " + String(msSince(starttime)) + "\n";
	} else {
		debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
	}
	debug_out(data_4_prometheus, DEBUG_MED_INFO, 1);
	server.send(200, FPSTR(TXT_CONTENT_TYPE_TEXT_PLAIN), data_4_prometheus);
}



/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
void setup_webserver() {
	server.on("/", webserver_root);
	server.on("/config", webserver_config);
    server.on("/config.json", HTTP_GET, webserver_config_json);
    server.on("/configSave.json", webserver_config_json_save);
    server.on("/forceUpdate", webserver_config_force_update);
    server.on("/wifi", webserver_wifi);
	server.on("/values", webserver_values);
	server.on("/debug", webserver_debug_level);
	server.on("/removeConfig", webserver_removeConfig);
	server.on("/reset", webserver_reset);
	server.on("/data.json", webserver_data_json);
	server.on("/metrics", webserver_prometheus_endpoint);
	server.on("/images", webserver_images);
	server.onNotFound(webserver_not_found);

	debug_out(F("Starting Webserver... "), DEBUG_MIN_INFO, 0);
//	debug_out(IPAddress2String(WiFi.localIP()), DEBUG_MIN_INFO, 1);
	debug_out(WiFi.localIP().toString(), DEBUG_MIN_INFO, 1);
	server.begin();
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

#if defined(ESP8266)
#include "ca-root.h"
BearSSL::X509List x509_dst_root_ca(dst_root_ca_x3);

static void configureCACertTrustAnchor(WiFiClientSecure* client) {
    constexpr time_t fw_built_year = (__DATE__[ 7] - '0') * 1000 + \
							  (__DATE__[ 8] - '0') *  100 + \
							  (__DATE__[ 9] - '0') *   10 + \
							  (__DATE__[10] - '0');
    if (time(nullptr) < (fw_built_year - 1970) * 365 * 24 * 3600) {
        debug_out(F("Time incorrect; Disabling CA verification."), DEBUG_MIN_INFO,1);
        client->setInsecure();
    }
    else {
        client->setTrustAnchors(&x509_dst_root_ca);
    }
}
#endif

/*****************************************************************
 * send data to rest api                                         *
 *****************************************************************/
void sendData(const LoggerEntry logger, const String &data, const int pin, const char *host, const int httpPort, const char *url, const bool verify) {
    WiFiClient *client;
	const __FlashStringHelper *contentType;
    bool ssl = false;
    if (httpPort == 443) {
        client = new WiFiClientSecure;
        ssl = true;
        configureCACertTrustAnchor(static_cast<WiFiClientSecure *>(client));
        static_cast<WiFiClientSecure *>(client)->setBufferSizes(1024, TCP_MSS > 1024 ? 2048 : 1024);
    } else {
        client = new WiFiClient;
    }
    client->setTimeout(20000);
    int result = 0;
	
	switch (logger)
	{
	case LoggerInflux:
		contentType = FPSTR(TXT_CONTENT_TYPE_INFLUXDB);
		break;
	default:
		contentType = FPSTR(TXT_CONTENT_TYPE_JSON);
		break;
	}

    debug_out(F("Start connecting to "), DEBUG_MIN_INFO, 0);
    debug_out(host, DEBUG_MIN_INFO, 1);

    HTTPClient *http;
    http = new HTTPClient;
    http->setTimeout(20 * 1000);
    http->setUserAgent(String(SOFTWARE_VERSION) + "/" + esp_chipid());
    http->setReuse(false);
    bool send_success = false;
    debug_out(String(host), DEBUG_MIN_INFO, 1);
    debug_out(String(httpPort), DEBUG_MIN_INFO, 1);
    debug_out(String(url), DEBUG_MIN_INFO, 1);
    if (http->begin(*client, host, httpPort, url, ssl)) {
		if (logger == LoggerCustom && (*cfg::user_custom || *cfg::pwd_custom))
		{
			http->setAuthorization(cfg::user_custom, cfg::pwd_custom);
		}
		if (logger == LoggerInflux && (*cfg::user_influx || *cfg::pwd_influx))
		{
			http->setAuthorization(cfg::user_influx, cfg::pwd_influx);
		}
		
        http->addHeader(F("Content-Type"), contentType);
        http->addHeader(F("X-Sensor"), String(F("esp8266-")) + esp_chipid());
        if (pin) {
            http->addHeader(F("X-PIN"), String(pin));
        }
        
        result = http->POST(data);

        if (result >= HTTP_CODE_OK && result <= HTTP_CODE_ALREADY_REPORTED) {
            debug_out(F("Succeeded - "), DEBUG_MIN_INFO, 1);
            send_success = true;
        } else {
            debug_out(F("Not succeeded "), DEBUG_MIN_INFO, 1);
        }
        debug_out(F("Request result: "), DEBUG_MIN_INFO, 0);
        debug_out(String(result), DEBUG_MIN_INFO, 1);
        if (result != 204 && http->getString().length() > 0) {
            debug_out(F("Details:"), DEBUG_MIN_INFO, 1);
            debug_out(http->getString(), DEBUG_MIN_INFO, 1);
        }


    } else {
        debug_out(F("Failed connecting"), DEBUG_MIN_INFO, 1);
    }
    http->end();
    debug_out(F("End connecting to "), DEBUG_MIN_INFO, 0);
    debug_out(host, DEBUG_MIN_INFO, 1);
    delete (http);
    delete (client);

    wdt_reset(); // nodemcu is alive
    yield();
}

/*****************************************************************
 * send single sensor data to luftdaten.info api                 *
 *****************************************************************/
void sendLuftdaten(const String& data, const int pin, const char* host, const int httpPort, const char* url, const bool verify, const char* replace_str) {
    debugData(data,F("sendLuftdaten data in:"));
	String data_4_dusti = FPSTR(data_first_part);
    debugData(data_4_dusti,String(__LINE__));
    data_4_dusti.replace(String("{v}"), String(SOFTWARE_VERSION));
    debugData(data_4_dusti,String(__LINE__));
    data_4_dusti += data;
    debugData(data_4_dusti,String(__LINE__));
    debugData(data,String(__LINE__));
    data_4_dusti.remove(data_4_dusti.length() - 1);
    debugData(data_4_dusti,String(__LINE__));
    debugData(data,String(__LINE__));
    data_4_dusti.replace(replace_str, String(""));
    debugData(data_4_dusti,String(__LINE__));
    debugData(data,String(__LINE__));
    data_4_dusti += String("]}");
    debugData(data_4_dusti,String(__LINE__));
    debugData(data,String(__LINE__));
    if (data != "") {
        sendData(LoggerDusti, data_4_dusti, pin, host, httpPort, url, verify);
	} else {
		debug_out(F("No data sent..."), DEBUG_MIN_INFO, 1);
	}
    debugData(data_4_dusti,F("sendLuftdaten data4dusti:"));
    debugData(data,F("sendLuftdaten data out:"));
}

/*****************************************************************
 * send data to LoRa gateway                                     *
 *****************************************************************/
// void send_lora(const String& data) {
// }

/*****************************************************************
 * send data to mqtt api                                         *
 *****************************************************************/
// rejected (see issue #33)

/*****************************************************************
 * send data to influxdb                                         *
 *****************************************************************/
String create_influxdb_string(const String& data) {
	String data_4_influxdb = "";
	debug_out(F("Parse JSON for influx DB"), DEBUG_MIN_INFO, 1);
	debug_out(data, DEBUG_MIN_INFO, 1);
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json2data = jsonBuffer.parseObject(data);
	if (json2data.success()) {
	    bool first_line = true;
		data_4_influxdb += F("feinstaub,node=esp8266-");
		data_4_influxdb += esp_chipid() + " ";
		for (uint8_t i = 0; i < json2data["sensordatavalues"].size(); i++) {
            String tmp_str = "";
		    if (first_line)
		        first_line = false;
		    else
		        tmp_str += ",";
		    tmp_str += json2data["sensordatavalues"][i]["value_type"].as<char*>();
			tmp_str += "=";
			tmp_str += json2data["sensordatavalues"][i]["value"].as<char*>();
            data_4_influxdb += tmp_str;
        }
        data_4_influxdb += F(",measurements=");
		data_4_influxdb += String(count_sends+1);
        data_4_influxdb += F(",free=");
		data_4_influxdb += String(memoryStatsMin.freeHeap);
        data_4_influxdb += F(",frag=");
		data_4_influxdb += String(memoryStatsMax.frag);
        data_4_influxdb += F(",max_block=");
		data_4_influxdb += String(memoryStatsMin.maxFreeBlock);
        data_4_influxdb += F(",cont_stack=");
		data_4_influxdb += String(memoryStatsMin.freeContStack);
		data_4_influxdb += "\n";
	} else {
		debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
	}
	debug_out(data_4_influxdb,DEBUG_MIN_INFO,1);
	return data_4_influxdb;
}

/*****************************************************************
 * send data as csv to serial out                                *
 *****************************************************************/
void send_csv(const String& data) {
	StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
	JsonObject& json2data = jsonBuffer.parseObject(data);
	debug_out(F("CSV Output"), DEBUG_MIN_INFO, 1);
	debug_out(data, DEBUG_MIN_INFO, 1);
	if (json2data.success()) {
		String headline = F("Timestamp_ms;");
		String valueline = String(act_milli) + ";";
		for (uint8_t i = 0; i < json2data["sensordatavalues"].size(); i++) {
			String tmp_str = json2data["sensordatavalues"][i]["value_type"].as<char*>();
			headline += tmp_str + ";";
			tmp_str = json2data["sensordatavalues"][i]["value"].as<char*>();
			valueline += tmp_str + ";";
		}
		static bool first_csv_line = true;
		if (first_csv_line) {
			if (headline.length() > 0) {
				headline.remove(headline.length() - 1);
			}
			Serial.println(headline);
			first_csv_line = false;
		}
		if (valueline.length() > 0) {
			valueline.remove(valueline.length() - 1);
		}
		Serial.println(valueline);
	} else {
		debug_out(FPSTR(DBG_TXT_DATA_READ_FAILED), DEBUG_ERROR, 1);
	}
}


/*****************************************************************
 * read BMP280 sensor values                                     *
 *****************************************************************/
static String sensorBMP280() {
	String s;

	debug_out(String(FPSTR(DBG_TXT_START_READING)) + FPSTR(SENSORS_BMP280), DEBUG_MED_INFO, 1);

	const auto p = bmp280.readPressure();
	const auto t = bmp280.readTemperature();
	if (isnan(p) || isnan(t)) {
		last_value_BMP280_T = -128.0;
		last_value_BMP280_P = -1.0;
		debug_out(String(FPSTR(SENSORS_BMP280)) + FPSTR(DBG_TXT_COULDNT_BE_READ), DEBUG_ERROR, 1);
	} else {
		debug_out(FPSTR(DBG_TXT_TEMPERATURE), DEBUG_MIN_INFO, 0);
		debug_out(String(t) + " C", DEBUG_MIN_INFO, 1);
		debug_out(FPSTR(DBG_TXT_PRESSURE), DEBUG_MIN_INFO, 0);
		debug_out(Float2String(p / 100) + " hPa", DEBUG_MIN_INFO, 1);
		last_value_BMP280_T = t;
		last_value_BMP280_P = p;
		s += Value2Json(F("BMP280_pressure"), Float2String(last_value_BMP280_P));
		s += Value2Json(F("BMP280_temperature"), Float2String(last_value_BMP280_T));
	}
	debug_out("----", DEBUG_MIN_INFO, 1);

	debug_out(String(FPSTR(DBG_TXT_END_READING)) + FPSTR(SENSORS_BMP280), DEBUG_MED_INFO, 1);

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


static String displayGenerateFooter(unsigned int screen_count) {
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
	int screen_count = 0;
	int screens[5];
	int line_count = 0;
//	debug_out(F("output values to display..."), DEBUG_MAX_INFO, 1);
	if (cfg::pms_read) {
		pm10_value = last_value_PMS_P1;
		pm10_sensor = FPSTR(SENSORS_PMSx003);
		pm25_value = last_value_PMS_P2;
		pm25_sensor = FPSTR(SENSORS_PMSx003);
	}
	if (cfg::sds_read) {
		pm10_value = last_value_SDS_P1;
		pm10_sensor = FPSTR(SENSORS_SDS011);
		pm25_value = last_value_SDS_P2;
		pm25_sensor = FPSTR(SENSORS_SDS011);
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
	if (cfg::heca_read) {
		t_hc_value = last_value_HECA_T;
		h_hc_value = last_value_HECA_H;
	}
	if (cfg::gps_read) {
		lat_value = last_value_GPS_lat;
		lon_value = last_value_GPS_lon;
		alt_value = last_value_GPS_alt;
		gps_sensor = "NEO6M";
	}
	if (cfg::pms_read || cfg::sds_read) {
		screens[screen_count++] = 1;
	}
	if (cfg::dht_read || cfg::ds18b20_read || cfg::bmp280_read || cfg::bme280_read) {
		screens[screen_count++] = 2;
	}
	if (cfg::heca_read) {
		screens[screen_count++] = 3;
	}
	if (cfg::gps_read) {
		screens[screen_count++] = 4;
	}
	screens[screen_count++] = 5;	// Wifi info
	screens[screen_count++] = 6;	// chipID, firmware and count of measurements
	if (cfg::has_display || cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) {
		switch (screens[next_display_count % screen_count]) {
		case (1):
			display_header = pm25_sensor;
			if (pm25_sensor != pm10_sensor) {
				display_header += " / " + pm25_sensor;
			}
			display_lines[0] = "PM2.5: " + check_display_value(pm25_value, -1, 1, 6) + " µg/m³";
			display_lines[1] = "PM10:  " + check_display_value(pm10_value, -1, 1, 6) + " µg/m³";
			display_lines[2] = "";
			break;
		case (2):
			display_header = t_sensor;
			if (h_sensor != "" && t_sensor != h_sensor) {
				display_header += " / " + h_sensor;
			}
			if ((h_sensor != "" && p_sensor != "" && (h_sensor != p_sensor)) || (h_sensor == "" && p_sensor != "" && (t_sensor != p_sensor))) {
				display_header += " / " + p_sensor;
			}
			if (t_sensor != "") { display_lines[line_count++] = "Temp.: " + check_display_value(t_value, -128, 1, 6) + " °C"; }
			if (h_sensor != "") { display_lines[line_count++] = "Hum.:  " + check_display_value(h_value, -1, 1, 6) + " %"; }
			if (p_sensor != "") { display_lines[line_count++] = "Pres.: " + check_display_value(p_value / 100, (-1 / 100.0), 1, 6) + " hPa"; }
			while (line_count < 3) { display_lines[line_count++] = ""; }
			break;
		case (3):
			display_header = F("NAM HECA (SHT30)");
			display_lines[0] = "Temp.: " + check_display_value(t_hc_value, -128, 1, 6) + " °C";
			display_lines[1] = "Hum.:  " + check_display_value(h_hc_value, -1, 1, 6) + " %";
			display_lines[2] = "PTC HE: Soon :)"; // PTC heater status
			break;
		case (4):
			display_header = gps_sensor;
			display_lines[0] = "Lat: " + check_display_value(lat_value, -200.0, 6, 10);
			display_lines[1] = "Lon: " + check_display_value(lon_value, -200.0, 6, 10);
			display_lines[2] = "Alt: " + check_display_value(alt_value, -1000.0, 2, 10);
			break;
		case (5):
			display_header = F("Wifi info");
			display_lines[0] = "IP: " + WiFi.localIP().toString();
			display_lines[1] = "SSID:" + WiFi.SSID();
			display_lines[2] = "Signal: " + String(calcWiFiSignalQuality(WiFi.RSSI())) + "%";
			break;
		case (6):
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
			display->drawString(64, 52, displayGenerateFooter(screen_count));
			display->display();
		}
		if (cfg::has_lcd2004_27 || cfg::has_lcd2004_3f) {
			display_header = String((next_display_count % screen_count) + 1) + "/" + String(screen_count) + " " + display_header;
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

// ----5----0----5----0
// PM10/2.5: 1999/999
// T/H: -10.0°C/100.0%
// T/P: -10.0°C/1000hPa

	switch (screens[next_display_count % screen_count]) {
	case (1):
		display_lines[0] = "PM2.5: " + check_display_value(pm25_value, -1, 1, 6);
		display_lines[1] = "PM10:  " + check_display_value(pm10_value, -1, 1, 6);
		break;
	case (2):
		display_lines[0] = "T: " + check_display_value(t_value, -128, 1, 6) + char(223) + "C";
		display_lines[1] = "H: " + check_display_value(h_value, -1, 1, 6) + "%";
		break;
	case (3):
		display_lines[0] = "Lat: " + check_display_value(lat_value, -200.0, 6, 11);
		display_lines[1] = "Lon: " + check_display_value(lon_value, -200.0, 6, 11);
		break;
	case (4):
		display_lines[0] = WiFi.localIP().toString();
		display_lines[1] = WiFi.SSID();
		break;
	case (5):
		display_lines[0] = "ID: " + esp_chipid();
		display_lines[1] = "FW: " + String(SOFTWARE_VERSION);
		break;
	}

	if (cfg::has_lcd1602_27 || cfg::has_lcd1602) {
		char_lcd->clear();
		char_lcd->setCursor(0, 0);
		char_lcd->print(display_lines[0]);
		char_lcd->setCursor(0, 1);
		char_lcd->print(display_lines[1]);
	}
	yield();
	next_display_count += 1;
	next_display_millis = millis() + DISPLAY_UPDATE_INTERVAL_MS;
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

/*****************************************************************
 * Init BMP280                                                   *
 *****************************************************************/
bool initBMP280(char addr) {
	debug_out(F("Trying BMP280 sensor on "), DEBUG_MIN_INFO, 0);
	debug_out(String(addr, HEX), DEBUG_MIN_INFO, 0);

	if (bmp280.begin(addr)) {
		debug_out(F(" ... found"), DEBUG_MIN_INFO, 1);
		return true;
	} else {
		debug_out(F(" ... not found"), DEBUG_MIN_INFO, 1);
		return false;
	}
}

static void powerOnTestSensors() {
     debug_out(F("PowerOnTest"),0,1);
    cfg::debug = DEBUG_MED_INFO;
	if (cfg::sds_read) {
		debug_out(F("Read SDS..."), 0, 1);
		SDS_cmd(PmSensorCmd::Start);
		delay(100);
		SDS_cmd(PmSensorCmd::ContinuousMode);
        delay(100);
        int pm10 = 0, pm25 = 0;
        unsigned timeOutCount = 0;
        while (timeOutCount < 200 && (pm10 == 0 || pm25 == 0)) {
            readSingleSDSPacket(&pm10, &pm25);
            timeOutCount++;
            delay(5);
        }
        if (timeOutCount == 200) {
            debug_out(F("SDS011 not sending data!"), DEBUG_ERROR, 1);
        } else {
            debug_out(F("PM10/2.5:"),  DEBUG_ERROR,1);
            debug_out(String(pm10/10.0,2),DEBUG_ERROR,1);
            debug_out(String(pm25/10.0,2),DEBUG_ERROR,1);
            last_value_SDS_P1 = double(pm10/10.0);
            last_value_SDS_P2 = double(pm25/10.0);
        }
        debug_out(F("Stopping SDS011..."),  DEBUG_ERROR,1);
        is_SDS_running = SDS_cmd(PmSensorCmd::Stop);
	}

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


	if (cfg::bmp280_read) {
		debug_out(F("Read BMP280..."), DEBUG_MIN_INFO, 1);
		if (!initBMP280(0x76) && !initBMP280(0x77)) {
			debug_out(F("Check BMP280 wiring"), DEBUG_MIN_INFO, 1);
			bmp280_init_failed = 1;
		}
	}

	if (cfg::bme280_read) {
		debug_out(F("Read BME280..."), DEBUG_MIN_INFO, 1);
		if (!initBME280(0x76) && !initBME280(0x77)) {
			debug_out(F("Check BME280 wiring"), DEBUG_MIN_INFO, 1);
			bme280_init_failed = 1;
		} else {
            sensorBME280();
        }
	}

	if (cfg::heca_read) {
		debug_out(F("Read HECA (SHT30)..."), DEBUG_MIN_INFO, 1);
		if (!initHECA()) {
			debug_out(F("Check HECA (SHT30) wiring"), DEBUG_MIN_INFO, 1);
			heca_init_failed = 1;
		} else {
		    sensorHECA();
		    debug_out(F("Temp: "),DEBUG_MIN_INFO,0);
		    debug_out(String(last_value_HECA_T,2),DEBUG_MIN_INFO,1);
            debug_out(F("Hum: "),DEBUG_MIN_INFO,0);
            debug_out(String(last_value_HECA_H,2),DEBUG_MIN_INFO,1);
		}
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
	debug_out(F("NTP.org:"),DEBUG_MIN_INFO,1);
	settimeofday_cb(time_is_set);
	configTime(0, 0, "pool.ntp.org");
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
    serialSDS.begin(9600);

    Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);

    cfg::initNonTrivials(esp_chipid().c_str());

    Serial.print(F("\nNAMF ver: "));
    Serial.print(SOFTWARE_VERSION);
    Serial.print(F("/"));
    Serial.println(INTL_LANG);
    Serial.print(F("Chip ID: "));
    Serial.println(esp_chipid());

    readConfig();
    resetMemoryStats();

    init_display();
    init_lcd();

    powerOnTestSensors();

    setup_webserver();
    display_debug(F("Connecting to"), String(cfg::wlanssid));
    debug_out(F("SSID: '"), DEBUG_ERROR, 0);
    debug_out(cfg::wlanssid, DEBUG_ERROR, 0);
    debug_out(F("'"), DEBUG_ERROR, 1);

    if (strlen(cfg::wlanssid) > 0) {
        connectWifi();
        got_ntp = acquireNetworkTime();
        debug_out(F("\nNTP time "), DEBUG_MIN_INFO, 0);
        debug_out(String(got_ntp ? "" : "not ") + F("received"), DEBUG_MIN_INFO, 1);
        updateFW();
    } else {
        startAP();
    }

    if (cfg::winsen_mhz14a_read) {
        serialGPS.begin(9600);
        setupWinsenMHZ(serialGPS);
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
        MDNS.addService("http", "tcp", 80);

    } else {
        debug_out(F("\nmDNS failure!"), DEBUG_ERROR, 1);
    }

    delay(50);

	// sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
	wdt_disable();
	wdt_enable(30000);

	starttime = millis();                                   // store the start time
	time_point_device_start_ms = starttime;
	starttime_SDS = starttime;
	next_display_millis = starttime + DISPLAY_UPDATE_INTERVAL_MS;



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
    debugData(data,F("po madavi:"));

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
        debugData(data,F("influx start:"));
        debug_out(String(FPSTR(DBG_TXT_SENDING_TO)) + F("custom influx db: "), DEBUG_MIN_INFO, 1);
        start_send = millis();
        const String data_4_influxdb = create_influxdb_string(data);
        debugData(data,F("po influx_string:"));

        sendData(LoggerInflux, data_4_influxdb, 0, cfg::host_influx, cfg::port_influx, cfg::url_influx, false);
		sum_send_time += millis() - start_send;
    }
    debugData(data,F("po influx "));

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
    debugData(data,F("po custom "));

    return sum_send_time;
}

/*****************************************************************
 * And action                                                    *
 *****************************************************************/
void loop() {
	String result_SDS = "";
	String result_PMS = "";
	String result_DHT = "";
	String result_BMP280 = "";
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

	wdt_reset(); // nodemcu is alive

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
#if !defined(BOOT_FW)
	if ((msSince(starttime_SDS) > SAMPLETIME_SDS_MS) || send_now) {
		if (cfg::sds_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + "SDS", DEBUG_MAX_INFO, 1);
			result_SDS = sensorSDS();
			starttime_SDS = act_milli;
		}

		if (cfg::pms_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + "PMS", DEBUG_MAX_INFO, 1);
			result_PMS = sensorPMS();
			starttime_SDS = act_milli;
		}

	}
    if (cfg::winsen_mhz14a_read)
	    readWinsenMHZ(serialGPS);

	server.handleClient();

	if (send_now) {
        debugData(String(F("****************** Upload data to APIs*****************************")));
        if (cfg::dht_read) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_DHT22), DEBUG_MAX_INFO, 1);
			result_DHT = sensorDHT();                       // getting temperature and humidity (optional)
		}

		if (cfg::bmp280_read && (! bmp280_init_failed)) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_BMP280), DEBUG_MAX_INFO, 1);
			result_BMP280 = sensorBMP280();                 // getting temperature, humidity and pressure (optional)
		}

		if (cfg::bme280_read && (! bme280_init_failed)) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_BME280), DEBUG_MAX_INFO, 1);
            debugData(result_BME280,F("BME  przed:"));
			result_BME280 = sensorBME280();                 // getting temperature, humidity and pressure (optional)
            debugData(result_BME280,F("BME po:"));

        }

		if (cfg::heca_read && (! heca_init_failed)) {
			debug_out(String(FPSTR(DBG_TXT_CALL_SENSOR)) + FPSTR(SENSORS_HECA), DEBUG_MAX_INFO, 1);
			debugData(result_HECA,F("HECA  przed:"));
			result_HECA = sensorHECA();                 // getting temperature, humidity and pressure (optional)
            debugData(result_HECA,F("HECA  po:"));
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

	if ((cfg::has_display || cfg::has_lcd2004_27 || cfg::has_lcd2004_3f || cfg::has_lcd1602 ||
			cfg::has_lcd1602_27) && (act_milli > next_display_millis)) {
		display_values();
	}

	if ((cfg::has_ledbar_32) && (send_now)) {
		displayLEDBar();
	}

	if (send_now) {
		debug_out(F("Creating data string:"), DEBUG_MIN_INFO, 1);
		String data = FPSTR(data_first_part);
		data.replace("{v}", String(SOFTWARE_VERSION));
		String data_sample_times  = Value2Json(F("samples"), String(sample_count));
		data_sample_times += Value2Json(String(F("min_micro")), String(min_micro));
		data_sample_times += Value2Json(String(F("max_micro")), String(max_micro));

		String signal_strength = String(WiFi.RSSI());
		debug_out(F("WLAN signal strength: "), DEBUG_MIN_INFO, 0);
		debug_out(signal_strength, DEBUG_MIN_INFO, 0);
		debug_out(" dBm", DEBUG_MIN_INFO, 1);
		debug_out("----", DEBUG_MIN_INFO, 1);

		server.handleClient();
		yield();
		server.stop();
		const int HTTP_PORT_DUSTI = (cfg::ssl_dusti ? 443 : 80);
		if (cfg::sds_read) {
			data += result_SDS;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(SDS): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_SDS, SDS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "SDS_");
				sum_send_time += millis() - start_send;
			}
		}
        debugData(data,F("po SDS/Luft "));

        if (cfg::pms_read) {
			data += result_PMS;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(PMS): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_PMS, PMS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "PMS_");
				sum_send_time += millis() - start_send;
			}
		}
		if (cfg::dht_read) {
			data += result_DHT;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DHT): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_DHT, DHT_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DHT_");
				sum_send_time += millis() - start_send;
			}
		}
        debugData(data,F("po DHT "));
		if (cfg::bmp280_read && (! bmp280_init_failed)) {
			data += result_BMP280;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(BMP280): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_BMP280, BMP280_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "BMP280_");
				sum_send_time += millis() - start_send;
			}
		}
		if (cfg::bme280_read && (! bme280_init_failed)) {
			data += result_BME280;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(BME280): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
                debugData(data,F("przed SendLuftdaten:"));
				sendLuftdaten(result_BME280, BME280_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "BME280_");
				sum_send_time += millis() - start_send;
			}
		}
        debugData(data,F("po BME280 "));

        if (cfg::heca_read && (! heca_init_failed)) {
			data += result_HECA;
//			if (cfg::send2dusti) {
//				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(HECA): "), DEBUG_MIN_INFO, 1);
//				start_send = millis();
//				sendLuftdaten(result_HECA, HECA_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "HECA_");
//				sum_send_time += millis() - start_send;
//			}
		}
        debugData(data,F("po HECA "));

		if (cfg::ds18b20_read) {
			data += result_DS18B20;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(DS18B20): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_DS18B20, DS18B20_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "DS18B20_");
				sum_send_time += millis() - start_send;
			}
		}

		if (cfg::gps_read) {
			data += result_GPS;
			if (cfg::send2dusti) {
				debug_out(String(FPSTR(DBG_TXT_SENDING_TO_LUFTDATEN)) + F("(GPS): "), DEBUG_MIN_INFO, 1);
				start_send = millis();
				sendLuftdaten(result_GPS, GPS_API_PIN, HOST_DUSTI, HTTP_PORT_DUSTI, URL_DUSTI, true, "GPS_");
				sum_send_time += millis() - start_send;
			}
		}

        if(cfg::winsen_mhz14a_read)
		    data += sensorMHZ();

		data_sample_times += Value2Json("signal", signal_strength);
        data_sample_times.remove(data_sample_times.length()-1);
		data += data_sample_times;

		data += "]}";
        debugData(data,F("po Luftdaten "));

		sum_send_time += sendDataToOptionalApis(data);

		server.begin();

		checkForceRestart();

		if (msSince(last_update_attempt) > PAUSE_BETWEEN_UPDATE_ATTEMPTS_MS) {
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
