# LCD backlight

You can disable LCD backlight in night hours. To do it - just enter hour when backlight should
be disabled (__Stop__) and when it should be enabled again (__Start__). 

For example `Stop`: _22_ and `Start`: _06_ will disable LCD backlight on 22:00 and re-enable it on 6:00.

To have backlight enabled all the time - enter at least one value 25 or bigger. If sensor can not
acquire time from NTP feature won't be working no matter what is placed as `Start` and `Stop` values.
