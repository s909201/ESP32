// Import required libraries
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <AsyncElegantOTA.h>

#include <ESPmDNS.h>
#include <Update.h>

#include "time.h"
#include "sntp.h"

// ESP32 Access credential
const char *myUsername = "admin";
const char *myPass = "admin";

// mDNS
const char *host = "esp32";

// Replace with your network credentials
const char *ssid = "elytone";
const char *password = "30965487";
// const char *ssid = "HH71V1_0801_2.4G";
// const char *password = "thisismyhouseandhome";

const char *PARAM_INPUT_1 = "output";
const char *PARAM_INPUT_2 = "state";

#define I2C_SDA 21
#define I2C_SCL 22

#define I2C_SDA2 32
#define I2C_SCL2 33

const uint8_t SPRINTF_BUFFER_SIZE{32}; ///< Buffer size for sprintf()

// for PICO D4
// byte PinArray[] = {21, 22, 19, 23, 18, 5, 10, 9, 35, 37, 25, 26, 27, 14, 13, 15, 2, 4, 0};
byte PinArray[] = {21, 22, 19, 23, 18, 5, 10, 9, 25, 26, 27, 14, 13, 15, 2, 4, 0}; // can't use pin 35, 37

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 1200; // 0.6 sec
void (*resetFunc)(void) = 0;  // declare reset fuction at address 0

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// ADC
const int ADCPin = 37;
int analogValue, analogVolts;
uint32_t TASK_ADC_TICK;

// LED PWM
const int analogOutPin = 4; // Analog output pin that the LED is attached to
String pwmSliderValue = "0";
int pwmOutputValue;

const char index_html[] PROGMEM = R"rawliteral(
<html>
<head>
    <title>Morgan Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        html {
            font-family: Arial;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
        }

        h1 {
            color: #0F3376;
            padding: 1vh;
        }

        h3 {
            color: brown;
        }

        p {
            font-size: 15px;
        }

        a {
            font-size: 15px;
        }

        .button {
            display: inline-block;
            background-color: #008CBA;
            border: none;
            border-radius: 4px;
            color: white;
            padding: 12px 24px;
            text-decoration: none;
            font-size: 25px;
            margin: 2px;
            cursor: pointer;
        }

        .button2 {
            background-color: #f44336;
        }

        .units {
            font-size: 5px;
        }

        .sensor-labels {
            font-size: 1rem;
            vertical-align: middle;
            padding-bottom: 15px;
        }

        .slidecontainer {
            width: 100%%;
        }

        .slider {
            -webkit-appearance: none;
            width: 100%%;
            height: 15px;
            border-radius: 5px;
            background: #d3d3d3;
            outline: none;
            opacity: 0.7;
            -webkit-transition: .2s;
            transition: opacity .2s;
        }

        .slider:hover {
            opacity: 1;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 25px;
            height: 25px;
            border-radius: 50%%;
            background: #04AA6D;
            cursor: pointer;
        }

        .slider::-moz-range-thumb {
            width: 25px;
            height: 25px;
            border-radius: 50%%;
            background: #04AA6D;
            cursor: pointer;
        }

        .btn3 {
            background: #a3a6ad;
            background-image: -webkit-linear-gradient(top, #a3a6ad, #262626);
            background-image: -moz-linear-gradient(top, #a3a6ad, #262626);
            background-image: -ms-linear-gradient(top, #a3a6ad, #262626);
            background-image: -o-linear-gradient(top, #a3a6ad, #262626);
            background-image: linear-gradient(to bottom, #a3a6ad, #262626);
            -webkit-border-radius: 10;
            -moz-border-radius: 10;
            border-radius: 10px;
            -webkit-box-shadow: 3px 3px 3px #666666;
            -moz-box-shadow: 3px 3px 3px #666666;
            box-shadow: 3px 3px 3px #666666;
            font-family: Arial;
            color: #ffffff;
            font-size: 16px;
            padding: 20px 10px 20px 10px;
            border: solid #565757 3px;
            text-decoration: none;
            margin: 20px;
            width: 120px;
        }

        .btn3:hover {
            text-decoration: none;
        }

        #clock {
            font-size: 16px;
            color: red;
        }
    </style>
</head>

<body>
    <h1>ESP32 Web Server</h1>
    <p><span id="clock">%CLOCK%</span></p>
    <p>GPIO state (Pin 2): <strong>%STATE%</strong></p>
    <p><a href="/on"><button class="button">ON</button></a><a href="/off"><button
                class="button button2">OFF</button></a></p>
    <p><span class="sensor-labels">Temperature= </span><span id="temperature">%TEMPERATURE%</span>&deg;
        C </p>
    <p><span class="sensor-labels">ADC 37=</span><span id="adc37">%ADC37%</span></p>
    <p><span class="sensor-labels">ADC voltge=</span><span id="adc37v">%ADC37v%</span>mV </p>
    <h3>Firmware Upgrade</h3>
    <p><a href="/update">Update Firmware</a></p>
    <h3>mDNS Test</h3>
    <p><a href="http://esp32.local" target="_blank">http://esp32.local</a>
    </p>
    <h3>Server IP Address</h3>
    <p><span id="IPAddr">%IPAddr%</span></p>
    <h3>LED Dimming (Pin 4)</h3>
    <div class="slidecontainer"><input type="range" onchange="updateSliderPWM(this)" min="0" max="100" value="0"
            class="slider" id="pwmSlider">
        <p>Value: <span id="textSliderValue"></span></p>
    </div>
    <script>
        var slider1 = document.getElementById("pwmSlider");
        var output1 = document.getElementById("textSliderValue");
        output1.innerHTML = slider1.value;
        slider1.oninput = function () {
            output1.innerHTML = this.value;
        }
        function updateSliderPWM(element) {
            var pwmSliderValue = document.getElementById("pwmSlider").value;
            var httpRequest = new XMLHttpRequest();
            httpRequest.open("Get", "/slider?value=" + pwmSliderValue, true);
            httpRequest.send();
        }
    </script>
    <h3>EQ Mode</h3><button class="btn3">Flat</button><button class="btn3">CINEMA</button><button
        class="btn3">MUSIC</button><button class="btn3">NIGHT</button><button class="btn3">USER 1</button><button
        class="btn3">USER 2</button>
    <h3>RCA Input Trim</h3>
    <div class="slidecontainer"><input type="range" min="1" max="100" value="50" class="slider" id="myRange2">
        <p>Value: <span id="demo2"></span></p>
    </div>
    <script>var slider2 = document.getElementById("myRange2");
        var output2 = document.getElementById("demo2");
        output2.innerHTML = slider2.value;

        slider2.oninput = function () {
            output2.innerHTML = this.value;
        }

    </script>
    <h3>Balance Input Trim</h3>
    <div class="slidecontainer"><input type="range" min="1" max="100" value="50" class="slider" id="myRange3">
        <p>Value: <span id="demo3"></span></p>
    </div>
    <script>var slider3 = document.getElementById("myRange3");
        var output3 = document.getElementById("demo3");
        output3.innerHTML = slider3.value;

        slider3.oninput = function () {
            output3.innerHTML = this.value;
        }

    </script>
    <h3>High Level Trim</h3>
    <div class="slidecontainer"><input type="range" min="1" max="100" value="50" class="slider" id="myRange4">
        <p>Value: <span id="demo4"></span></p>
    </div>
    <script>var slider4 = document.getElementById("myRange4");
        var output4 = document.getElementById("demo4");
        output4.innerHTML = slider4.value;

        slider4.oninput = function () {
            output4.innerHTML = this.value;
        }

    </script>
    <h3>Wireless Input Trim</h3>
    <div class="slidecontainer"><input type="range" min="1" max="100" value="50" class="slider" id="myRange5">
        <p>Value: <span id="demo5"></span></p>
    </div>
    <script>var slider5 = document.getElementById("myRange5");
        var output5 = document.getElementById("demo5");
        output5.innerHTML = slider5.value;

        slider5.oninput = function () {
            output5.innerHTML = this.value;
        }

    </script>
</body>
<script>setInterval(function () {
        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("temperature").innerHTML = this.responseText;
            }
        }
            ;
        xhttp.open("GET", "/temperature", true);
        xhttp.send();
    }
        , 10000);

    setInterval(function () {
        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("clock").innerHTML = this.responseText;
            }
        }
            ;
        xhttp.open("GET", "/clock", true);
        xhttp.send();
    }
        , 1000);

    setInterval(function () {
        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("adc37").innerHTML = this.responseText;
            }
        }
            ;
        xhttp.open("GET", "/adc37", true);
        xhttp.send();
    }
        , 500);

    setInterval(function () {
        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                document.getElementById("adc37v").innerHTML = this.responseText;
            }
        }
            ;
        xhttp.open("GET", "/adc37v", true);
        xhttp.send();
    }
        , 1000);

</script>

</html>
)rawliteral";

// -----------------------------------------------------------
// TempClock
// millis(): Get System Tick
struct _tm
{
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint32_t sec1;
} t;
// -----------------------------------------------------------
// Clock
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

// const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)
const char *time_zone = "CST-8";

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // Serial.println(String(timeinfo.tm_year+1900) + "-" + String(timeinfo.tm_mon+1) + "-" + String(timeinfo.tm_mday) + "," + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min)+ ":" + String(timeinfo.tm_sec));
  // get Time information
  t.year = timeinfo.tm_year + 1900;
  t.month = timeinfo.tm_mon + 1;
  t.day = timeinfo.tm_mday;
  t.hour = timeinfo.tm_hour;
  t.min = timeinfo.tm_min;
  t.sec = timeinfo.tm_sec;
  t.sec1 = millis();
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void initClock()
{
  // set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);
  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagicaly.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  configTzTime(time_zone, ntpServer1, ntpServer2);
}
// -----------------------------------------------------------

// -----------------------------------------------------------
// Initialize SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}
// -----------------------------------------------------------
// Initialize WiFi
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  // WiFi Connect
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  currentTime = millis();
  previousTime = currentTime;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
    if (millis() - previousTime >= timeoutTime)
    {
      resetFunc(); // call reset
    }
  }
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
}
// -----------------------------------------------------------

// -----------------------------------------------------------
void initMDNS()
{
  if (!MDNS.begin(host))
  { // http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
}
// -----------------------------------------------------------
String outputState(int output)
{
  if (digitalRead(output))
  {
    return "checked";
  }
  else
  {
    return "";
  }
}
// -----------------------------------------------------------
void InitGPIO(byte i)
{
  pinMode(i, OUTPUT);
  digitalWrite(i, LOW);
}
// -----------------------------------------------------------

// -----------------------------------------------------------
String getTemperature()
{
  float temperature = 24.5; // fake
  // Serial.println(temperature);
  return String(temperature);
}
// -----------------------------------------------------------
String getClock()
{
  // Use sprintf() to pretty print the date/time with leading zeros
  char output_buffer[SPRINTF_BUFFER_SIZE]; ///< Temporary buffer for sprintf()
  sprintf(output_buffer, "%04d-%02d-%02d %02d:%02d:%02d", (t.year + 1900), (t.month + 1), (t.day), t.hour, t.min, t.sec);
  return String(output_buffer);
}
// -----------------------------------------------------------
String getADC37()
{
  return String(analogValue);
}

String getADC37v()
{
  return String(analogVolts);
}

String getIPAddr()
{
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  return ipStr;
}
// -----------------------------------------------------------
// Replaces placeholder with button section in your web page
String processor(const String &var)
{
  // Serial.println(var);
  if (var == "STATE")
  {
    if (digitalRead(ledPin))
    {
      ledState = "ON";
    }
    else
    {
      ledState = "OFF";
    }
    Serial.println(ledState);
    return ledState;
  }
  else if (var == "TEMPERATURE")
  {
    return getTemperature();
  }
  else if (var == "CLOCK")
  {
    return getClock();
  }
  else if (var == "ADC37")
  {
    return getADC37();
  }
  else if (var == "ADC37v")
  {
    return getADC37v();
  }
  else if (var == "IPAddr")
  {
    return getIPAddr();
  }
}
// -----------------------------------------------------------

// -----------------------------------------------------------
void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);
  InitGPIO(ledPin);
  InitGPIO(analogOutPin);

  // ADC: set the resolution to 12 bits (0-4096)
  analogReadResolution(12);

  // Init SPIFFS
  initSPIFFS();

  // Init WiFi
  initWiFi();

  // Init WebSocket
  // initWebSocket();

  // Init MDNS
  initMDNS();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Route to set GPIO to LOW
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  digitalWrite(ledPin, HIGH);
  // request->send(SPIFFS, "/index.html", String(),false, processor); 
  request->send_P(200, "text/html", index_html, processor); });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
  digitalWrite(ledPin, LOW);
  // request->send(SPIFFS, "/index.html", String(),false, processor); 
  request->send_P(200, "text/html", index_html, processor); });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getTemperature().c_str()); });

  server.on("/clock", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getClock().c_str()); });

  server.on("/adc37", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getADC37().c_str()); });

  server.on("/adc37v", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getADC37v().c_str()); });

  server.on("/IPAddr", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", getIPAddr().c_str()); });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam("value")) {
      inputMessage = request->getParam("value")->value();
      pwmSliderValue = inputMessage;
      // ledcWrite(pwmChannel, pwmSliderValue.toInt());
      // map it to the range of the analog out:
      pwmOutputValue = map(pwmSliderValue.toInt(), 0, 100, 0, 255);
      analogWrite(analogOutPin, pwmOutputValue);
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK"); });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server); // without password
  // AsyncElegantOTA.begin(&server, myUsername, myPass); // Start ElegantOTA
  Serial.println("FOTA server ready!");

  // Start server
  server.begin();

  // Add service to MDNS-SD, it's necessary.
  MDNS.addService("http", "tcp", 80);

  // Init Clock
  initClock();
}
// -----------------------------------------------------------
void SysTick()
{
  if ((millis() - t.sec1) >= 1000) // 1 sec
  {
    t.sec1 = millis(); // reset

    t.sec++;
    if (t.sec >= 60)
    {
      t.sec = 0;
      t.min++;
      if (t.min >= 60)
      {
        t.min = 0;
        t.hour++;
        if (t.hour >= 24)
        {
          t.hour = 0;
        }
      }
    }
  }
}
// -----------------------------------------------------------
void TASK_ADC()
{
  if((millis() - TASK_ADC_TICK)>=250)
  {
    // read the analog / millivolts value for pin 2:
    analogValue = analogRead(ADCPin);
    analogVolts = analogReadMilliVolts(ADCPin);
    // print out the values you read:
    // Serial.printf("ADC analog value = %d\n",analogValue);
    // Serial.printf("ADC millivolts value = %d\n",analogVolts);
  }
}
// -----------------------------------------------------------
void loop()
{
  SysTick();
  TASK_ADC();
  // printLocalTime();
  // delay(250);
}
// -----------------------------------------------------------

// -----------------------------------------------------------
// -----------------------------------------------------------
