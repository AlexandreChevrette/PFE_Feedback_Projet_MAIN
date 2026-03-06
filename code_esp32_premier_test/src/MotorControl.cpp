#include <Arduino.h>
#include "MotorControl.h"
#include <spi.h>

SPIClass spi3(HSPI); //SPI3

// page 59 et 64

MotorControl::MotorControl(){

    //à voir
    //spi3.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);
}

void MotorControl::setPwmFreq(uint8_t p_pwmMode) const{
    uint8_t write = 0b00;  
    uint16_t address = PWM_FREQ_ADDR & 0x3F;
    uint16_t command = (write << 6) | address;
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1)); 
    spi3.transfer(command);
    spi3.transfer(p_pwmMode);
    spi3.endTransaction();
}