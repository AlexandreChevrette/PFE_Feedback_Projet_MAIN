#include <Arduino.h>
#include "MotorControl.h"
#include <SPI.h>

SPIClass spi3(HSPI); // SPI3

MotorControl::MotorControl()
    : m_pwmValues{},
      m_direction1Values{0x00},
      m_direction2Values{0x00}
{}

void MotorControl::writeRegister(uint8_t address, uint8_t value) const {
    digitalWrite(NSCS_MOTOR, LOW);
    spi3.transfer(address);
    spi3.transfer(value);
    digitalWrite(NSCS_MOTOR, HIGH);
    delay(1);
}

uint8_t MotorControl::readRegister(uint8_t address) const {
    digitalWrite(NSCS_MOTOR, LOW);
    spi3.transfer(address | READ_ADDRESS);
    uint8_t value = spi3.transfer(0x00);
    spi3.transfer(0x00);
    digitalWrite(NSCS_MOTOR, HIGH);
    delay(1);
    return value;
}

void MotorControl::setup() {
    pinMode(NSLEEP_MOTOR, OUTPUT);
    digitalWrite(NSLEEP_MOTOR, HIGH);

    pinMode(NSCS_MOTOR, OUTPUT);
    digitalWrite(NSCS_MOTOR, HIGH);

    pinMode(NFAULT_MOTOR, INPUT);

    spi3.begin(SPI_SCLK_MOTOR, SPI_MISO_MOTOR, SPI_MOSI_MOTOR);
    spi3.beginTransaction(SPISettings(SPI_SCLK_SPEED_MOT, MSBFIRST, SPI_MODE1));

    enablePwmChannels();
    setMotorMode();
    enableSynchronousRectification();
    enableDangerousMode();
    disableOLD();
    mapHalfBridges();
    setPwmFreq(PWM_FREQ_2000);
}


void MotorControl::setPWM(size_t p_motorIndex, uint8_t p_pwmValue) {
    if ((p_motorIndex < numberOfMotors + 1) && (p_motorIndex > 0)) {
        writeRegister(pwmDutyAddress[p_motorIndex - 1], p_pwmValue);
        m_pwmValues[p_motorIndex - 1] = p_pwmValue;
    }
}

void MotorControl::setDirection(size_t p_motorIndex, uint8_t newValue) {
    if ((p_motorIndex < numberOfMotors + 1) && (p_motorIndex > 0)) {

        uint8_t* data = (p_motorIndex % 2)
                        ? &m_direction1Values
                        : &m_direction2Values;

        uint8_t shift = (p_motorIndex > 1) ? 0 : 4;

        *data = (*data & ~(0x0F << shift)) | (newValue << shift);

        writeRegister(directionAddress[p_motorIndex - 1], *data);
    }
}

void MotorControl::setForward(size_t p_motorIndex) {
    setDirection(p_motorIndex, 0b1001);
}

void MotorControl::setReverse(size_t p_motorIndex) {
    setDirection(p_motorIndex, 0b0110);
}

void MotorControl::cutPowerMotor(size_t p_motorIndex) {
    setDirection(p_motorIndex, 0b0000);
}


void MotorControl::setPwmFreq(uint8_t p_pwmMode) const {
    writeRegister(PWM_FREQ1_ADDR, p_pwmMode);
    writeRegister(PWM_FREQ2_ADDR, p_pwmMode);
}

void MotorControl::mapHalfBridges() const {
    writeRegister(PWM_MAP1_ADDR, MAPPING3);
    writeRegister(PWM_MAP2_ADDR, MAPPING1);
    writeRegister(PWM_MAP3_ADDR, MAPPING2);
    writeRegister(PWM_MAP4_ADDR, MAPPING4);
}

void MotorControl::setMotorMode() const {
    writeRegister(HB_PWM_MODE_ADDR, PWM_MODE);
}

void MotorControl::enablePwmChannels() const {
    writeRegister(PWM_MAP1_ADDR, PWM_ENABLE1234);
}

void MotorControl::enableSynchronousRectification() const {
    writeRegister(SYNC_RECT_ADDR, ENABLE_FREE_W);
}

void MotorControl::enableNegativeOLD() const {
    writeRegister(OLD_CTRL_3_ADDR, ENABLE_NEGATIVE_OLD);
}

void MotorControl::enableDangerousMode() const {
    writeRegister(OLD_CTRL_2_ADDR, ENABLE_DANGER);
}

void MotorControl::disableOLD() const {
    writeRegister(OLD_CTRL_1_ADDR, DISABLE_OLD);
}

uint8_t MotorControl::getStatus() const {
    return readRegister(IC_STATUS_ADDR);
}

MotorControl::~MotorControl() {
    digitalWrite(NSLEEP_MOTOR, LOW);
    spi3.endTransaction();
}