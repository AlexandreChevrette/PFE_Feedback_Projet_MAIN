#include <Arduino.h>
#include "ADC.h"
#include "MotorControl.h"
#include <vector>
volatile bool dataReady = false;
volatile bool updateMotors = false;

// a voir, ce semble bizarre cette facon de faire
volatile void (*readDataCallbackPtr)() = nullptr; // global function pointer
volatile void (*readDataCallbackPtr)() = nullptr; // global function pointer
volatile void (*readDataCallbackPtr)() = nullptr; // global function pointer
volatile void (*readDataCallbackPtr)() = nullptr; // global function pointer


std::array<float, numberOfChannels> currentData;
ADC adc;
MotorControl motorControl;

void IRAM_ATTR drdyISR()
{
    dataReady = true;
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(DRDY_PIN), drdyISR, FALLING);
    adc.setup();
    motorControl.setup();
}

// spi communication outside of ISR
void loop() {
    if (dataReady){
        adc.readData();
        dataReady = false;
    }
    if (updateMotors){

    }
}

