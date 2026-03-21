#include <Arduino.h>
#include "ADC.h"
#include <stdint.h>
#include <SPI.h>
#include <cmath>


ADC::ADC(SPIClass* p_spi): m_adcValues{}, m_spi{p_spi}{}

void ADC::setup(){
    pinMode(DRDY_PIN, INPUT);
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH);
    // clock for ADC chip (conversions)
    ledcSetup(1, CLOCK_FREQ, 1);
    ledcAttachPin(CLOCK_OUT, 1);
    ledcWrite(1, 1); // 50% duty cycle
    delay(100); // let clock stabilize before talking to ADC

    m_spi->begin(SPI_SCLK, SPI_MISO, SPI_MOSI, CS_PIN); 
    delay(100); // let clock stabilize before talking to ADC

    setupChannels(OSR_16384);
}

void ADC::readID() const {
    // RREG command: 0b001a aaaa 000n nnnn
    // ID register = address 0x00 → cmd = 0x2000
    digitalWrite(CS_PIN, LOW);
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    
    // Send RREG command in first frame
    m_spi->transfer(0x20);
    m_spi->transfer(0x00);
    m_spi->transfer(0x00);
    for(int i = 0; i < 15; i++) m_spi->transfer(0x00); // flush rest of frame
    
    m_spi->endTransaction();
    digitalWrite(CS_PIN, HIGH);

    // Response comes in NEXT frame
    while(digitalRead(DRDY_PIN) == HIGH);
    digitalWrite(CS_PIN, LOW);
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    
    uint8_t b0 = m_spi->transfer(0x00); // STATUS byte 0
    uint8_t b1 = m_spi->transfer(0x00); // STATUS byte 1  
    uint8_t b2 = m_spi->transfer(0x00); // STATUS byte 2 — response word here
    uint8_t b3 = m_spi->transfer(0x00); // ID high byte
    uint8_t b4 = m_spi->transfer(0x00); // ID low byte
    for(int i = 0; i < 13; i++) m_spi->transfer(0x00);
    
    m_spi->endTransaction();
    digitalWrite(CS_PIN, HIGH);

}

void ADC::reset() const{
    digitalWrite(CS_PIN, LOW);
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));

    m_spi->transfer(0x00);
    m_spi->transfer(0x11);
    m_spi->transfer(0x00);

    // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - 3); i++){
        m_spi->transfer(0x00);
    }
    m_spi->endTransaction();
    digitalWrite(CS_PIN, HIGH);
}


void ADC::resetSpiInterface() const{
    uint32_t delay_ms = (32768.0f / SPI_SCLK_SPEED) * 1000.0f;
    delay((uint32_t)delay_ms);
}

void ADC::flushFrame() const
{
    digitalWrite(CS_PIN, LOW);
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    for(int i=0;i<18;i++)
        m_spi->transfer(0x00);
    m_spi->endTransaction();
    digitalWrite(CS_PIN, HIGH);
}

void ADC::writeRegister(uint8_t p_reg, uint16_t p_value) const
{   
    digitalWrite(CS_PIN, LOW);
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    uint16_t cmd = CMD_WREG | (p_reg << 7) | 0x00; // 0x00 = write 1 register

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

 
    m_spi->transfer(buffer, 9);

    // flush remaining frames
    for(int i = 0; i < (FRAME_SIZE_BYTES_ADC - 9); i++){
        m_spi->transfer(0);
    }
    m_spi->endTransaction();
    digitalWrite(CS_PIN, HIGH);
}

const std::array<float, numberOfChannels>& ADC::readData() 
{
    m_spi->beginTransaction(SPISettings(SPI_SCLK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(CS_PIN, LOW);

    m_spi->transfer(0x00);
    m_spi->transfer(0x00);
    m_spi->transfer(0x00);

    // 4 canaux — même si tu n'en utilises que 3, tu DOIS lire les 4
    for(int ch = 0; ch < 4; ch++) {
        int32_t value = 0;
        value |= (int32_t)m_spi->transfer(0x00) << 16;
        value |= (int32_t)m_spi->transfer(0x00) << 8;
        value |= (int32_t)m_spi->transfer(0x00);

        if(value & 0x800000)// Max value
            value |= 0xFF000000;
        

        if(ch > 0)
            m_adcValues[ch-1] = convert24BitToVoltage(value, 1.f);

    }

    digitalWrite(CS_PIN, HIGH);
    m_spi->endTransaction();
    
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
    uint8_t enable0123 = 0b1110; //i

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

