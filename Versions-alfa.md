# List of changes in alfa versions

This is list of test/experiment firmwares. If You need to see what have changed in stable/beta see [Versions.md](Versions.md)

### NAMF-2020-38a3 (2021-06-14)
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
