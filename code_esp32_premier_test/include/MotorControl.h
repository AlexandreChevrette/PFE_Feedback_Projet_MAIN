#ifndef MOTEUR_H_
#define MOTEUR_H_

#include <stdint.h>
#include <Arduino.h>



#define PWM_MAP1_ADDR       0x0F
#define PWM_MAP2_ADDR       0x10
#define PWM_MAP3_ADDR       0x11
#define PWM_MAP4_ADDR       0x12
#define MAPPING1            0x00 // 0000 0000
#define MAPPING2            0x09 // 0000 1001
#define MAPPING3            0x12 // 0001 0010
#define MAPPING4            0x3F // 0011 1111

#define HB_PWM_MODE_ADDR    0x0B
#define PWM_MODE            0xFF 

#define PWM_ENABLE_ADDR     0x0C
#define PWM_ENABLE123       0xF8 // 1111 1000

#define PWM1_DUTY           0x15 
#define PWM2_DUTY           0x16 
#define PWM3_DUTY           0x17 

#define PWM_FREQ1_ADDR      0x13
#define PWM_FREQ2_ADDR      0x14
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