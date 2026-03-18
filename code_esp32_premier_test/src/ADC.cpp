#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <SPI.h>
#include <cmath>

const int channelClock = 0;
const int clockRes = 1; // 1-bit res

SPIClass spi2; //SPI2

ADC::ADC(): m_adcValues{}{}

void ADC::setup(){
    pinMode(DRDY_PIN, INPUT);
    Serial.println("Hello!");
    // clock for ADC chip (conversions)
    ledcSetup(channelClock, CLOCK_FREQ, clockRes);
    ledcAttachPin(CLOCK_OUT, channelClock);
    ledcWrite(channelClock, (1 << (clockRes - 1))); // 50% duty cycle
    
    spi2.begin(SPI_SCLK, SPI_MISO, SPI_MOSI); 

    // setGain();
    // setupChannels(OSR_16384);
    resetSpiInterface();
}




void ADC::resetSpiInterface() const{
    uint32_t delay_ms = (32768.0f / SPI_SCLK_SPEED) * 1000.0f;
    delay((uint32_t)delay_ms);
}

void ADC::flushFrame() const
{
    for(int i=0;i<15;i++)
        spi2.transfer(0);
}

void ADC::writeRegister(uint8_t p_reg, uint16_t p_value) const
{   
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    uint16_t cmd = CMD_WREG | (p_reg << 8);

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
    for (int i = 0; i < 9; i++) {
        Serial.print("0x");
        if (buffer[i] < 0x10) Serial.print("0"); // leading zero
        Serial.print(buffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
 
    spi2.transfer(buffer, 9);

    // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - 9); i++){
        spi2.transfer(0);
        Serial.println("emptying data line");
    }
    spi2.endTransaction();
}

const std::array<float, numberOfChannels>& ADC::readData() 
{
    // Frame 2 : vraies données
    spi2.beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));

    // Status (3 bytes)
    spi2.transfer(0x00);
    spi2.transfer(0x00);
    spi2.transfer(0x00);

    // 4 canaux — même si tu n'en utilises que 3, tu DOIS lire les 4
    for(int ch = 0; ch < 4; ch++) {
        int32_t value = 0;
        value |= (int32_t)spi2.transfer(0x00) << 16;
        value |= (int32_t)spi2.transfer(0x00) << 8;
        value |= (int32_t)spi2.transfer(0x00);
        Serial.println(value);
        if(value & 0x800000)
            value |= 0xFF000000;
        

        if(ch < numberOfChannels)
            m_adcValues[ch] = convert24BitToVoltage(value, 1.f);
    }

    // CRC (3 bytes) — flush
    spi2.transfer(0x00);
    spi2.transfer(0x00);
    spi2.transfer(0x00);

    spi2.endTransaction();
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
    
}

