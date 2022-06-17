#include "SPIFFS.h"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
