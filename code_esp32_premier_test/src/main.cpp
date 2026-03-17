#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;

int i = 150;

int ENA = 5;   // PWM for motor speed
int IN1 = 18;  // direction pin
int IN2 = 19;  // direction pin

int pwmChannel = 0;
int pwmFreq = 20000; // 20 kHz
int pwmRes = 8;      // 8-bit PWM

int16_t adc1 = 0;
int pwm = 0;
int pwm2 = 0;
float consigne = 0;
float mesure = 0;
int flag = 0;


float maxBitPWM = 255;
float maxDutyCycle = 255; // For 8V DC operation
float outputValue = 0;

int value = 0;
 
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


void loop() {
  int16_t adc1 = ads.readADC_SingleEnded(1);
  float mesure = (float)adc1/32768.*4.096;


  if (Serial.available() > 0) {
    pwm = Serial.parseInt();   // reads the integer sent
    Serial.print("You entered: ");
    Serial.println(pwm);
  }

  if (pwm != pwm2){
    if (pwm < pwm2){
      digitalWrite(IN2, LOW);
      digitalWrite(IN1, HIGH);
      ledcWrite(pwmChannel, 160);
      delay((pwm2 - pwm)*7);
    }
    digitalWrite(IN2, HIGH);
    digitalWrite(IN1, LOW);
    ledcWrite(pwmChannel, pwm);
    delay(500);
    adc1 = ads.readADC_SingleEnded(1);
    consigne = (float)adc1/32768.*4.096;
    pwm2 = pwm;
  }

  adc1 = ads.readADC_SingleEnded(1);
  mesure = (float)adc1/32768.*4.096;

  if (mesure < consigne - 0.10){
    flag = 1;
  }

  if (flag == 1 && mesure < consigne){
    digitalWrite(IN2, LOW);
    digitalWrite(IN1, HIGH);
    ledcWrite(pwmChannel, 160);
  }

  else if (flag == 1 && mesure >= consigne){
    digitalWrite(IN2, HIGH);
    digitalWrite(IN1, LOW);
    ledcWrite(pwmChannel, pwm);
    delay(500);
    int16_t adc1 = ads.readADC_SingleEnded(1);
    consigne = (float)adc1/32768.*4.096;
    flag = 0;
  }

  Serial.print("Consigne = ");
  Serial.print(consigne);
  Serial.print("     Mesure = ");
  Serial.println(mesure);


  delay(1);
}