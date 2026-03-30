#include "Arduino.h"
#include "Electronics.h"
#include <Wire.h>
#include <Adafruit_ADS1X15.h>


DiviseurTension bridge1{43000, 3.3};
DiviseurTension bridge2{43000, 3.3};
DiviseurTension bridge3{43000, 3.3};
PressureSensor sensor1{3065365, 21539, 15};
PressureSensor sensor2{3065365, 21539, 15};
PressureSensor sensor3{3065365, 21539, 15};

Adafruit_ADS1115 ads;

void setup(){
    Serial.begin(115200);
    Wire.begin(21, 22);   // SDA, SCL
    ads.begin();
    ads.setGain(GAIN_ONE);   // ±4.096V
}
void loop(){
    int16_t adc1 = ads.readADC_SingleEnded(1);
    float mesure = (float)adc1/32768.*4.096;

    float R1 = bridge1.convertDeltaVtoR(mesure);
    float tension1 = sensor1.convertRtoRopeTension(R1);

    int16_t adc2 = ads.readADC_SingleEnded(2);
    float mesure2 = (float)adc2/32768.*4.096;

    float R2 = bridge2.convertDeltaVtoR(mesure2);
    float tension2 = sensor2.convertRtoRopeTension(R2);

    int16_t adc3 = ads.readADC_SingleEnded(3);
    float mesure3 = (float)adc3/32768.*4.096;

    float R3 = bridge3.convertDeltaVtoR(mesure3);
    float tension3 = sensor3.convertRtoRopeTension(R3);

    Serial.printf("Tension 1: %f, Tension 2: %f, Tension 3: %f\n", tension1, tension2, tension3);
    delay(10);
}