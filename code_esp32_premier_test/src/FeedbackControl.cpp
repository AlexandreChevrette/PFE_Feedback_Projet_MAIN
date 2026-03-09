#include <Arduino.h>
#include "FeedbackControl.h"
#include "MotorControl.h"
#include <cmath>

FeedbackControl::FeedbackControl(): m_pidP{PROPORTIONAL},m_pidI{INTEGRAL},
                                    m_pidD{DERIVATIVE},m_errorSum{}, m_setpoints{}{}

void FeedbackControl::updateLoop(MotorControl& p_motorControl, const std::array<float, numberOfChannels>& p_currentRopeTension){
    for (int motorIndex = 1; motorIndex < numberOfChannels+1; motorIndex++){
        applyPID(p_motorControl, p_currentRopeTension[motorIndex-1], motorIndex);
    }
}

void FeedbackControl::applyPID(MotorControl& p_motorControl, float p_currentRopeTension, int motorIndex){
    float error = p_currentRopeTension - m_setpoints[motorIndex-1];
    m_errorSum[motorIndex-1] += error;

    float controlInput = m_pidP*error + m_pidI*m_errorSum[motorIndex-1];

    controlInput >= 0 ? p_motorControl.setForward(motorIndex) : p_motorControl.setReverse(motorIndex);
    uint8_t pwmDutyCycle = (uint8_t)std::abs(controlInput);

    //cap pwm
    if (pwmDutyCycle > 170) pwmDutyCycle = 170;

    p_motorControl.setPWM(motorIndex, pwmDutyCycle);
}


void FeedbackControl::updateSetpoint(size_t motorIndex, float p_ropeTension){
    if ((motorIndex < numberOfChannels+1) && (motorIndex > 0)){
        m_setpoints[motorIndex-1] = p_ropeTension; 
    }
}
void FeedbackControl::setProportional(float p_value){
    m_pidP = p_value;
};
void FeedbackControl::setIntegral(float p_value){
    m_pidI = p_value;
};
void FeedbackControl::setDerivative(float p_value){
    m_pidD = p_value;
};