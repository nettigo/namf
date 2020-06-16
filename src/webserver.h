//
// Created by viciu on 17.02.2020.
//

#ifndef NAMF_WEBSERVER_H
#define NAMF_WEBSERVER_H

#include <Arduino.h>
#include "defines.h"
#include "variables.h"
#include "helpers.h"
#include "html-content.h"
String table_row_from_value(const String& sensor, const String& param, const String& value, const String& unit);
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

String make_header(const String& title);
String make_footer();
//Webserver - current config as JSON (txt) to save
void webserver_config_json();
//Webserver - force update with custom URL
void webserver_config_force_update();
//Webserver - current config as JSON (txt) to save
void webserver_config_json_save();
void webserver_config();

#endif //NAMF_WEBSERVER_H
