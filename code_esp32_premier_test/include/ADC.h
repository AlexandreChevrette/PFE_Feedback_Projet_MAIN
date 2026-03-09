#ifndef ADC_H_
#define ADC_H_

#include <Arduino.h>
#include <stdint.h>
#include <array>

#define DRDY_PIN        14
#define SPI_MISO        13
#define SPI_MOSI        11
#define SPI_SCLK        12
#define CLOCK_OUT       10
#define CLOCK_FREQ      4096000
#define SPI_SCLK_SPEED  1000000

#define CMD_RESET       0x0011
#define CMD_RREG        0x2000
#define CMD_WREG        0x4000
#define CMD_NULL        0x0000

#define GAIN_REG        0x04
#define GAIN_1          0x0000

#define CLOCK_REG       0x03   
#define OSR_128         0b000
#define OSR_256         0b001
#define OSR_512         0b010
#define OSR_1024        0b011  
#define OSR_2048        0b100  
#define OSR_4096        0b101  
#define OSR_8192        0b110  
#define OSR_16256       0b111 
#define LOWPOWER        0b01  
#define HIGHPOWER       0b10 


const int FRAME_SIZE_BYTES_ADC = 15; // for 24 bits word length

const int numberOfChannels = 3;


class ADC{
    public:
        ADC();
        ~ADC();
        void readData();
        const std::array<float, numberOfChannels>& getData() const;
        void resetSpiInterface() const;
        void setup();

    private:
        void setGain() const; // set to one for this code
        std::array<float, numberOfChannels> m_adcValues;
        void writeRegister(uint8_t p_reg, uint16_t p_value) const; 
        float convert24BitToVoltage(int32_t p_adcValue, float p_gain) const;
        void flushFrame() const;
        void setupChannels(uint8_t p_osrMode) const; // enable and differential
        // uint16_t readRegister(uint8_t p_reg);
};




#endif 