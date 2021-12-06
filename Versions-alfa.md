# List of changes in alfa versions

This is list of test/experiment firmwares. If You need to see what have changed in stable/beta see [Versions.md](Versions.md)

### NAMF-2020-42a2 (2021-12-1 rev )
* moved more variables to dynamic memory
* new option - Fallback WiFi - on startup, if NAM can not connect to WiFi and fallback is configured then connects to second network.


### NAMF-2020-42a1 (2021-11-29 rev cbcefe113638bfa72fc66634ce49d9bf50e9773b)
* moved some crucial variables (SSIDs, pwds) to dynamic memory

### NAMF-2020-39a7 (2021-07-22) (rev 524f9b9ff6045a1be86d9755877b61c790b76525)
* SDS011 - count number of power off/on cycles (hw watchdog) 

### NAMF-2020-39a6 (2021-06-17) (rev bb6a2b09283416f22a8403eb7f3b618e675b4439)
* reset failed SDS measurements counter when good measurement is taken

### NAMF-2020-39a5 (2021-06-16) (rev d6111748003dbf4a12ac0bc5b0fd7f3260541aa3)
* enable only one PCF channel in SDS restarter (only one LED is enabled)
* collect failed SDS packets count (checksum error)

### NAMF-2020-39a4 (2021-06-16) (rev c1110cde625c6fd0daadfb50adfe59caa425f29b)
* hardware SDS restarter support
* code cleanup

### NAMF-2020-39a3 (2021-06-15) (rev c50ee76fbc50d6c59dc495eb8bb09ab6d38ee92c)
* SDS011 - respect warmup time set in config. For now measurement is fixed and its 10 seconds
no matter what is set in config
* SDS011 - better timing for operation to get first measurement ready before sending data 

### NAMF-2020-39a2 (2021-06-15) (rev f6923c5e079c16461bd45c86b575325b81e2f028)
* SDS version uses new code
* Platformio Espressif8266@3.0.0
* code cleanup

### NAMF-2020-39a1 (2021-06-15) (rev 6f0759c68f8f3ba3928e1a456d2481c6014f4e2b)

* SDS is serviced with https://github.com/dok-net/esp_sds011 
* Arduino core - 3.0.1dev

### NAMF-2020-38a5 (2021-06-14) (rev e769f87a22e016ca6750df2d45e75e0fc4c923bc)
* new SDS code, support for SDS hw watchdog (extra module) 

### NAMF-2020-38a3 (2021-05-17)

* sync to beta 38rc5 (no current work in alfa, so we keep in sync with beta)

### NAMF-2020-37a10

* send -1 as SDS reading on error. Only to Influx,other loggers are not affected

### NAMF-2020-37a9

* nice confirmation page on device restart
* SDS displays on LCD (not on OLED)

### NAMF-2020-37a8

* drop old display for SDS (HTML on /values page)
* Network Watchdog reports state on /status not /values

### NAMF-2020-37a7

* respect send to SC setting in new scheduler
* different update check intervals for different update channels


### NAMF-2020-37a6

* if old cfg::sds_read is set, then always enable new SDS support
* firmware version in header is linked to changelog 

### NAMF-2020-37a5

* SDS011 code moved to new scheduler
* sensor shows both time from last measurement and time left to next one

### NAMF-2020-37a4 
* sending to influx FW version and update channel
* always send to influx SDS results (when sensor is enabled), no reading is sent as -1

### NAMF-2020-37a3

* new communication with SDS (ported from Sensor Community fw, branch master on March 31st)
