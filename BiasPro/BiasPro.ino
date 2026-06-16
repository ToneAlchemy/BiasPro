#include "ApplicationController.h"

ApplicationController app;

void setup() {
  app.begin();
}

void loop() {
  app.tick();
}
