#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <spi.h>


const int channelClock = 0;
const int clockRes = 1; // 1-bit res


SPIClass spi2(VSPI); //SPI2

ADC::ADC(): m_adcValues{0.,0.,0.}{
    pinMode(DRDY_PIN, INPUT);
    

    ledcSetup(channelClock, CLOCK_FREQ, clockRes);
    ledcAttachPin(CLOCK_OUT, channelClock);
    spi2.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);

    ADC::setGain();
    ADC::setupChannels(OSR_16256); 
    ADC::flushFrame();
}

void ADC::flushFrame() const
{
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));

    for(int i=0;i<15;i++)
        spi2.transfer(0);

    spi2.endTransaction();
}

void ADC::writeRegister(uint8_t p_reg, uint16_t p_value) const
{
    uint16_t cmd = CMD_WREG | (p_reg << 7) | 0;

    uint8_t buffer[6] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd),
        0x00,                 // padding (voir page 37)

        (uint8_t)(p_value >> 8),
        (uint8_t)(p_value),
        0x00                  // padding
    };

    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    spi2.transfer(buffer, 6);

    // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - 6); i++){
        spi2.transfer(0);
    }


    spi2.endTransaction();


}

void ADC::readData() 
{
    uint32_t status = 0;
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    for(int i=0;i<3;i++)
        status = (status << 8) | spi2.transfer(0);

    for(int ch=0; ch<numberOfChannels; ch++)
    {
        int32_t value = 0;
        
        value |= spi2.transfer(0) << 16;
        value |= spi2.transfer(0) << 8;
        value |= spi2.transfer(0);

        if(value & 0x800000)
            value |= 0xFF000000;

        m_adcValues[ch] = convert24BitToVoltage(value, 1.f);
    }

        // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - (3*(1+numberOfChannels))); i++){
        spi2.transfer(0);
    }

    spi2.endTransaction();
}  

float ADC::convert24BitToVoltage(int32_t p_adcValue, float p_gain) const
{
    const float FSR = 1.2f / p_gain;  // full-scale positive voltage
    return (float)p_adcValue / 8388607.0f * FSR; // 2^23 - 1 = 8388607
}

void ADC::setGain() const{
    writeRegister(GAIN_REG, GAIN_1);
}

void ADC::setupChannels(uint8_t p_osrMode) const{
    // disable channel 3
    // page 45
    uint8_t enable012 = 0b0111;

    uint8_t firstByte = (0b0000 << 4) | enable012;

    uint8_t secondByte = (0b000 << 5) | (p_osrMode << 2) | LOWPOWER;
    uint16_t value = firstByte << 8 | secondByte;

    writeRegister(CLOCK_REG, value);
}

const std::array<float, numberOfChannels>& ADC::getData() const{
    return m_adcValues;
}


// à voir, est-ce qu'il faut flush le register si on reset
// void ADC::reset() const{
//     uint8_t buffer[2] = {
//             (uint8_t)(CMD_RESET >> 8),
//             (uint8_t)(CMD_RESET), 
//     };
//     spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
//     spi2.transfer(buffer, 2);
//     spi2.endTransaction();
//     delay(10);
// }


// put this function on hold, i don't really think its needed (flush register)
// (difficult to run while take data in)
// uint16_t ADC::readRegister(uint8_t p_reg)
// {
//     uint16_t cmd = CMD_RREG | (p_reg << 7) | 0;

//     uint8_t buffer[2] = {
//             (uint8_t)((cmd >> 8) & 0xFF),
//             (uint8_t)(cmd & 0xFF),
//     };

//     uint16_t value = 0;

//     spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 

//     spi2.transfer(buffer, 2);
//     value |= spi2.transfer(0x00) << 8;
//     value |= spi2.transfer(0x00);

//     spi2.endTransaction();

//     return value;
// }
