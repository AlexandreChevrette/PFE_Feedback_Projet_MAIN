#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <SPI.h>

#define DRDY_PIN 14

#define CMD_RESET   0x0011
#define CMD_RREG    0x2000
#define CMD_WREG    0x4000
#define CMD_NULL    0x0000

SPIClass spi;

ADC::ADC(): m_adcValues{0,0,0}{
    pinMode(DRDY_PIN, INPUT);

    // initialise spi transfer
    //setup drdy pin pour lire lorsque c'est prêt
}

uint32_t ADC::spiTransfer24(uint32_t p_data){

    uint32_t rx = 0;
    rx |= spi.transfer((p_data >> 16) & 0xFF) << 16;
    rx |= spi.transfer((p_data >> 8) & 0xFF) << 8;
    rx |= spi.transfer(p_data & 0xFF);

    return rx;
}


void ADC::writeRegister(uint8_t p_reg, uint16_t p_value)
{
    uint16_t cmd = CMD_WREG | (p_reg << 7) | 0;


    spi.transfer((cmd >> 8) & 0xFF);
    spi.transfer(cmd & 0xFF);

    spi.transfer((p_value >> 8) & 0xFF);
    spi.transfer(p_value & 0xFF);
}

uint16_t ADC::readRegister(uint8_t p_reg)
{
    uint16_t cmd = CMD_RREG | (p_reg << 7) | 0;

    spi.transfer((cmd >> 8) & 0xFF);
    spi.transfer(cmd & 0xFF);

    uint16_t value = 0;
    value |= spi.transfer(0x00) << 8;
    value |= spi.transfer(0x00);

    return value;
}

void ADC::readData(int32_t* p_channels)
{
    uint32_t status = 0;

    for(int i=0;i<3;i++)
        status = (status << 8) | spi.transfer(0);

    for(int ch=0; ch<4; ch++)
    {
        int32_t value = 0;

        value |= spi.transfer(0) << 16;
        value |= spi.transfer(0) << 8;
        value |= spi.transfer(0);

        if(value & 0x800000)
            value |= 0xFF000000;

        p_channels[ch] = value;
    }
}


void ADC::reset(){
    spiTransfer24(CMD_RESET);
    delay(10);
}



