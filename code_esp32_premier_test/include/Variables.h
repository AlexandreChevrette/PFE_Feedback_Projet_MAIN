#ifndef VARIABLES_H_
#define VARIABLES_H_

#include <Arduino.h>

// PID

extern float consigne0; // en g
extern float consigne1; // en g
extern float consigne2; // en g

extern float pidP;
extern float pidI;
extern float pidD;

// Lecture pression

extern float adc0; // en volt
extern float adc1; // en volt
extern float adc2; // en volt

extern float tension0; // en g
extern float tension1; // en g
extern float tension2; // en g

// Commande moteurs

// Pourcentage PWM
extern float pPwmMot0; // en %
extern float pPwmMot1; // en %
extern float pPwmMot2; // en %


#endif 