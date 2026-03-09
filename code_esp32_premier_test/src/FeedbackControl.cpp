#include <Arduino.h>
#include "FeedbackControl.h"
FeedbackControl::FeedbackControl(): m_pidP{0.f},m_pidI{0.f},
                                    m_pidD{0.f},m_errors{0.f, 0.f, 0.f},m_setpoint{0.f}{}
void FeedbackControl::updatePID(const MotorControl& p_motorControl, const std::array<float, numberOfChannels>& p_adcData){
    for (int motorIndex = 1; motorIndex < 4; motorIndex++){
        

    }
}
void FeedbackControl::updateSetpoint(float p_cordeTension){
    m_setpoint = p_cordeTension;
}
void FeedbackControl::setIntegral(float p_value){
    m_pidP = p_value;
};
void FeedbackControl::setIntegral(float p_value){
    m_pidI = p_value;
};
void FeedbackControl::setDerivative(float p_value){
    m_pidD = p_value;
};