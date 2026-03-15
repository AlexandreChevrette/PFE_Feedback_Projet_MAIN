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

float consigne = 2.5;
float integral = 0; //0.1
float proportionnel = 5; //20
float differentiel = 0; //500000

float output = 0;
float erreurSum = 0;
float erreurDif = 0;
float tim1 = 0;
float tim2 = 0;
float erreur2 = 0;
float dt = 0;

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
  tim1 = millis();
}


void loop() {
  int16_t adc1 = ads.readADC_SingleEnded(1);
  float mesure = (float)adc1/32768.*4.096;
  // Serial.println(mesure);
  float erreur = mesure - consigne;
  // Serial.println(erreur);

  tim2 = millis();
  dt = (tim2 - tim1)/1000.0;

  if (abs(erreur/consigne) < 0.20){
    erreurSum += erreur*dt;
  }
  else{
    erreurSum = 0;
  }

  if (dt > 0) {
    erreurDif = (erreur - erreur2) / dt;
  }

  tim1 = tim2;
  erreur2 = erreur;
  
  
  output = erreur*proportionnel + integral*erreurSum + differentiel*erreurDif;
  output = constrain(output, -255, 255);

  if (Serial.available() > 0) {
    consigne = Serial.parseInt()/10.;   // reads the integer sent
    Serial.print("You entered: ");
    Serial.println(consigne);
  }

  if (output < 0) {
    digitalWrite(IN2, LOW);
    digitalWrite(IN1, HIGH);
    outputValue = abs(output)+160;
    ledcWrite(pwmChannel, outputValue);
    Serial.print("Consigne = ");
    Serial.print(consigne);
    Serial.print("    Mesure = ");
    Serial.print(mesure);
    Serial.print("    Erreur Dif = ");
    Serial.print(erreurDif);
    Serial.print("    Output = ");
    Serial.println(output-160);
  }
  else if (output >= 0){
    digitalWrite(IN2, HIGH);
    digitalWrite(IN1, LOW);
    outputValue = output+150;
    ledcWrite(pwmChannel, outputValue);
    Serial.print("Consigne = ");
    Serial.print(consigne);
    Serial.print("    Mesure = ");
    Serial.print(mesure);
    Serial.print("    Erreur Dif = ");
    Serial.print(erreurDif);
    Serial.print("    Output = ");
    Serial.println(output+150);
  }
  delay(1);
}