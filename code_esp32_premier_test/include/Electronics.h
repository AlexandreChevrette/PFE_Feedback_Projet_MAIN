#ifndef ELECTRONICS_H_
#define ELECTRONICS_H_

class WheatstoneBridge{
    public:
        WheatstoneBridge()= delete;
        WheatstoneBridge(float p_R1, float p_R2, float p_R3, float p_VIN);
        float convertDeltaVtoR(float p_voltage);
    private:
        float m_R1, m_R2, m_R3, m_VIN;
};

class PressureSensor{
    public:
        PressureSensor()= delete;
        PressureSensor(float p_alpha, float p_beta); // see for coeff
        float convertRtoRopeTension(float p_resistance);
    private:
        float m_alpha, m_beta;
};



#endif