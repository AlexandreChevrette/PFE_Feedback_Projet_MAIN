#include "Electronics.h"

WheatstoneBridge::WheatstoneBridge(float p_R1, float p_R2, float p_R3, float p_VIN):
                                    m_R1{p_R1},m_R2{p_R2},m_R3{p_R3}, m_VIN{p_VIN}{}

float WheatstoneBridge::convertDeltaVtoR(float p_voltage){
    float num = m_R2*m_VIN + (m_R1+m_R2)*p_voltage;
    float denum = m_R1*m_VIN - (m_R1+m_R2)*p_voltage;
    return num/denum*m_R3; // see wiki wheastone bridge (I inverted p_voltage so negative sign)
}



