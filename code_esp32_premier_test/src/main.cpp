#include <Arduino.h>
#include "Application.h"
#include "Tests.h"


// MainApp mainApp;
// MotorTest motorTest;
AdcTest adcTest;
// PidTest pidTest;

void setup() {
    adcTest.setup();
}

void loop() {
    adcTest.run();
}
