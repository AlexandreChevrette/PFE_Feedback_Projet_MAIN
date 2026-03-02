#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);   // SDA, SCL
  ads.setGain(GAIN_ONE);   // ±4.096V
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115.");
    while (1);
  }
}

void loop() {
  int16_t adc1 = ads.readADC_SingleEnded(1);
  Serial.println((float)adc1/32768.*4.096, 4);
  delay(10);
}