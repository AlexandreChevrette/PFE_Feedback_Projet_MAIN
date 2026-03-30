#include "Arduino.h"
#include "ADC.h"
#include "SPI.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include "Application.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "BluetoothComm.h"


SPIClass spiADC(FSPI);
ADC adc(&spiADC);
MotorControl motorControl;
FeedbackControl  feedbackControl;
WheatstoneBridge bridge1{18000, 10000, 82000, 3.3};
WheatstoneBridge bridge2{18000, 10000, 82000, 3.3};
WheatstoneBridge bridge3{18000, 10000, 82000, 3.3};
PressureSensor sensor1{3065365, 21539, 15};
PressureSensor sensor2{3065365, 21539, 15};
PressureSensor sensor3{3065365, 21539, 15};

Bluetooth bluetooth;


Application app(&adc,&motorControl,&feedbackControl,&bridge1,&bridge2,&bridge3,&sensor1,&sensor2,&sensor3,&bluetooth);

 

void setup(){
    app.setup();
}
void loop(){
    // at 8192000 clock and OSR 16384 -> constant 250 samples per second.
    app.run();
}