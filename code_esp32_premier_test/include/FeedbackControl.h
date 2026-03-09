#ifndef FEEDBACK_CONTROL_H_
#define FEEDBACK_CONTROL_H_

#include <Arduino.h>
#include "MotorControl.h"
#include "ADC.h"
#include <array>

#define PROPORTIONAL    0.2f
#define INTEGRAL        0.f
#define DERIVATIVE      0.f

class FeedbackControl{
    public:
        FeedbackControl();
        void updateLoop(MotorControl& p_motorControl, const std::array<float, numberOfChannels>& p_currentRopeTension);
        void updateSetpoint(size_t motorIndex, float p_ropeTension); 
        void setProportional(float p_value);
        void setIntegral(float p_value);
        void setDerivative(float p_value);

    private:
        float m_pidP, m_pidI, m_pidD;
        std::array<float, numberOfChannels> m_setpoints, m_errorSum;
        void applyPID(MotorControl& p_motorControl, float p_currentRopeTension, int motorIndex);
};


#endif 