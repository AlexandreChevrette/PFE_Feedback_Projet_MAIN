#include <Arduino.h>
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include <vector>
volatile bool dataReady = false;
volatile bool updateMotors = false;

std::array<float, numberOfChannels> currentData;
ADC adc;
MotorControl motorControl;
FeedbackControl feedbackControl;

void IRAM_ATTR drdyISR()
{
    dataReady = true;
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(DRDY_PIN), drdyISR, FALLING);
    adc.setup();
    motorControl.setup();
    feedbackControl.setup();
}

// spi communication outside of ISR
void loop() {
    if (dataReady){
        adc.readData();
        dataReady = false;
        
    }
}

