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
        Application(ADC*, MotorControl*, FeedbackControl*, WheatstoneBridge*,WheatstoneBridge*,WheatstoneBridge*, PressureSensor*, PressureSensor*, PressureSensor*);
        virtual void setup();
        virtual void run();
    protected:
        static volatile bool s_dataReady;
        static void IRAM_ATTR drdyISR();
        ADC* m_adc;
        MotorControl* m_motorControl;
        FeedbackControl* m_feedbackControl;
        WheatstoneBridge* m_bridge1;
        WheatstoneBridge* m_bridge2;
        WheatstoneBridge* m_bridge3;
        PressureSensor* m_sensor1;
        PressureSensor* m_sensor2;
        PressureSensor* m_sensor3;

};

// class MainApp : public Application{
//     public:
//         MainApp();        
// };



#endif