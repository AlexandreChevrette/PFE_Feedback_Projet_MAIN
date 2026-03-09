#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <spi.h>
#include <cmath>

const int channelClock = 0;
const int clockRes = 1; // 1-bit res


SPIClass spi2(VSPI); //SPI2

ADC::ADC(): m_adcValues{}{}

void ADC::setup(){
    pinMode(DRDY_PIN, INPUT);
    
    // clock for ADC chip (conversions)
    ledcSetup(channelClock, CLOCK_FREQ, clockRes);
    ledcAttachPin(CLOCK_OUT, channelClock);
    
    spi2.begin(SPI_SCLK, SPI_MISO, SPI_MOSI);
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1)); 
    setGain();
    setupChannels(OSR_16384); 
    resetSpiInterface();
}

void ADC::resetSpiInterface() const{
    delay(ceil((float)pow(2, 15)/(float)SPI_SCLK_SPEED*1000.0f)); //reset SPI interface (2^15 clocks)
}

void ADC::flushFrame() const
{
    for(int i=0;i<15;i++)
        spi2.transfer(0);
}

void ADC::writeRegister(uint8_t p_reg, uint16_t p_value) const
{
    uint16_t cmd = CMD_WREG | (p_reg << 7) | 0;

    uint8_t buffer[9] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd),
        0x00,                 // padding (voir page 37)

        (uint8_t)(p_value >> 8),
        (uint8_t)(p_value),
        0x00,                  // padding

        0x00, // EMPTY CRC
        0x00,
        0x00
    };

 
    spi2.transfer(buffer, 9);

    // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - 9); i++){
        spi2.transfer(0);
    }
}

const std::array<float, numberOfChannels>& ADC::readData() 
{
    uint32_t status = 0;
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
    
    return m_adcValues;
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
    uint8_t enable0123 = 0b1111; // I keep the fourth adc because I don't know how it impacts spi

    uint8_t firstByte = (0b0000 << 4) | enable0123;

    uint8_t secondByte = (0b000 << 5) | (p_osrMode << 2) | LOWPOWER;
    uint16_t value = firstByte << 8 | secondByte;

    writeRegister(CLOCK_REG, value);
}

const std::array<float, numberOfChannels>& ADC::getData() const{
    return m_adcValues;
}

ADC::~ADC(){
    spi2.endTransaction();
}

