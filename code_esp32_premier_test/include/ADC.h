#ifndef ADC_H_
#define ADC_H_

#include <Arduino.h>
#include "Variables.h"
#include <stdint.h>
#include <array>

const int numberOfChannels = 3;

class ADC{
    public:
        ADC();
        void reset();
        void setGain(); // set to one for this code
        void setSampleRate(int p_sampleSpeed);
        void setupChannel(int p_channel); // enable and differential
        void disableChannel(int p_channel);
        void readValues(std::array<float, numberOfChannels>& p_values);
        
    
    private:
        std::array<float, numberOfChannels> m_adcValues;
        uint32_t spiTransfer24(uint32_t p_data);
        void writeRegister(uint8_t p_reg, uint16_t p_value); 
        uint16_t readRegister(uint8_t p_reg);
        void ADC::readData(int32_t* p_channels);
};



/*chat me dit de mettre ça? 
volatile bool dataReady = false;

void IRAM_ATTR drdyISR()
{
    dataReady = true;
}

void setup()
{
    pinMode(14, INPUT);
    attachInterrupt(14, drdyISR, FALLING);
}*/


#endif 