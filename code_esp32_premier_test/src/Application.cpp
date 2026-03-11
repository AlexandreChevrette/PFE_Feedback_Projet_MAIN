#include "Arduino.h"
#include "Application.h"
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include <array>

volatile bool MainApp::s_dataReady = false;

void IRAM_ATTR MainApp::drdyISR()
{
    s_dataReady = true;
}

void MainApp::setup(){
    attachInterrupt(digitalPinToInterrupt(DRDY_PIN), MainApp::drdyISR, FALLING);
    m_adc.setup();
    m_motorControl.setup();
}

void MainApp::run(){
        bool ready = s_dataReady; // safe practice according to Chat

        if (!ready) return;
        
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