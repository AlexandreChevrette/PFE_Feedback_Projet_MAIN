#include <Arduino.h>
#include "MotorControl.h"
#include <spi.h>

SPIClass spi3(HSPI); //SPI3

// page 59 et 64
//PAGE 94 pour les bon registers
//PAGE 105

MotorControl::MotorControl(){

    //à voir
    //spi3.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);
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
    // to change (only one HB has pwm attached)
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


// pour le control, il faudra que je met un des deux en input en PWM et l'autre en 

// uint16_t address = PWM_FREQ_ADDR & 0x3F; for reading i need this