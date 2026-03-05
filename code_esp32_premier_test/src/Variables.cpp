#include <Arduino.h>
#include "Variables.h"

// PID

float consigne0 = 0.; // en g
float consigne1 = 0.; // en g
float consigne2 = 0.; // en g

float pidP = 0.2;
float pidI = 0.;
float pidD = 0.;

// Lecture pression

float adc0 = 0.; // en volt
float adc1 = 0.; // en volt
float adc2 = 0.; // en volt

float tension0 = 0.; // en g
float tension1 = 0.; // en g
float tension2 = 0.; // en g

// Commande moteurs

// Pourcentage PWM
float pPwmMot0 = 0.; // en %
float pPwmMot1 = 0.; // en %
float pPwmMot2 = 0.; // en %