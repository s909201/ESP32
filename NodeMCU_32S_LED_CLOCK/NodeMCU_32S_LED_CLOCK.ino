// Project: NodeMCU_32S_LED_CLOCK
// Created by Morgan, 2023.6.17
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "sntp.h"

// for WiFi
const char *ssid = "elytone";
const char *password = "30965487";

// for Clock
// millis(): Get System Tick
struct _tm
{
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint32_t sec1;
} t;

const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
// const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)
const char *time_zone = "CST-8";
unsigned long times_up = 0;
unsigned char TASK_LED_DISPLAY_STEP = 0;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 1200; // 0.6 sec
void (*resetFunc)(void) = 0;   // declare reset fuction at address 0
void printLocalTime();

// for GPIO
#define LEDDelay 3 // Hold Time

#define D1 12 // P6A, High Enable
#define D2 14 // P5A
#define D3 27 // P4A
#define D4 26 // P3A
// CHx_on(1), D3 on
// CHx_on(2), D4 on
// CHx_on(3), D1 on
// CHx_on(4), D2 on

#define LED1 23 // P17, Low Enable
#define LED2 22 // P18
#define LED3 21 // P24
#define LED4 19 // P23
#define LED5 18 // P22
#define LED6 5  // P21
#define LED7 17 // P20
#define LED8 16 // P19

void initWiFi();
void initClock();
void TASK_LED_FRAME_BUFFER();
void TASK_LED_DISPLAY();
void No2LED2(uint8_t ch, uint8_t val);
void LED_CLEAN();

int8_t fZeroH = 0, fZeroM = 0;
int8_t fLED[8]; // skip 0
uint8_t yLED[8] = {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8};
uint8_t xLED[4] = {D1, D2, D3, D4};
uint8_t LEDData[4][9]; // skip 0
uint8_t iLED = 0, jLED = 0;
uint8_t bNO[10] = {0b01111110, 0b00110000, 0b01101101, 0b01111001, 0b00110011, 0b01011011, 0b00011111, 0b01110010, 0b01111111, 0b01110011};
uint8_t val = 0, tmp = 0;

// LEDData[0][8] = 1; // PM
// LEDData[1][8] = 1; // AL
// LEDData[2][8] = 1; // SEC
// LEDData[3][8] = 1; // TEMP
// --------------------------------------------------------------
void setup()
{
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT);
  pinMode(LED6, OUTPUT);
  pinMode(LED7, OUTPUT);
  pinMode(LED8, OUTPUT);
  LED_CLEAN();

  // init UART
  Serial.begin(115200); // GPIO 1(Tx), 3(Rx)

  // init WiFi
  initWiFi();

  // Init Clock and Show Time
  initClock();

  // Sync Clock Time
  // printLocalTime();
}
// --------------------------------------------------------------
void loop()
{
  SysTick();
  TASK_LED_FRAME_BUFFER();
  TASK_LED_DISPLAY();
}
// --------------------------------------------------------------
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
  t.year = timeinfo.tm_year;
  t.month = timeinfo.tm_mon;
  t.day = timeinfo.tm_mday;
  t.hour = timeinfo.tm_hour;
  t.min = timeinfo.tm_min;
  t.sec = timeinfo.tm_sec;
  t.sec1 = millis();
}
// --------------------------------------------------------------
// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}
// --------------------------------------------------------------
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
// --------------------------------------------------------------
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
// --------------------------------------------------------------

// --------------------------------------------------------------
void TASK_LED_FRAME_BUFFER()
{
  LED_CLEAN();
  // LED Frame Buffer
  for (jLED = 0; jLED < 4; jLED++)
  {
    if (jLED == 0)
    {
      if (fZeroH)
      {
        digitalWrite(D1, LOW);
        digitalWrite(D2, LOW);
        digitalWrite(D3, LOW);
        digitalWrite(D4, LOW);
      }
      else
      {
        digitalWrite(D1, LOW);
        digitalWrite(D2, LOW);
        digitalWrite(D3, HIGH);
        digitalWrite(D4, LOW);
      }
    }
    else if (jLED == 1)
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, LOW);
      digitalWrite(D3, LOW);
      digitalWrite(D4, HIGH);
    }
    else if (jLED == 2)
    {
      if (fZeroM)
      {
        digitalWrite(D1, LOW);
        digitalWrite(D2, LOW);
        digitalWrite(D3, LOW);
        digitalWrite(D4, LOW);
      }
      else
      {
        digitalWrite(D1, HIGH);
        digitalWrite(D2, LOW);
        digitalWrite(D3, LOW);
        digitalWrite(D4, LOW);
      }
    }
    else
    {
      digitalWrite(D1, LOW);
      digitalWrite(D2, HIGH);
      digitalWrite(D3, LOW);
      digitalWrite(D4, LOW);
    }

    for (iLED = 0; iLED < 8; iLED++) // 8 LED, OK
    {
      if (LEDData[jLED][iLED + 1]) // skip [][0]
      {
        digitalWrite(yLED[iLED], LOW);
      }
      else
      {
        digitalWrite(yLED[iLED], HIGH);
      }
    }
    delay(LEDDelay);
  }
}
// --------------------------------------------------------------
// Table
// X [1234567]
// 0 [1111110]
// 1 [0110000]
// 2 [1101101]
// 3 [1111001]
// 4 [0110011]
// 5 [1011011]
// 6 [0011111]
// 7 [1110010]
// 8 [1111111]
// 9 [1110011]
void No2LED2(uint8_t ch, uint8_t val)
{
  LEDData[ch][1] = (bNO[val] & 0b01000000) >> 6;
  LEDData[ch][2] = (bNO[val] & 0b00100000) >> 5;
  LEDData[ch][3] = (bNO[val] & 0b00010000) >> 4;
  LEDData[ch][4] = (bNO[val] & 0b00001000) >> 3;
  LEDData[ch][5] = (bNO[val] & 0b00000100) >> 2;
  LEDData[ch][6] = (bNO[val] & 0b00000010) >> 1;
  LEDData[ch][7] = (bNO[val] & 0b00000001) >> 0;
  // LEDData[ch][8] = (bNO[val] & 0b00000001) >> 0; // symbol
}
// --------------------------------------------------------------
void TASK_LED_DISPLAY()
{
  // 0.5s blink for second
  switch (TASK_LED_DISPLAY_STEP)
  {
  case 0:
    No2LED2(0, 8); // ch, val
    No2LED2(1, 8); // ch, val
    No2LED2(2, 8); // ch, val
    No2LED2(3, 8); // ch, val

    TASK_LED_DISPLAY_STEP = 1;
    times_up = millis() + 6000; // wait WiFi connection
    break;

  case 1:
    if (millis() > times_up)
    {
      // No2LED2(0, (uint8_t)(t.hour / 10));
      tmp = (uint8_t)(t.hour / 10);
      if (tmp == 0)
      {
        fZeroH = 1;
      }
      else
      {
        fZeroH = 0;
        No2LED2(0, tmp);
      }

      No2LED2(1, (uint8_t)(t.hour % 10));

      // No2LED2(2, (uint8_t)(t.min / 10));
      tmp = (uint8_t)(t.min / 10);
      if (tmp == 0)
      {
        fZeroM = 1;
      }
      else
      {
        fZeroM = 0;
        No2LED2(2, tmp);
      }

      No2LED2(3, (uint8_t)(t.min % 10));

      // LEDData[0][8] = 1; // PM
      // LEDData[1][8] = 1; // AL
      LEDData[2][8] = 1; // SEC
      // LEDData[3][8] = 1; // TEMP

      TASK_LED_DISPLAY_STEP = 2;
      times_up = millis() + 500;
    }
    break;

  case 2:
    if (millis() > times_up)
    {
      LEDData[2][8] = 0;
      TASK_LED_DISPLAY_STEP = 1;
      times_up = millis() + 500;
    }
    break;
  }
}
// --------------------------------------------------------------
void LED_CLEAN()
{
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D3, LOW);
  digitalWrite(D4, LOW);

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  digitalWrite(LED4, HIGH);
  digitalWrite(LED5, HIGH);
  digitalWrite(LED6, HIGH);
  digitalWrite(LED7, HIGH);
  digitalWrite(LED8, HIGH);
}
// --------------------------------------------------------------

// --------------------------------------------------------------
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
// --------------------------------------------------------------

