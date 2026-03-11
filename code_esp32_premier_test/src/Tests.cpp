#include "Arduino.h"
#include "Tests.h"
#include "Application.h"
#include "MotorControl.h"

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

