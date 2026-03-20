#include <Arduino.h>
#include "FeedbackControl.h"
#include "MotorControl.h"
#include <cmath>

FeedbackControl::FeedbackControl(): m_pidP{PROPORTIONAL},m_pidI{INTEGRAL},
                                    m_pidD{DERIVATIVE},m_errorSum{}, m_setpoints{}, 
                                    m_pwmValues{}{}

void FeedbackControl::updateLoop(MotorControl* p_motorControl, const std::array<float, numberOfChannels>& p_currentRopeTension){
    for (int motorIndex = 1; motorIndex < numberOfChannels+1; motorIndex++){
        applyPID(p_motorControl, p_currentRopeTension[motorIndex-1], motorIndex);
    }
}

void FeedbackControl::applyPID(MotorControl* p_motorControl, float p_currentRopeTension, int motorIndex){
    float error = p_currentRopeTension - m_setpoints[motorIndex-1];
    m_errorSum[motorIndex-1] += error;

    float controlInput = m_pidP*error + m_pidI*m_errorSum[motorIndex-1];
    bool sign = controlInput >=0 ? 1:0;
    // Serial.println(controlInput);
    if (error > 0.1) p_motorControl->setForward(motorIndex);
    else if (error < -0.1) p_motorControl->setReverse(motorIndex);
    else p_motorControl->cutPowerMotor(motorIndex);
    
    if (abs(controlInput) > 220.0f) controlInput = 220.0f;
    
    uint8_t pwmDutyCycle = (uint8_t)std::abs(controlInput);

    m_pwmValues[motorIndex-1] = pwmDutyCycle* sign ? 1:-1; // multiply by direction
    
    p_motorControl->setPWM(motorIndex, pwmDutyCycle);
}


void FeedbackControl::updateSetpoint(size_t motorIndex, float p_ropeTension){
    if ((motorIndex < numberOfChannels+1) && (motorIndex > 0)){
        m_setpoints[motorIndex-1] = p_ropeTension; 
    }
    Serial.println(m_setpoints[0]);
    Serial.println(m_setpoints[1]);
    Serial.println(m_setpoints[2]);
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

const std::array<float, numberOfChannels>&  FeedbackControl::getCommands() const{
    return m_pwmValues;
}
const std::array<float, numberOfChannels>&  FeedbackControl::getSetPoints() const{
    return m_setpoints;
}
float FeedbackControl::getPropotional() const{
    return m_pidP;
}
float FeedbackControl::getIntegral() const{
    return m_pidI;
}
float FeedbackControl::getDerivative() const{
    return m_pidD;
}