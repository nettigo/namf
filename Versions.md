NAMF 2020-17
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

