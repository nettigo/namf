//
// Created by viciu on 17.02.2020.
//

#ifndef NAMF_WEBSERVER_H
#define NAMF_WEBSERVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include "defines.h"
#include "variables.h"
#include "helpers.h"
#include "display/commons.h"
#include "system/scheduler.h"
#include "system/components.h"
#include "html-content.h"

extern String table_row_from_value(const String& sensor, const String& param, const String& value, const String& unit);
extern void updateFW(const String host, const String port, const String path);
extern void updateFW();

//latest stack dump
void webserver_dump_stack();

void sendHttpRedirect(ESP8266WebServer &httpServer);
/*****************************************************************
 * Webserver Images                                              *
 *****************************************************************/
void webserver_images();
/*****************************************************************
 * Webserver page not found                                      *
 *****************************************************************/
void webserver_not_found();
/*****************************************************************
 * Webserver request auth: prompt for BasicAuth
 *
 * -Provide BasicAuth for all page contexts except /values and images
 *****************************************************************/
bool webserver_request_auth();

/*****************************************************************
 * Webserver root: show all options                              *
 *****************************************************************/
void webserver_root();

/*****************************************************************
 * html helper functions                                         *
 *****************************************************************/
void webserverPartialSend(String &s);
String make_header(const String& title, bool configPage=false);
String make_footer(bool configPage = false);
//Webserver - current config as JSON (txt) to save
void webserver_config_json();
//Webserver - force update with custom URL
void webserver_config_force_update();
//Webserver - current config as JSON (txt) to save
void webserver_config_json_save();
void webserver_config();

/*****************************************************************
 * Webserver wifi: show available wifi networks                  *
 *****************************************************************/
void webserver_wifi();

/*****************************************************************
 * Webserver root: show latest values                            *
 *****************************************************************/
void webserver_values();

/*****************************************************************
 * Webserver set debug level                                     *
 *****************************************************************/
void webserver_debug_level();

/*****************************************************************
 * Webserver remove config                                       *
 *****************************************************************/
void webserver_removeConfig();
/*****************************************************************
 * Webserver reset NodeMCU                                       *
 *****************************************************************/
void webserver_reset();
/*****************************************************************
 * Webserver data.json                                           *
 *****************************************************************/
void webserver_data_json();
/*****************************************************************
 * Webserver prometheus metrics endpoint                         *
 *****************************************************************/
void webserver_prometheus_endpoint();
/*****************************************************************
 * Webserver setup                                               *
 *****************************************************************/
void setup_webserver();

void webserver_status_page();

#endif //NAMF_WEBSERVER_H
