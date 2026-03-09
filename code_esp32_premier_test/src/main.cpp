#include <Arduino.h>
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include <array>

volatile bool dataReady = false;

ADC adc;
MotorControl motorControl;
FeedbackControl feedbackControl;
WheatstoneBridge bridge1(18000, 10000, 82000, 3.3);
WheatstoneBridge bridge2(18000, 10000, 82000, 3.3);
WheatstoneBridge bridge3(18000, 10000, 82000, 3.3);
PressureSensor sensor1(0.0462,200000);
PressureSensor sensor2(0.0462,200000);
PressureSensor sensor3(0.0462,200000);

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
        const std::array<float, numberOfChannels>& adcValues = adc.readData();
        dataReady = false;
        
        const std::array<float,numberOfChannels> currentRopeTension = {
            sensor1.convertRtoRopeTension(bridge1.convertDeltaVtoR(adcValues[0])),
            sensor2.convertRtoRopeTension(bridge2.convertDeltaVtoR(adcValues[1])),
            sensor3.convertRtoRopeTension(bridge3.convertDeltaVtoR(adcValues[2]))
        }; 
        feedbackControl.updateLoop(motorControl, currentRopeTension);
    }
}

