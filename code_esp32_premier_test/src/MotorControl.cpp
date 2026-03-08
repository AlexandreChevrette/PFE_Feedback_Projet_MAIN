#include <Arduino.h>
#include "MotorControl.h"
#include <spi.h>

SPIClass spi3(HSPI); //SPI3

// page 59 et 64
//PAGE 94 pour les bon registers
//PAGE 105

MotorControl::MotorControl(): m_pwmValues{0,0,0,0}, 
                              m_pwmDutyAddress{PWM1_DUTY_ADDR, PWM2_DUTY_ADDR, PWM3_DUTY_ADDR, PWM4_DUTY_ADDR},
                              m_directionAddress{DIRECTION_ADDR1, DIRECTION_ADDR1, DIRECTION_ADDR2, DIRECTION_ADDR2},
                              m_direction1Values{0x00},
                              m_direction2Values{0x00}
{

    //à voir
    //spi3.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);
}

// index 1,2,3,4 for motors
void MotorControl::setPWM(size_t p_motorIndex, uint8_t p_pwmValue){
    if ((p_motorIndex < numberOfMotors+1) && (p_motorIndex > 0))
    {
        spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
        spi3.transfer(m_pwmDutyAddress[p_motorIndex-1]);
        spi3.transfer(p_pwmValue);
        spi3.endTransaction();
        m_pwmValues[p_motorIndex-1] = p_pwmValue;
    }
}

void MotorControl::setDirection(size_t p_motorIndex, uint8_t newValue)
{
    if ((p_motorIndex < numberOfMotors + 1) && (p_motorIndex > 0))
    {
        uint8_t* reg = (p_motorIndex <= 2) ? &m_direction1Values : &m_direction2Values;
        uint8_t shift = (p_motorIndex % 2) ? 0 : 4;

        *reg = (*reg & ~(0x0F << shift)) | ((newValue & 0x0F) << shift);

        spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1));
        spi3.transfer(m_directionAddress[p_motorIndex - 1]);
        spi3.transfer(*reg);
        spi3.endTransaction();
    }
}

void MotorControl::setForward(size_t p_motorIndex){
    setDirection(p_motorIndex, 0b1001);
}

void MotorControl::setReverse(size_t p_motorIndex){
    setDirection(p_motorIndex, 0b0110)
}

void MotorControl::setPwmFreq(uint8_t p_pwmMode) const{
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    spi3.transfer(PWM_FREQ1_ADDR);
    spi3.transfer(p_pwmMode);
    spi3.transfer(PWM_FREQ2_ADDR);
    spi3.transfer(p_pwmMode);
    spi3.endTransaction();
}

void MotorControl::mapHalfBridges() const{
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    spi3.transfer(PWM_MAP1_ADDR);
    spi3.transfer(MAPPING1); // HB1 and 2 to PWM1
    spi3.transfer(PWM_MAP2_ADDR);
    spi3.transfer(MAPPING2); // HB3 and 4 to PWM2
    spi3.transfer(PWM_MAP3_ADDR);
    spi3.transfer(MAPPING3); // HB5 and 6 to PWM3
    spi3.transfer(PWM_MAP4_ADDR);
    spi3.transfer(MAPPING4); // HB7 and 8 to not used  
    spi3.endTransaction();
}


void MotorControl::setMotorMode() const{
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    spi3.transfer(HB_PWM_MODE_ADDR);
    spi3.transfer(PWM_MODE);
    spi3.endTransaction();
}

void MotorControl::enablePwmChannels() const{
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    spi3.transfer(PWM_MAP1_ADDR);
    spi3.transfer(PWM_ENABLE1234);
    spi3.endTransaction();
} 


// uint16_t address = PWM_FREQ_ADDR & 0x3F; for reading i need this