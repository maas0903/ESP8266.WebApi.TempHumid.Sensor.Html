# esp8266 thermometer and humidity sensor 
1-wire esp8266-01 thermometer and humidity web page with DHT22 chip.
Reads first found Device ID, must be on GPIO2.
Example:

http://192.168.xx.xx/

gives the heb page.

For connection to your WiFi, create your own credentials.h and put it in \includes:
```
const char* ssid = "Enter SSID here";
const char* password = "Enter Password here";
```

