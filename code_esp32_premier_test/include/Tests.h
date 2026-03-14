#ifndef TESTS_H_
#define TESTS_H_

#include "Application.h"


class MotorTest : public Application{
    public:
        void setup() override;
        void run() override;
    private:
        int m_currentMotor = 1;
};

class AdcTest : public Application{
    public:
        void setup() override;
        void run() override;
    private:
        int m_currentAdc = 1;
};

class PidTest : public Application{
    public:
        void setup() override;
        void run() override;
};


#endif