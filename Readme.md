![GitHub release (latest by date)](https://img.shields.io/github/v/release/nettigo/namf) ![GitHub](https://img.shields.io/github/license/nettigo/namf) ![Hardware](https://img.shields.io/badge/hardware%20license-TAPR-orange) ![GitHub issues](https://img.shields.io/github/issues/nettigo/namf)

# Nettigo Air Monitor Firmware

NAMF is a firmware for [Nettigo Air Monitor Sensor](https://nettigo.eu/products/tagged/NAM) - a modular platform for measuring air quality. Hardware is OSHWA Certified with [UID PL000001](https://certification.oshwa.org/pl000001.html).

NAM Sensor features WiFi connectivity provided by ESP8266 (and ESP32 in future). It's build around Nova Fitness SDS011 PM sensor. Device has a build-in PTC heater with control circuitry for air conditioning in a high humidity environment (to avoid counting water vapor as dust).

### Changelog

The code base has roots in [Luftdaten.info firmware](https://github.com/opendata-stuttgart/sensors-software/). We aim to rewrite the code to make it modular, easier to modify and extend in future. 

Detailed changelog is in [Versions](Versions.md).

To build this project You need Platformio installed with python 3.

### Firmware web interface

* / - main menu
* Data related:
  * /values - current measured values (page always available)
  * /status - sensor status and debug info
  * /data.json - measured values in JSON format (this is snapshot of last data set sent to APIs not current values as shown on /values page)
  * /metrics - measured values in Prometheus format
* Configuration related:
  * /config - configuration
  * /simple_config - interface for new scheduler subsystems configuration  
  * /removeConfig - remove configuration files
  * /config.json - current configure file in JSON format
  * /configSave.json - form for pasting configuration file
  * /forceUpdate - form for changing update server or link to other binary file
  * /wifi - list of wifi networks (active only in AP mode)
* /stack_dump - show stack dump from last exception with timestamp, FW version, language and MD5 sum of image  
* /reset - sensor reboot
* /ota - enable OTA firmware update for 60 seconds. Will work only if admin password is set and enabled
* /debug?lvl=x - sets debug serial messages info level to x:
  * 0 - no debug
  * 1 - errors
  * 2 - errors & warnings
  * 3 - errors, warnings & min. info
  * 4 - errors, warnings, min. info & med. info
  * 5 - all debug messages
* /images - serving images - used for the NAM logo 

## For developers and interested in

In [Details](Details.md) You can find a bit more info about NAMF internals.