#ifndef NAMF_HTML_CONTENT_H
#define NAMF_HTML_CONTENT_H

const char TXT_CONTENT_TYPE_JSON[] PROGMEM = "application/json";
const char TXT_CONTENT_TYPE_INFLUXDB[] PROGMEM = "application/x-www-form-urlencoded";
const char TXT_CONTENT_TYPE_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char TXT_CONTENT_TYPE_TEXT_PLAIN[] PROGMEM = "text/plain";
const char TXT_CONTENT_TYPE_IMAGE_SVG[] PROGMEM = "image/svg+xml";


const char SENSORS_SDS011[] PROGMEM = "SDS011";
const char SENSORS_DHT22[] PROGMEM = "DHT22";
const char SENSORS_PMSx003[] PROGMEM = "PMSx003";
const char SENSORS_DS18B20[] PROGMEM = "DS18B20";
const char SENSORS_BMP280[] PROGMEM = "BMP280";
const char SENSORS_BME280[] PROGMEM = "BME280";
const char SENSORS_HECA[] PROGMEM = "HECA";

const char WEB_PAGE_HEADER[] PROGMEM = "<!DOCTYPE html><html>\
<head>\
<title>{t}</title>\
<meta name='viewport' content='width=device-width'>\
<style type='text/css'>\
body{font-family:Arial;margin:0;}\
.content{margin:10px;}\
.spacer{padding:10px;}\
.r{text-align:right;}\
td{vertical-align:top;}\
a{text-decoration:none;padding:10px;background:#2B4;color:white;display:block;width:auto;border-radius:5px;}\
a:hover{background:#1A4};\
.wifi{background:none;color:blue;padding:5px;display:inline;}\
input[type='text']{width:100%;}\
input[type='password']{width:100%;}\
input[type='submit']{border-radius:5px;font-size:medium;padding:5px;}\
.s_red{padding:9px !important;width:100%;border-style:none;background:#D44;color:white;text-align:left;}\
.s_red:hover {background:#E33;}\
.s_orange{background:#F80;}\
.s_orange:hover{background:#E70;}\
#ncf hr{margin:25px 10px;}\
#ncf {max-width:800px;}\
</style>\
</head><body>\
<div style='min-height:135px;background-color:#2B4;margin-bottom:20px'>\
<a href='/' style='background:none;width:0;display:inline'><img src='/images?name=luftdaten_logo' style='float:left;margin:20px'/></a>\
<h3 style='margin:0'>{tt}<br>{sname}</h3>\
<small>ID: {id}<br/>MAC: {mac}<br/>{fwt}: {fw}</small></div><div class='content'><h4>{h} {n} {t}</h4>";

const char BR_TAG[] PROGMEM = "<br/>";
const char TABLE_TAG_OPEN[] PROGMEM = "<table>";
const char TABLE_TAG_CLOSE_BR[] PROGMEM = "</table>";
const char EMPTY_ROW[] PROGMEM = "<tr><td colspan='3' align='center'>&nbsp;</td></tr>";
const char WEB_PAGE_FOOTER[] PROGMEM = "<br/><br/><a href='/' style='display:inline;'>{t}</a><br/><br/><br/>\
<b><a href='https://air.nettigo.pl/' target='_blank' style='display:inline;background:none;color:black;'>&copy; Nettigo Air Monitor (Koduj dla Polski)</a></b><br/><br/>\
<small>Firmware based on Sensor.Community project by<br/><a href='https://codefor.de/stuttgart/' target='_blank' style='display:inline;background:none;color:black;padding:0'>Open Knowledge Lab Stuttgart a.o. (Code for Germany)</a></small>\
</div></body></html>\r\n";

const char WEB_ROOT_PAGE_CONTENT[] PROGMEM = "<a href='/values'>&#128200; {t}</a><br/>\
<a href='/config'>&#128295; {conf}</a><br/>\
<a href='/status'>&#x1FA7A; {status}</a><br/>\
<a href='/reset'>&#128260; {restart}</a><br/>\
<table style='width:100%;'>\
<tr><td colspan='3'>&#128027; <b>{debug_level}</b></td></tr>\
<tr><td style='width:33%;'><a href='/debug?lvl=0'>{none}</a></td>\
<td style='width:33%;'><a href='/debug?lvl=1'>{error}</a></td>\
<td style='width:33%;'><a href='/debug?lvl=2'>{warning}</a></td>\
</tr><tr>\
<td><a href='/debug?lvl=3'>{min_info}</a></td>\
<td><a href='/debug?lvl=4'>{med_info}</a></td>\
<td><a href='/debug?lvl=5'>{max_info}</a></td>\
</tr>\
</table>\
<br/><a href='/removeConfig' class='s_orange'>&#128293; {conf_delete}</a><br/>\
";

const char WEB_CONFIG_SCRIPT[] PROGMEM = "<script>\
function setSSID(ssid){document.getElementById('wlanssid').value=ssid.innerText||ssid.textContent;document.getElementById('wlanpwd').focus();}\
function load_wifi_list(){var x=new XMLHttpRequest();x.open('GET','/wifi');x.onload = function(){if (x.status === 200) {document.getElementById('wifilist').innerHTML = x.responseText;}};x.send();}\
</script>";

const char WEB_REMOVE_CONFIG_CONTENT[] PROGMEM = "<h3>{t}</h3>\
<table><tr><td><form method='POST' action='/removeConfig'><input type='submit' class='s_red' name='submit' value='&#128293; {b}'/></form></td><td><a href='/'>{c}</a></td></tr></table>\
";

const char WEB_RESET_CONTENT[] PROGMEM = "<h3>{t}</h3>\
<table><tr><td><form method='POST' action'/reset'><input type='submit' class='s_red' name='submit' value='&#128260; {b}'/></form></td><td><a href='/'>{c}</a></td></tr></table>\
";

const char WEB_IOS_REDIRECT[] PROGMEM = "<html><body>Redirecting...\
<script type=\"text/javascript\">\
window.location = \"http://192.168.4.1/config\";\
</script>\
</body></html>";

#endif