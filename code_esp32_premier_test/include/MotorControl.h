#ifndef MOTEUR_H_
#define MOTEUR_H_

#include <stdint.h>
#include <Arduino.h>


#define PWM_FREQ_ADDR       0x18
#define PWM_FREQ_80         0x00
#define PWM_FREQ_2000       0xFF
#define SPI_SCLK_SPEED_MOT  1000000

#define PWM_CHANNEL_1       0b00
#define PWM_CHANNEL_2       0b01
#define PWM_CHANNEL_3       0b10
#define PWM_CHANNEL_4       0b11

const int FRAME_SIZE_BYTES_MOT = 16;

class MotorControl{
    public:
        MotorControl();
        void setSpeed(uint8_t p_indiceMoteur, uint8_t p_speed) const;
        void setPwmFreq(uint8_t p_pwmMode) const;
        

    private:
        void mapHalfBridges() const;
        // page 82
};


#endif 