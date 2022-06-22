#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // 0x27 is the LCD i2c address

// ADC
const int ADCPin = 37;
int analogValue, analogVolts;

void setup()
{
  Wire.begin();
  Serial.begin(115200);
  while (!Serial)
    ;
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("  I2C LCD with ");
  lcd.setCursor(0,1);
  lcd.print("  ESP32 DevKit ");
}

void loop()
{
  // read the analog / millivolts value for pin 37:
  analogValue = analogRead(ADCPin); // 直接讀暫存器內容
  analogVolts = analogReadMilliVolts(ADCPin); // 直接讀取ms的數值
  
  // print out the values you read:
  Serial.printf("ADC analog value = %d\n",analogValue);
  Serial.printf("ADC millivolts value = %d\n",analogVolts);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("AD value = " + String(analogValue));
  lcd.setCursor(0,1);
  lcd.print("millivolt = " + String(analogVolts));
  delay(1000);
  
}
