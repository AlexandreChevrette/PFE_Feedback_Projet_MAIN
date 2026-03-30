#include "Electronics.h"

DiviseurTension::DiviseurTension(float p_R1, float p_VIN):
                                    m_R1{p_R1}, m_VIN{p_VIN}{}

float DiviseurTension::convertDeltaVtoR(float p_voltage){
    float num = m_R1 * p_voltage;
    float denum = m_VIN - p_voltage;
    return denum > 0 ? num/denum : 0; 
}

PressureSensor::PressureSensor(float p_alpha1, float p_alpha2, float p_alpha3):
                                m_alpha1{p_alpha1}, m_alpha2{p_alpha3}, m_alpha3{p_alpha3}{}

float PressureSensor::convertRtoRopeTension(float p_resistance){
    return m_alpha1/(p_resistance-m_alpha2)- m_alpha3 > 0 ? m_alpha1/(p_resistance-m_alpha2)- m_alpha3 : 0;
}

