#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "Arduino.h"
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include <array>


class MainApp{
    public:
        MainApp();
        void setup();
        void run();
    private:
        static void IRAM_ATTR drdyISR();
        static volatile bool s_dataReady;
        ADC m_adc;
        MotorControl m_motorControl;
        FeedbackControl m_feedbackControl;
        WheatstoneBridge m_bridge1{18000, 10000, 82000, 3.3};
        WheatstoneBridge m_bridge2{18000, 10000, 82000, 3.3};
        WheatstoneBridge m_bridge3{18000, 10000, 82000, 3.3};
        PressureSensor m_sensor1{0.0462, 200000};
        PressureSensor m_sensor2{0.0462, 200000};
        PressureSensor m_sensor3{0.0462, 200000};
};



#endif