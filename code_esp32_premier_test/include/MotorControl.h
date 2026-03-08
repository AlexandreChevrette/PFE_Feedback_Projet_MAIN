#ifndef MOTEUR_H_
#define MOTEUR_H_

#include <stdint.h>
#include <Arduino.h>
#include <array>

#define SPI_SCLK_MOTOR      7
#define SPI_MISO_MOTOR      5
#define SPI_MOSI_MOTOR      4    
#define NSCS_MOTOR          8
#define NFAULT_MOTOR        9
#define NSLEEP_MOTOR        6


#define DIRECTION_ADDR1     0x08
#define DIRECTION_ADDR2     0x09

// I map 2,4,6 to PWMs but I don't assign them PWM mode. 
#define PWM_MAP1_ADDR       0x0F
#define PWM_MAP2_ADDR       0x10
#define PWM_MAP3_ADDR       0x11
#define PWM_MAP4_ADDR       0x12
#define MAPPING1            0x00 // 0000 0000
#define MAPPING2            0x09 // 0000 1001
#define MAPPING3            0x12 // 0001 0010
#define MAPPING4            0x1B // 0001 1011

#define HB_PWM_MODE_ADDR    0x0B
#define PWM_MODE            0xD5 // 1101 0101
// setting HB1, 3, 5, 7, 8 to PWM. (7 and 8 will be set to low) 

#define PWM_ENABLE_ADDR     0x0C
#define PWM_ENABLE1234      0xF0 // 1111 0000

#define PWM1_DUTY_ADDR      0x15 
#define PWM2_DUTY_ADDR      0x16 
#define PWM3_DUTY_ADDR      0x17
#define PWM4_DUTY_ADDR      0x18

#define PWM_FREQ1_ADDR      0x13
#define PWM_FREQ2_ADDR      0x14
#define PWM_FREQ_80         0x00
#define PWM_FREQ_2000       0xFF
#define SPI_SCLK_SPEED_MOT  1000000

#define SYNC_RECT_ADDR      0x0D // Free-wheeling
#define ENABLE_FREE_W       0xFF // 1111 1111

#define READ_ADDRESS        0x40 // concat to address to read

#define IC_STATUS_ADDR      0x00

const int numberOfMotors = 4;

std::array<uint8_t, numberOfMotors> pwmDutyAddress{PWM1_DUTY_ADDR, PWM2_DUTY_ADDR, PWM3_DUTY_ADDR, PWM4_DUTY_ADDR};
std::array<uint8_t, numberOfMotors> directionAddress{DIRECTION_ADDR1, DIRECTION_ADDR1, DIRECTION_ADDR2, DIRECTION_ADDR2};

const int FRAME_SIZE_BYTES_MOT = 16;

class MotorControl{
    public:
        MotorControl();
        ~MotorControl();
        void setPWM(size_t p_motorIndex, uint8_t p_pwmValue);
        void setForward(size_t p_motorIndex);
        void setReverse(size_t p_motorIndex);
        uint8_t getStatus() const;

    private:
        std::array<uint8_t, numberOfMotors> m_pwmValues;
        byte m_direction1Values;
        byte m_direction2Values;
        void setDirection(size_t p_motorIndex, uint8_t newValue);
        void mapHalfBridges() const;
        void setMotorMode() const; 
        void enablePwmChannels() const; 
        void setPwmFreq(uint8_t p_pwmMode) const;
        void enableSynchronousRectification() const;
        // page 82
};


#endif 