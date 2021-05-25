# NAMF internals

## HECA
With release 38rcX NAMF reports more info about HECA. SHT31 used in HECA has alarm feature. It tracks RH and temperature
and when is out of given presets _ALARM_ pin on SHT31 is enabled. We use that to control
heating element when air is too cold or too humid.

On `/status` page HECA reports duty cycle. This is how long alert was enabled due to humidity (`DutyCycleRH`) or 
temperature (`DutyCycleTemp`). `DutyCycle` is total time when alarm was active, no matter what condition
has triggered it. DC is in %, so 50% means that alarm (and heater) was on during half time since last sending data.

Duty Cycle shown on `/status` page, and on LCD is from current cycle (so, it will be changing if heater is enabled). 
Values presented in `/data.json` are from previous cycle (sent to APIs).
