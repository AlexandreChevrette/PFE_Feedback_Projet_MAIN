#include "Arduino.h"
#include "Application.h"
#include "Tests.h"
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include <array>


void MotorTest::setup(){
    m_motorControl.setup();
    Serial.begin(115200);
}

void MotorTest::run(){
    if (Serial.available()) {
        char cmd = Serial.read();

        if (cmd == '1') m_currentMotor = 1;
        if (cmd == '2') m_currentMotor = 2;
        if (cmd == '3') m_currentMotor = 3;
        if (cmd == '[') {
            while (!Serial.available());
            char c3 = Serial.read(); // 'A', 'B', 'C', 'D'
            
            if (c3 == 'A') m_motorControl.incrementPwm(m_currentMotor);
            if (c3 == 'B') m_motorControl.decrementPwm(m_currentMotor);
            if (c3 == 'C') m_motorControl.setForward(m_currentMotor);
            if (c3 == 'D') m_motorControl.setReverse(m_currentMotor);
        }

    }
}


void AdcTest::setup(){
    m_adc.setup();
    Serial.begin(115200);
}

void AdcTest::run(){
    
}




float allowFeedback = false; 


void PidTest::setup(){
    attachInterrupt(digitalPinToInterrupt(DRDY_PIN), Application::drdyISR, FALLING);
    m_adc.setup();
    m_motorControl.setup();
    m_motorControl.cutPowerMotor(1);
    m_motorControl.cutPowerMotor(2);
    m_motorControl.cutPowerMotor(3);

    Serial.begin(115200);
}

void PidTest::run(){
        bool ready = s_dataReady; // safe practice according to Chat

        if (ready && allowFeedback){
            
            s_dataReady = false;

            const std::array<float, numberOfChannels>& adcValues = m_adc.readData();
            float r1 = m_bridge1.convertDeltaVtoR(adcValues[0]);
            float r2 = m_bridge2.convertDeltaVtoR(adcValues[1]);
            float r3 = m_bridge3.convertDeltaVtoR(adcValues[2]);

            const std::array<float, numberOfChannels> currentRopeTension = {
                m_sensor1.convertRtoRopeTension(r1),
                m_sensor2.convertRtoRopeTension(r2),
                m_sensor3.convertRtoRopeTension(r3)
            };
            m_feedbackControl.updateLoop(m_motorControl, currentRopeTension);
        }

        if (Serial.available() >= 3){
            uint8_t cmd = Serial.read();
            uint16_t val;
            Serial.readBytes((char*)&val, 2);

            if (cmd == 1){
                m_feedbackControl.setProportional((float)val);
            }
            if (cmd == 2){
                m_feedbackControl.setIntegral((float)val);
            }
            if (cmd == 3){
                m_feedbackControl.setDerivative((float)val);
            }
            if (cmd == 4){
                allowFeedback = !allowFeedback;
                if (!allowFeedback){
                    m_motorControl.cutPowerMotor(1);
                    m_motorControl.cutPowerMotor(2);
                    m_motorControl.cutPowerMotor(3);
                }
            }
        }
}
