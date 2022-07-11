#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 8

CRGB leds[NUM_LEDS];

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  Serial.println("setup, done..");
}

uint8_t COLOR_R, COLOR_G, COLOR_B, i;

void loop()
{
  // put your main code here, to run repeatedly:
  Serial.println("start");

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255, 0, 0); // R
    FastLED.show();
  }
  delay(1000);

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 255, 0); // G
    FastLED.show();
  }
  delay(1000);

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 255); // B
    FastLED.show();
  }
  delay(1000);

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255, 255, 255); // White
    FastLED.show();
  }
  delay(1000);
}
