#include <Arduino.h>
#include "Application.h"

MainApp mainApp;

void setup() {
    mainApp.setup();
}

void loop(){
    mainApp.run();
}

