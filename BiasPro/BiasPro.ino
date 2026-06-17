#include "ApplicationController.h"

ApplicationController app;

void setup() {
  Serial.begin(9600);
  Serial.println(F("BiasPro Booting..."));
  app.begin();
}

void loop() {
  app.tick();
}
