#include <Arduino.h>
#include "FeedbackControl.h"
FeedbackControl::FeedbackControl(): m_pidP{0.f},m_pidI{0.f},
                                    m_pidD{0.f},m_errors{},m_setpoints{}{}
void FeedbackControl::updatePID(const MotorControl& p_motorControl, const std::array<float, numberOfChannels>& p_adcData){
    for (int motorIndex = 1; motorIndex < 4; motorIndex++){
        

    }
}
void FeedbackControl::setup(){

    // possibly remove this functions
}

void FeedbackControl::updateSetpoint(size_t motorIndex, float p_cordeTension){
    if ((motorIndex < numberOfChannels+1) && (motorIndex > 0)){
        m_setpoints[motorIndex-1] = p_cordeTension; 
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