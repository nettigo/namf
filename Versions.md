NAMF-2020-33 (2020-08-22)
* stable relase based on rc3

NAMF-2020-33rc3 (2020-08-21)
* SPS30 - added missing 0.5 particles count reading 

NAMF-2020-33rc2 (2020-08-14)
* version bump to check autoupdate software option
* increased update check interval from 1h to 6h
* enabling SPS30 w/o connected hardware should not stop whole sensor
* use UTF-8 explict when open language files (required on Windows to compile) 

NAMF-2020-33rc1 (2020-08-13)
* respect autoupdate option

NAMF 2020-32 (2020-08-01)
* Updated missing translations in Polish

NAMF 2020-31 (2020-07-08)
* new scheduler
* SPS30 support (high precision particular matter sensor from Sensirion)
* network watchdog - restart sensor if can not ping given IP for three times in row (10 pings each time)

NAMF 2020-30 (2020-06-02)
* CPU clock boosted to 160 MHz
* display info about free SPIFFS and CPU freq on start (Serial)
* First green LED on LED bar now starts from 0.1 instead 1.
* Option to set WiFi PHY Mode to extend range

NAMF 2020-29 (2020-05-18)
* in case of crash, stack dump will be saved in /stack_dump file on SPIFFS
* new url /stack_dump shows latest stack dump

NAMF 2020-28 (2020-04-24)
* option to disable displaying WiFi SSID/IP addr
* code cleanup - moved some display functions to separate files

NAMF 2020-27 (2020-04-05)
* fixed malformed JSON in /data.json (merged PR#5)

NAMF 2020-26 (2020-03-17)
* added I2C RGB LED Bar support

NAMF 2020-25 (2020-03-09)
* merged PR#4 with Logger differentiation based on Luftdaten code
 
NAMF 2020-24 (2020-03-08)
* added Content-Length to headers when sending data to API. Should fix issue #3 

NAMF 2020-23 (2020-03-3)
* technical release to make sure binaries on update server correspond to code in GH

NAMF 2020-22 (2020-03-3)
* check for updates every hour
* don't check for update every time when sending data 

NAMF 2020-21 (2020-02-27)
* default Winsen MH-Z14A CO2 range set to 2000 ppm
* Winsens averaging function should not lower result in first measurement
* polish fw can be built with build timestamp in page header (create .add_build_time empty file in root directory)

NAMF 2020-20 (2020-02-22)
* bugfix in Winsen CO2 averaging function

NAMF 2020-19 (2020-02-22)
* both BME280 and HECA should not send to API results when communication error (still -128°C as temperature in WWW intreface)
* changed BME280 library to fork with fixed hangs. Waiting for AF to include changes
* (almost) all logs on Serial are with timestamps
* don't send HECA logs to Luftdaten API
* support for CO2 sensor - Winsen MH-Z14A, connect it to D6/D5 instead of GPS. So CO2 sensor or GPS no both  

NAMF 2020-18 (2020-02-21)
* we return to 80MHz on ESP8266
* VTables in DRAM
* bugfixes, hope to get more reliable

NAMF 2020-17 (2020-02-19)
* uptime is nice formatted not just seconds
* outputPower can be adjusted also in AP mode

NAMF 2020-16 (2020-02-19)
* hostname for DHCP server
* interface changes: removed map link, utf-8 icons in main menu
* interface changes: removeConfig moved to bottom, save & restart is now red, more icons
* config AP SSID is now mDNS and DHCP hostname. Using UTF-8 special chars and dot (".") may mess with propper mDNS resolving.
* disabled HTTP 1.1 support in HTTP client
* removed unused links /generate_204 & /fwlink
* readme update
* lang files update
* sensor name based on AP SSID visible in header
* removed int_templ.h 

NAMF 2020-15 (2020-02-18)
* /forceUpdate will now work with custom URL, until now it worked only with default
* new config option - WiFi TX output power

NAMF 2020-14 (2020-02-17)
* HECA support moved to src/sensors/heca
* started move webserver functions to separate file
* stopped software version spoofing

NAMF 2020-13 (2020.02.17)
* fix for garbage auth data when sending to Luftdaten/Madavi

NAMF 2020-12 (2020.02.17)
* allow to disable character LCD
* remove config button is now orange to avoid confusion with restart button
* fixed basic auth when sending to influx and custom API

NAMF 2020-11 (2020.02.16)
* moved to dynamic definition of LCDs (both character and graphical) Less code, less memory
* since only one char LCD is supported in given config HTML was changed to dropdown on config page
* memory stats are being collected in more logic way (max/min values measured at end of each loop)
* WiFi password is visible in input field when sensor is being first time configured. When already configured sensor is booted in AP mode, password is not shown (thus saving form with empty field will remove password from config)  

NAMF 2020-10 (2020.02.14)
* new core 2.6.3
* added Core & SDK version to measurements page
* more internal parameters (heap frag, free mem etc) is being sent to Influx
* a lots love with st Valentine's day!

NAMF 2020-9 (2020.02.13)
* /data.json now returns also measurements count
* added MaxFreeBlockSize and HeapFragmentation to influx and diagnostics (on current values pages)
* added Uptime, Reset Reason, Free Memory and Heap Fragmentation to measurements page

NAMF 2020-8 (2020.02.11)
* mDNS brought back
* to influx sensor sends also amount of free memory
    
NAMF 2020-7 (2020.02.10)
* memory leak in sendData fixed
* new target with stage boot firmware (migration from old firmware with Flash 1:3)

NAMF 2020-6
* SSL/plain connection fixes

NAMF 2020-5
* updating libraries and core to most recent
* SSL seems to be working 

NAMF 2020-4
* forceUpdate - page to force update from custom URL
* provide way to display current config as JSON and upload it later to sensor (/configSave.json)

NAMF 2020-3
* display on webpage first reading from SDS011 (collected on POST)

NAMF 2020-2 (2020-01-25)
* OTA update working

NAMF 2020-1
* Firmware separated from Luftdaten project structure
* New flash layout (2m for code, 2m for SPIFFS). 

