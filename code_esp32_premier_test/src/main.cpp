// #include "Arduino.h"
// #include "ADC.h"
// #include "SPI.h"
// #include "array"
// #include "WiFi.h"
// #include "MotorControl.h"
// SPIClass spiADC(FSPI);
// ADC adc(&spiADC);
// MotorControl motorControl;


// std::array<float, numberOfChannels> adcValues;

// void setup(){
//     Serial.begin(115200);
//     // à vérifier
//     WiFi.mode(WIFI_OFF);
//     btStop();

// }

// void loop(){
//     motorControl.setForward(1);
//     motorControl.setPWM(1, 200);
//     delay(2000);
//     motorControl.setReverse(1);
//     motorControl.setPWM(1, 200);
//     adcValues = adc.readData();
//     Serial.print("CH0:");
//     Serial.println(adcValues[0], 8);
//     Serial.print("CH1:");
//     Serial.println(adcValues[1], 8);
//     Serial.print("CH2:");
//     Serial.println(adcValues[2], 8);
//     Serial.print("CH3:");
//     Serial.println(adcValues[3], 8);
//     delay(2000);
//     // spiADC.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
//     // spiADC.transfer(0xAA);
//     // spiADC.endTransaction();
//     // while(digitalRead(DRDY_PIN)==HIGH);
    
//     // Serial.println(digitalRead(DRDY_PIN));
    
// }




#include "Arduino.h"
#include "ADC.h"
#include "SPI.h"
#include "array"
#include "WiFi.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include "Application.h"
SPIClass spiADC(FSPI);
ADC adc(&spiADC);
MotorControl motorControl;
FeedbackControl  feedbackControl;
WheatstoneBridge bridge1{18000, 10000, 82000, 3.3};
WheatstoneBridge bridge2{18000, 10000, 82000, 3.3};
WheatstoneBridge bridge3{18000, 10000, 82000, 3.3};
PressureSensor sensor1{0.0462, 200000};
PressureSensor sensor2{0.0462, 200000};
PressureSensor sensor3{0.0462, 200000};
Application app(&adc,&motorControl,&feedbackControl,&bridge1,&bridge2,&bridge3,&sensor1,&sensor2,&sensor3);

void setup(){

    WiFi.mode(WIFI_OFF);
    btStop();
    app.setup();
    feedbackControl.updateSetpoint(1, 100);
    feedbackControl.updateSetpoint(2, 100);
    feedbackControl.updateSetpoint(3, 100);
}
void loop(){
    // at 8192000 clock and OSR 16384 -> constant 250 samples per second.
    app.run();
}