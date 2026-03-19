#include "Arduino.h"
#include "ADC.h"
#include "SPI.h"
#include "array"
#include "WiFi.h"
#include "MotorControl.h"
SPIClass spiADC(FSPI);
ADC adc(&spiADC);
MotorControl motorControl;


std::array<float, numberOfChannels> adcValues;

void setup(){

    // à vérifier
    WiFi.mode(WIFI_OFF);
    btStop();


    Serial.begin(115200);
    adc.setup();
    motorControl.setup();
    
    // adc.readID();
}

void loop(){
    motorControl.setForward(1);
    motorControl.setPWM(1, 200);
    delay(2000);
    motorControl.setReverse(1);
    motorControl.setPWM(1, 200);
    adcValues = adc.readData();
    Serial.print("CH0:");
    Serial.println(adcValues[0], 8);
    Serial.print("CH1:");
    Serial.println(adcValues[1], 8);
    Serial.print("CH2:");
    Serial.println(adcValues[2], 8);
    Serial.print("CH3:");
    Serial.println(adcValues[3], 8);
    delay(2000);
    // spiADC.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    // spiADC.transfer(0xAA);
    // spiADC.endTransaction();
    // while(digitalRead(DRDY_PIN)==HIGH);
    
    // Serial.println(digitalRead(DRDY_PIN));
    
}

