#include <Arduino.h>
#include "ADC.h"
volatile bool dataReady = false;

void IRAM_ATTR drdyISR()
{
    dataReady = true;
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(DRDY_PIN), drdyISR, FALLING);
    ADC adc;
}


void loop() {
    
}

