#include "Arduino.h"
#include "ADS131M04.h"
#define CLOCK_OUT       10
#define SPI_MOSI        11
#define SPI_MISO        12
#define SPI_SCLK        13
#define DRDY_PIN        14

SPIClass spi;
ADS131M04 adc();

void setup(){
    

}