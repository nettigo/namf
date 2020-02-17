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
#endif //NAMF_WEBSERVER_H
