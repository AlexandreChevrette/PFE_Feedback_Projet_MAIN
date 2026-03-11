#include <Arduino.h>
#include "MotorControl.h"
#include <spi.h>

SPIClass spi3(HSPI); //SPI3

// page 59 et 64
//PAGE 94 pour les bon registers
//PAGE 105

MotorControl::MotorControl(): m_pwmValues{}, 
                              m_direction1Values{0x00},
                              m_direction2Values{0x00}{}

void MotorControl::setup(){
    pinMode(NSLEEP_MOTOR, OUTPUT);
    digitalWrite(NSLEEP_MOTOR, HIGH);
    pinMode(NSCS_MOTOR, OUTPUT);
    digitalWrite(NSCS_MOTOR, LOW);
    pinMode(NFAULT_MOTOR, INPUT); // not used for now

    spi3.begin(SPI_SCLK_MOTOR, SPI_MISO_MOTOR, SPI_MOSI_MOTOR);
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    //This logic follows page 29
    enablePwmChannels();
    setMotorMode();
    enableSynchronousRectification();
    mapHalfBridges();
    setPwmFreq(PWM_FREQ_2000);
}

// index 1,2,3,4 for motors
void MotorControl::setPWM(size_t p_motorIndex, uint8_t p_pwmValue){
    if ((p_motorIndex < numberOfMotors+1) && (p_motorIndex > 0))
    {
        spi3.transfer(pwmDutyAddress[p_motorIndex-1]);
        spi3.transfer(p_pwmValue);
        m_pwmValues[p_motorIndex-1] = p_pwmValue;
    }
}

void MotorControl::incrementPwm(size_t p_motorIndex){
    if ((p_motorIndex < numberOfMotors+1) && (p_motorIndex > 0))
    {
        m_pwmValues[p_motorIndex-1] += 1;
    }
}

void MotorControl::decrementPwm(size_t p_motorIndex){
    if ((p_motorIndex < numberOfMotors+1) && (p_motorIndex > 0))
    {
        m_pwmValues[p_motorIndex-1] -= 1;
    }
}

void MotorControl::setDirection(size_t p_motorIndex, uint8_t newValue)
{
    if ((p_motorIndex < numberOfMotors + 1) && (p_motorIndex > 0))
    {
        uint8_t* data = (p_motorIndex <= 2) ? &m_direction1Values : &m_direction2Values;
        uint8_t shift = (p_motorIndex % 2) ? 0 : 4;

        *data = (*data & ~(0x0F << shift)) | (newValue << shift);

        spi3.transfer(directionAddress[p_motorIndex - 1]);
        spi3.transfer(*data);
    }
}

void MotorControl::setForward(size_t p_motorIndex){
    setDirection(p_motorIndex, 0b1001);
}

void MotorControl::setReverse(size_t p_motorIndex){
    setDirection(p_motorIndex, 0b0110);
}

void MotorControl::cutPowerMotor(size_t p_motorIndex){
    setDirection(p_motorIndex, 0b0000);
};

void MotorControl::setPwmFreq(uint8_t p_pwmMode) const{
    spi3.transfer(PWM_FREQ1_ADDR);
    spi3.transfer(p_pwmMode);
    spi3.transfer(PWM_FREQ2_ADDR);
    spi3.transfer(p_pwmMode);
}

void MotorControl::mapHalfBridges() const{
    spi3.transfer(PWM_MAP1_ADDR);
    spi3.transfer(MAPPING1); // HB1 and 2 to PWM1
    spi3.transfer(PWM_MAP2_ADDR);
    spi3.transfer(MAPPING2); // HB3 and 4 to PWM2
    spi3.transfer(PWM_MAP3_ADDR);
    spi3.transfer(MAPPING3); // HB5 and 6 to PWM3
    spi3.transfer(PWM_MAP4_ADDR);
    spi3.transfer(MAPPING4); // HB7 and 8 to not used  
}


void MotorControl::setMotorMode() const{
    spi3.transfer(HB_PWM_MODE_ADDR);
    spi3.transfer(PWM_MODE);
}

void MotorControl::enablePwmChannels() const{
    spi3.transfer(PWM_MAP1_ADDR);
    spi3.transfer(PWM_ENABLE1234);
} 


uint8_t MotorControl::getStatus() const{
    spi3.transfer(IC_STATUS_ADDR | READ_ADDRESS);
    uint8_t status = spi3.transfer(0x00); 
    return status;
}

void MotorControl::enableSynchronousRectification() const{
    spi3.transfer(SYNC_RECT_ADDR);
    spi3.transfer(ENABLE_FREE_W);
}

MotorControl::~MotorControl(){
    digitalWrite(NSLEEP_MOTOR, LOW);
    spi3.endTransaction();
}