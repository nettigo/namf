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

#endif //NAMF_WEBSERVER_H
