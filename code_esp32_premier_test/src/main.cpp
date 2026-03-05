#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;


int ENA = 5;   // PWM for motor speed
int IN1 = 18;  // direction pin
int IN2 = 19;  // direction pin

int pwmChannel = 0;
int pwmFreq = 20000; // 20 kHz
int pwmRes = 8;      // 8-bit PWM

float consigne = 2.5;
float integral = 0;
float proportionnel = 2;
float output = 0;

float erreurSum = 0;

float maxBitPWM = 255;
float maxDutyCycle = 255; // For 8V DC operation
float outputValue = 0;

void setup() {

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  ledcSetup(pwmChannel, pwmFreq, pwmRes);
  ledcAttachPin(ENA, pwmChannel);

  Serial.begin(115200);
  Wire.begin(21, 22);   // SDA, SCL
  ads.setGain(GAIN_ONE);   // ±4.096V
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115.");
    while (1);
  }
}

// void loop() {
//   int16_t adc1 = ads.readADC_SingleEnded(1);
//   float mesure = (float)adc1/32768.*4.096;
//   // Serial.println(mesure);
//   float erreur = mesure - consigne;
//   // Serial.println(erreur);
//   erreurSum += erreur*integral;
//   if (erreur < 0) {
//     digitalWrite(IN2, LOW);
//     digitalWrite(IN1, HIGH);
//     outputValue += (erreur*proportionnel+erreurSum)*maxBitPWM > maxDutyCycle? maxDutyCycle : (erreur*proportionnel+erreurSum)*maxBitPWM ;
//     ledcWrite(pwmChannel, outputValue);
//     Serial.println(outputValue);
//   }
//   else if (erreur >= 0){
//     digitalWrite(IN2, HIGH);
//     digitalWrite(IN1, LOW);
//     outputValue += (erreur*proportionnel+erreurSum)*maxBitPWM > maxDutyCycle? maxDutyCycle : (erreur*proportionnel+erreurSum)*maxBitPWM ;
//     ledcWrite(pwmChannel, outputValue);
//     Serial.println(outputValue);
//   }
//   delay(10);
// }

void loop() {
  int16_t adc1 = ads.readADC_SingleEnded(1);
  float mesure = (float)adc1/32768.*4.096;
  // Serial.println(mesure);
  float erreur = mesure - consigne;
  // Serial.println(erreur);
  erreurSum += erreur*integral;
  output += erreur*proportionnel+erreurSum;

  if (output < 0) {
    digitalWrite(IN2, LOW);
    digitalWrite(IN1, HIGH);

    outputValue = abs(output)+160;
    ledcWrite(pwmChannel, outputValue);
    Serial.println(output-160);
  }
  else if (output >= 0){
    digitalWrite(IN2, HIGH);
    digitalWrite(IN1, LOW);
    outputValue = output+150;
    ledcWrite(pwmChannel, outputValue);
    Serial.println(output+150);
  }
  delay(10);
}