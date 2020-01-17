

#include <bcm2835.h>

#ifndef _PIN_DEFINITIONS_
#define _PIN_DEFINITIONS_

// Pin Numbering
/*
Rasperry Pi Physical Pin Numbering w/ bcm2835 library pin Numbering
            BCM2835           Raspberry Pi
Function     pin#             physical pin#
SDA1          2                   3       INA233_I2C
SCL1          3                   5       INA233_I2C
GPCLK0        4                   7                         OPEN
GPCLK1        5                   29      MAX31856_DRDY1
GPCLK2        6                   31      SW_CTRL_100uA                  
SPI0_CE1_N    7                   26      MAX31856_CS3
SPI0_CE0_N    8                   24      MAX31856_CS2
SPI0_MISO     9                   21      ADS1256_MISO
SPI0_MOSI     10                  19      ADS1256 MOSI
SPI0_SCLK     11                  23      ADS1256_SCLK
PWM0          12                  32      RELAY_RESET
PWM1          13                  33                        OPEN
TXD0          14                  8       CCC_UART_TX
RXD0          15                  10      CCC_UART_RX
SPI1_CE2      16                  36      MAX31856_DRDY2
SPI1_CE1      17                  11      ADS1256_DRDY
PCM_CLK       18                  12      ADS1256_RESET
SPI1_MISO     19                  35      RESERVED_SPI1
SPI1_MOSI     20                  38      RESERVED_SPI1
SPI1_SCLK     21                  40      RESERVED_SPI1
SD0_CLK       22                  15      ADS1256_CS
SD0_CMD       23                  16      DAC8532_CS
SD0_DAT0      24                  18                        OPEN
SD0_DAT1      25                  22      MAX31856_CS1
SD0_DAT2      26                  37      MAX31856_DRDY3
SD0_DAT3      27                  13      ADS1256_SYNC
              01                  28      RELAY_SET
*/


#define ADS1256_DRDY  17            // physical pin 11
#define ADS1256_RST  18             // physical pin 12
#define ADS1256_CS  22              // Physical pin 15
#define	DAC8532_CS	RPI_GPIO_P1_16	// Physical pin 16
#define MAX31856_CHIP_SELECT_1   25
#define MAX31856_CHIP_SELECT_2   8
#define MAX31856_CHIP_SELECT_3   7
#define MAX31856_DRDY_1   5
#define MAX31856_DRDY_2   16
#define MAX31856_DRDY_3   26


static const uint8_t INA233_HEAT_LOAD_ADDR = 0x45;
//static const uint8_t INA233_CCC_ADDR = 0x40;

#endif
