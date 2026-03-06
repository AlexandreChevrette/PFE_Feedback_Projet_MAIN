#include <Arduino.h>
#include "ADC.h"
volatile bool dataReady = false;

void IRAM_ATTR drdyISR()
{
    dataReady = true;
}

void setup() {
    attachInterrupt(DRDY_PIN, drdyISR, FALLING);
}


void loop() {

}

