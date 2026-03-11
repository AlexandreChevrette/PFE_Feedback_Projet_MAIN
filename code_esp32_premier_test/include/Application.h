#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "Arduino.h"
#include "ADC.h"
#include "MotorControl.h"
#include "FeedbackControl.h"
#include "Electronics.h"
#include <array>


class Application{
    public:
        virtual void setup();
        virtual void run();
    protected:
        static volatile bool s_dataReady;
        static void IRAM_ATTR drdyISR();
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

class MainApp : public Application{
    public:
        MainApp();        
};



#endif