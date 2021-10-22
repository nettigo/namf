# Diagnostic data

Each NAM sensor is sending diagnostic data to Nettigo. We don't collect any personal information,
these data will help us understand how sensors are being used and monitor software
status.

You can opt-out from sending diagnostic data to Nettigo on config page. We ask You to leave sending diagnostic 
data to us, that allows us making better software for NAM.

## What is being sent to Nettigo?

This document will record what data is being sent. We don't collect WiFi passwords :) IP addresses are not stored 
(of course web server has it's own logs, but they are rotated and that information is lost after some time) 

Each time sensor start it sends:

* software version, firmware MD5 checksum, which software update channel it uses and if auto update is enabled
* restart reason (power on/exception/watchdog etc)
* which subsystems from new scheduler are enabled

Periodically it sends:
* max/min memory usage
* max/min WiFi signal strength
* if SDS is enabled:
  * total number of readings and number of failed readings
  * SDS packets checksum error rate
  * if hardware watchdog for SDS is present and enabled: how many times it was active