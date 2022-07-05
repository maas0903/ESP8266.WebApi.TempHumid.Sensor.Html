#include <stdio.h>
#include <ArduinoJson.h>
#include <Arduino.h>

#include <credentials.h>

//#include <OneWire.h>
#include <Wire.h>
#include <DHT.h>
#include <SPI.h>

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// //For hostname
// //does not work
// extern "C"
// {
// #include <user_interface.h>
// }

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

#define LED_0 0
#define DHTPIN 2
#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321

String statusStr = " nothing ";
char JSONmessageBuffer[600];

DHT dht(DHTPIN, DHTTYPE);

// Define NTP Client to get time
WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 0;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

IPAddress staticIP(192, 168, 63, 123);
IPAddress gateway(192, 168, 63, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 63, 21);
IPAddress dnsGoogle(8, 8, 8, 8);
String hostName = "esp01";

int deviceCount = 1;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

String GetCurrentTime()
{
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    char buff[32];

    sprintf(buff, "%02d-%02d-%02d %02d:%02d:%02d",
            year(epochTime),
            month(epochTime),
            day(epochTime),
            hour(epochTime),
            minute(epochTime),
            second(epochTime));
    String currentTime = buff;
    return currentTime;
}

void BlinkNTimes(int pin, int blinks, unsigned long millies)
{
    digitalWrite(pin, LOW);
    for (int i = 0; i < blinks; i++)
    {
        digitalWrite(pin, HIGH);
        delay(millies);
        digitalWrite(pin, LOW);
        delay(millies);
    }
}

void get_temps()
{
    BlinkNTimes(LED_0, 2, 500);
    StaticJsonBuffer<600> jsonBuffer;
    JsonObject &jsonObj = jsonBuffer.createObject();

    try
    {
        jsonObj["UtcTime"] = GetCurrentTime();
        jsonObj["DeviceCount"] = deviceCount;
        jsonObj["Hostname"] = hostName;
        jsonObj["IpAddress"] = WiFi.localIP().toString();
        jsonObj["Mac Address"] = WiFi.macAddress();

        if (deviceCount == 0)
        {
            Serial.print("No Content");
            //http_rest_server.send(204);
            //CORS
            http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");
            String sHostName(WiFi.hostname());

            http_rest_server.send(200, "text/html", "No devices found on " + sHostName + " (" + WiFi.macAddress() + ")");
        }
        else
        {
            jsonObj["Gpio"] = 2;

            float h = dht.readHumidity();
            //celcius
            float t = dht.readTemperature();

            String mess;
            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t))
            {
                mess = "Failed to read from DHT sensor!";
                Serial.println(mess);
                return;
            }

            // Compute heat index in Celcius
            float hic = dht.computeHeatIndex(t, h, false);

            mess = "Humidity: " + (String)h + " % ** " 
                + "Temperature: " + (String)t + " °C ** "
                + "Heat index: " + (String)hic + " °C ";
            jsonObj["DHT22"] = mess;
        }
    }
    catch (const std::exception &e)
    {
        // String exception = e.what();
        // jsonObj["Exception"] = exception.substring(0, 99);
        jsonObj["Exception"] = " ";
        //std::cerr << e.what() << '\n';
    }

    jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    //http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");

    //http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

String SendHTML()
{
  get_temps();
  String ptr = "<!DOCTYPE html>";
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>ESP8266 with DS18B20 Temperature Monitor and Relay Switch</title>";
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";

  ptr += "<style>";
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr += "body{margin-top: 50px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr += ".temperature{font-weight: 300;font-size: 50px;padding-right: 15px;}";
  ptr += ".Sensor1 .Settings1 .temperature{color: #3B97D3;}";
  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -5px;top: 15px;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:82px}";
  ptr += "</style>";

  //AJAX to auto refresh the body
  ptr += "<script>\n";
  ptr += "setInterval(loadDoc,1000);\n";
  ptr += "function loadDoc() {\n";
  ptr += "var xhttp = new XMLHttpRequest();\n";
  ptr += "xhttp.onreadystatechange = function() {\n";
  ptr += "if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "document.body.innerHTML =this.responseText}\n";
  ptr += "};\n";
  ptr += "xhttp.open(\"GET\", \"/\", true);\n";
  ptr += "xhttp.send();\n";
  ptr += "}\n";
  ptr += "</script>\n";

  ptr += "</head>";

  ptr += "<body>";
  ptr += "<h1>ESP8266 with DHT22 Temperature and Humidity Monitor</h1>";
  ptr += "<div class='container'>";

  ptr += "<div class='data Sensor1'>";
  ptr += "<div class='side-by-side text'>Sensor 1</div>";
  ptr += "<div class='side-by-side data'>";
  ptr += JSONmessageBuffer;
ptr += "<div style=\"margin-left:.5em;\">";
ptr += "The computation of the heat index is a refinement of a result";
ptr += "obtained by multiple regression analysis carried out by Lans P. Rothfusz";
ptr += "and described in a 1990 National Weather Service (NWS) Technical Attachment";
ptr += "(SR 90-23).&nbsp; The regression equation of Rothfusz is";
ptr += "<blockquote>";
ptr += "<b>";
ptr += "HI = -42.379 + 2.04901523*T + 10.14333127*RH - .22475541*T*RH - .00683783*T*T";
ptr += "- .05481717*RH*RH + .00122874*T*T*RH + .00085282*T*RH*RH - .00000199*T*T*RH*RH</b>";
ptr += "</blockquote>";
ptr += "where <b>T</b> is temperature in degrees F and <b>RH</b> is relative humidity";
ptr += "in percent.&nbsp; <b>HI</b> is the heat index expressed as an apparent";
ptr += "temperature in degrees F.&nbsp; If the <b>RH</b> is less than 13% and the";
ptr += "temperature is between 80 and 112 degrees F, then the following adjustment";
ptr += "is subtracted from <b>HI</b>:";
ptr += "<blockquote>";
ptr += "<b>";
ptr += "ADJUSTMENT = [(13-RH)/4]*SQRT{[17-ABS(T-95.)]/17}</b>";
ptr += "</blockquote>";
ptr += "where <b>ABS</b> and <b>SQRT</b> are the absolute value and square root";
ptr += "functions, respectively.&nbsp; On the other hand, if the <b>RH </b>is greater";
ptr += "than 85% and the temperature is between 80 and 87 degrees F, then the following";
ptr += "adjustment is added to <b>HI</b>:";
ptr += "<blockquote>";
ptr += "<b>";
ptr += "ADJUSTMENT = [(RH-85)/10] * [(87-T)/5]</b>";
ptr += "</blockquote>";
ptr += "The Rothfusz regression is not appropriate when conditions of temperature and";
ptr += "humidity warrant a heat index value below about 80 degrees F.  In those cases,";
ptr += "a simpler formula is applied to calculate values consistent with";
ptr += "Steadman's results:";
ptr += "<blockquote>";
ptr += "<b>";
ptr += "HI = 0.5 * {T + 61.0 + [(T-68.0)*1.2] + (RH*0.094)} ";
ptr += "</b>";
ptr += "</blockquote>";
ptr += "In practice, the simple formula is computed first and the result averaged with";
ptr += "the temperature.  If this heat index value is 80 degrees F or higher,  the full";
ptr += "regression equation along with any adjustment as described above is applied.";
ptr += "<p>";
ptr += "The Rothfusz regression is not valid for extreme temperature and";
ptr += "relative humidity conditions beyond the range of data considered by Steadman.";
ptr += "</p></div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}

int init_wifi()
{
    int retries = 0;

    Serial.println("Connecting to WiFi");

    WiFi.config(staticIP, gateway, subnet, dns, dnsGoogle);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostName);
    WiFi.begin(ssid, password);

    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY))
    {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    Serial.println();
    //Serial.println("hostName = " + WiFi.hostname);
    BlinkNTimes(LED_0, 3, 500);
    return WiFi.status();
}

// String GetAddressToString(DeviceAddress deviceAddress)
// {
//     String str = "";
//     for (uint8_t i = 0; i < 8; i++)
//     {
//         if (deviceAddress[i] < 16)
//             str += String(0, HEX);
//         str += String(deviceAddress[i], HEX);
//     }
//     return str;
// }

void config_rest_server_routing()
{
    http_rest_server.on("/", HTTP_GET, []() {
    http_rest_server.send(200, "text/html", SendHTML());
  });

  
}

void setup(void)
{
    Serial.begin(115200);

    pinMode(DHTPIN, INPUT);
    dht.begin();

    if (init_wifi() == WL_CONNECTED)
    {
        Serial.print("Connected to ");
        Serial.print(ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.print("Error connecting to: ");
        Serial.println(ssid);
    }

    timeClient.begin();

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}

void loop(void)
{
    http_rest_server.handleClient();
}