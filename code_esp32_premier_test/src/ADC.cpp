#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <spi.h>

#define DRDY_PIN 14
#define SPI_MISO 13
#define SPI_MOSI 11
#define SPI_SCLK 12
#define CLOCK_OUT 10
#define CLOCK_FREQ 5000000
const int channelClock = 0;
const int clockRes = 1; // 1-bit res

#define SPI_SCLK_SPEED 1000000

#define CMD_RESET   0x0011
#define CMD_RREG    0x2000
#define CMD_WREG    0x4000
#define CMD_NULL    0x0000

#define GAIN_REG    0x04
#define GAIN_1      0x0000


SPIClass spi2(VSPI); //SPI2

ADC::ADC(): m_adcValues{0,0,0}{
    pinMode(DRDY_PIN, INPUT);
    ledcSetup(channelClock, CLOCK_FREQ, clockRes);
    ledcAttachPin(CLOCK_OUT, channelClock);
    spi2.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);


    //setup drdy pin pour lire lorsque c'est prêt
}

uint32_t ADC::spiTransfer24(uint32_t p_data){
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    uint32_t rx = 0;
    rx |= spi2.transfer((p_data >> 16) & 0xFF) << 16;
    rx |= spi2.transfer((p_data >> 8) & 0xFF) << 8;
    rx |= spi2.transfer(p_data & 0xFF);
    spi2.endTransaction();
    return rx;
}


void ADC::writeRegister(uint8_t p_reg, uint16_t p_value)
{
    uint16_t cmd = CMD_WREG | (p_reg << 7) | 0;

    uint8_t buffer[4] = {
            (uint8_t)(cmd >> 8), // MSByte register
            (uint8_t)(cmd), // LSByte register
            (uint8_t)(p_value >> 8), //MSByte value
            (uint8_t)(p_value) //MSByte value
    };

    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    spi2.transfer(buffer, 4);
    spi2.endTransaction();
}

uint16_t ADC::readRegister(uint8_t p_reg)
{
    uint16_t cmd = CMD_RREG | (p_reg << 7) | 0;

    uint8_t buffer[2] = {
            (uint8_t)((cmd >> 8) & 0xFF),
            (uint8_t)(cmd & 0xFF),
    };

    uint16_t value = 0;

    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 

    spi2.transfer(buffer, 2);
    value |= spi2.transfer(0x00) << 8;
    value |= spi2.transfer(0x00);

    spi2.endTransaction();

    return value;
}

void ADC::readData(int32_t* p_channels)
{
    // cette fonction me semble louche
    uint32_t status = 0;
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    for(int i=0;i<3;i++)
        status = (status << 8) | spi2.transfer(0);

    for(int ch=0; ch<4; ch++)
    {
        int32_t value = 0;
        
        value |= spi2.transfer(0) << 16;
        value |= spi2.transfer(0) << 8;
        value |= spi2.transfer(0);

        if(value & 0x800000)
            value |= 0xFF000000;

        p_channels[ch] = value;
    }
    spi2.endTransaction();
}


void ADC::reset(){
    spiTransfer24(CMD_RESET);
    delay(10);
}

void ADC::setGain(){
    writeRegister(GAIN_REG, GAIN_1);
}


void ADC::setSampleRate(int p_sampleSpeed){

}


void ADC::setupChannel(int channel){

}

void ADC::disableChannel(int channel){

}

void ADC::readValues(std::array<float, numberOfChannels>& p_values){

}