#include <bcm2835.h>
#include "Pin_Defs.h"

#include <stdio.h>
#include <stdlib.h>

// Pin Numbering
/*
Rasperry Pi Physical Pin Numbering w/ bcm2835 library pin Numbering
            BCM2835           Raspberry Pi
Function     pin#             physical pin#
SDA1          2                   3       INA233_I2C
SCL1          3                   5       INA233_I2C
GPCLK0        4                   7                         OPEN
GPCLK1        5                   29      MAX31856_DRDY1
GPCLK2        6                   31                        OPEN
SPI0_CE1_N    7                   26      MAX31856_CS3
SPI0_CE0_N    8                   24      MAX31856_CS2
SPI0_MISO     9                   21      ADS1256_MISO
SPI0_MOSI     10                  19      ADS1256 MOSI
SPI0_SCLK     11                  23      ADS1256_SCLK
PWM0          12                  32                        OPEN
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
                                  28      RELAY_OUT
*/

#define MAX31856_CHIP_SELECT_1   25   /* physical pin 22 */
#define MAX31856_CHIP_SELECT_2   8    /* pysical pin 24 */
#define MAX31856_CHIP_SELECT_3   7    /* physical pin 26 */
#define MAX31856_DRDY_1   5           /* physical pin 29 */
#define MAX31856_DRDY_2   16          /* physical pin 36 */
#define MAX31856_DRDY_3   26          /* physical pin 37 */

#define ONE_MILLISECOND 1000
#define TEMP_LSB 0.0078125


#define MAX31856_CONFIG_REG0                0x00    ///< Config 0 register
#define MAX31856_CONFIG_REG0_AUTOCONVERT    0x80    ///< Config 0 Auto convert flag
#define MAX31856_CONFIG_REG0_1SHOT          0x40    ///< Config 0 one shot convert flag
#define MAX31856_CONFIG_REG0_OCFAULT1       0x20    ///< Config 0 open circuit fault 1 flag
#define MAX31856_CONFIG_REG0_OCFAULT0       0x10    ///< Config 0 open circuit fault 0 flag
#define MAX31856_CONFIG_REG0_FAULT          0x04    ///< Config 0 fault mode flag
#define MAX31856_CONFIG_REG0_FAULTCLR       0x02    ///< Config 0 fault clear flag

#define MAX31856_CONFIG_REG1                0x01    ///< Config 1 register
#define MAX31856_MASK_REG                   0x02    ///< Fault Mask register
#define MAX31856_CJHF_REG                   0x03    ///< Cold junction High temp fault register
#define MAX31856_CJLF_REG                   0x04    ///< Cold junction Low temp fault register
#define MAX31856_LTHFTH_REG                 0x05    ///< Linearized Temperature High Fault Threshold Register, MSB
#define MAX31856_LTHFTL_REG                 0x06    ///< Linearized Temperature High Fault Threshold Register, LSB
#define MAX31856_LTLFTH_REG                 0x07    ///< Linearized Temperature Low Fault Threshold Register, MSB
#define MAX31856_LTLFTL_REG                 0x08    ///< Linearized Temperature Low Fault Threshold Register, LSB
#define MAX31856_CJ_TEMP_OFFSET_REG         0x09    ///< Cold-Junction Temperature Offset Register
#define MAX31856_CJTH_REG                   0x0A    ///< Cold-Junction Temperature Register, MSB
#define MAX31856_CJTL_REG                   0x0B    ///< Cold-Junction Temperature Register, LSB
#define MAX31856_LTCBH_REG                  0x0C    ///< Linearized TC Temperature, Byte 2
#define MAX31856_LTCBM_REG                  0x0D    ///< Linearized TC Temperature, Byte 1
#define MAX31856_LTCBL_REG                  0x0E    ///< Linearized TC Temperature, Byte 0
#define MAX31856_SR_REG                     0x0F    ///< Fault Status Register

#define MAX31856_FAULT_CJRANGE     0x80    ///< Fault status Cold Junction Out-of-Range flag
#define MAX31856_FAULT_TCRANGE     0x40    ///< Fault status Thermocouple Out-of-Range flag
#define MAX31856_FAULT_CJHIGH      0x20    ///< Fault status Cold-Junction High Fault flag
#define MAX31856_FAULT_CJLOW       0x10    ///< Fault status Cold-Junction Low Fault flag
#define MAX31856_FAULT_TCHIGH      0x08    ///< Fault status Thermocouple Temperature High Fault flag
#define MAX31856_FAULT_TCLOW       0x04    ///< Fault status Thermocouple Temperature Low Fault flag
#define MAX31856_FAULT_OVUV        0x02    ///< Fault status Overvoltage or Undervoltage Input Fault flag
#define MAX31856_FAULT_OPEN        0x01    ///< Fault status Thermocouple Open-Circuit Fault flag

#define MAX31856_READ              0x7F
#define MAX31856_WRITE             0x80

typedef enum
{
  TYPE_B  = 0b0000,
  TYPE_E  = 0b0001,
  TYPE_J  = 0b0010,
  TYPE_K  = 0b0011,
  TYPE_N  = 0b0100,
  TYPE_R  = 0b0101,
  TYPE_S  = 0b0110,
  TYPE_T  = 0b0111,
} max31856_thermocoupletype_t;

typedef enum
{
  ONE_SAMPLE    = 0b0000000,
  TWO_SAMPLES   = 0b0010000,
  FOUR_SAMPLES  = 0b0100000,
  EIGHT_SAMPLES = 0b0110000,
} max31856_num_samples_averaged_t;


void MAX31856_Begin(void);
void Set_Thrmcpl_Type(uint8_t chip_select, max31856_thermocoupletype_t type);
uint8_t Read_Fault(uint8_t chip_select);
void Set_Noise_Filter_60Hz(uint8_t chip_select);
void Set_One_Shot_Measurement(uint8_t chip_select);
float Read_CJ_Temp(uint8_t chip_select);
float Read_Thrmcpl_Temp(uint8_t chip_select);
uint8_t Read_Reg_8(uint8_t chip_select, uint8_t register_addr);
uint16_t Read_Reg_16(uint8_t chip_select, uint8_t register_addr);
uint32_t Read_Reg_24(uint8_t chip_select, uint8_t register_addr);
void Read_Reg_N(uint8_t chip_select, uint8_t register_addr, uint8_t buffer[], uint8_t n);
void Write_Reg_Byte(uint8_t chip_select, uint8_t register_addr, uint8_t data);
void Set_Read_Continous(uint8_t chip_select);
void Set_Samples_Averaged(uint8_t chip_select, max31856_num_samples_averaged_t num);
void MAX31856_WaitDRDY(uint8_t DRDY);

void MAX31856_Begin(void) {

  
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_512);
  bcm2835_gpio_fsel(MAX31856_CHIP_SELECT_1, BCM2835_GPIO_FSEL_OUTP);		// Set Chip Select as output pin
  bcm2835_gpio_fsel(MAX31856_CHIP_SELECT_2, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(MAX31856_CHIP_SELECT_3, BCM2835_GPIO_FSEL_OUTP);

  bcm2835_gpio_write(MAX31856_CHIP_SELECT_1, HIGH);											// Write Chip Select High
  bcm2835_gpio_write(MAX31856_CHIP_SELECT_2, HIGH);
  bcm2835_gpio_write(MAX31856_CHIP_SELECT_3, HIGH);

  bcm2835_gpio_fsel(MAX31856_DRDY_1, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_fsel(MAX31856_DRDY_2, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_fsel(MAX31856_DRDY_3, BCM2835_GPIO_FSEL_INPT);

  bcm2835_gpio_set_pud(MAX31856_DRDY_1, HIGH);											// Write DRDY High
  bcm2835_gpio_set_pud(MAX31856_DRDY_2, HIGH);
  bcm2835_gpio_set_pud(MAX31856_DRDY_3, HIGH);

  Write_Reg_Byte(MAX31856_CHIP_SELECT_1, MAX31856_MASK_REG, 0x0);
  delayMicroseconds(5);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_1, MAX31856_CONFIG_REG0, MAX31856_CONFIG_REG0_OCFAULT0); // assert on any fault
  delayMicroseconds(5);
  Set_Thrmcpl_Type(MAX31856_CHIP_SELECT_1, TYPE_K);
  delayMicroseconds(5);
  Set_Noise_Filter_60Hz(MAX31856_CHIP_SELECT_1);
  delayMicroseconds(5);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_1, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
  delayMicroseconds(5);
  Set_Samples_Averaged(MAX31856_CHIP_SELECT_1, TWO_SAMPLES);
  delayMicroseconds(5);
  Set_Read_Continous(MAX31856_CHIP_SELECT_1);

  Write_Reg_Byte(MAX31856_CHIP_SELECT_2, MAX31856_MASK_REG, 0x0);
  delayMicroseconds(5);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_2, MAX31856_CONFIG_REG0, MAX31856_CONFIG_REG0_OCFAULT0); // assert on any fault
  delayMicroseconds(5);
  Set_Thrmcpl_Type(MAX31856_CHIP_SELECT_2, TYPE_K);
  Set_Noise_Filter_60Hz(MAX31856_CHIP_SELECT_2);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_2, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
  Set_Samples_Averaged(MAX31856_CHIP_SELECT_2, TWO_SAMPLES);
  Set_Read_Continous(MAX31856_CHIP_SELECT_2);

  Write_Reg_Byte(MAX31856_CHIP_SELECT_3, MAX31856_MASK_REG, 0x0);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_3, MAX31856_CONFIG_REG0, MAX31856_CONFIG_REG0_OCFAULT0); // assert on any fault
  Set_Thrmcpl_Type(MAX31856_CHIP_SELECT_3, TYPE_K);
  Set_Noise_Filter_60Hz(MAX31856_CHIP_SELECT_3);
  Write_Reg_Byte(MAX31856_CHIP_SELECT_3, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
  Set_Samples_Averaged(MAX31856_CHIP_SELECT_3, TWO_SAMPLES);
  Set_Read_Continous(MAX31856_CHIP_SELECT_3);

  return;
}

void MAX31856_Config_SPI()
{
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_512);
}

  // Set type (K, J, T, etc)
void Set_Thrmcpl_Type(uint8_t chip_select, max31856_thermocoupletype_t type)
{
  uint8_t t = Read_Reg_8(chip_select, MAX31856_CONFIG_REG1);
  t &= 0xF0; // mask off bottom 4 bits
  t |= (uint8_t)type & 0x0F;
  Write_Reg_Byte(chip_select, MAX31856_CONFIG_REG1, t);
}

void Set_Samples_Averaged(uint8_t chip_select, max31856_num_samples_averaged_t num)
{
  uint8_t reg_value = Read_Reg_8(chip_select, MAX31856_CONFIG_REG1);
  reg_value &= 0x0F; // mask off top 4 bits
  reg_value |= (uint8_t)num & 0xF0;
  Write_Reg_Byte(chip_select, MAX31856_CONFIG_REG1, reg_value);
}

uint8_t Read_Fault(uint8_t chip_select)
{
  return Read_Reg_8(chip_select, MAX31856_SR_REG);
}

void Set_Noise_Filter_60Hz(uint8_t chip_select)
{
  uint8_t t = Read_Reg_8(chip_select, MAX31856_CONFIG_REG0);
  t&=0xFE;
  Write_Reg_Byte(chip_select, MAX31856_CONFIG_REG0, t);
}

 // Begin a one-shot measurement, value not returned here
 // takes 250 ms for a measurement
void Set_One_Shot_Measurement(uint8_t chip_select)
{
  Write_Reg_Byte(chip_select, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
  uint8_t t = Read_Reg_8(chip_select, MAX31856_CONFIG_REG0);

  t &= ~MAX31856_CONFIG_REG0_AUTOCONVERT; // turn off autoconvert!
  t |= MAX31856_CONFIG_REG0_1SHOT;

  Write_Reg_Byte(chip_select, MAX31856_CONFIG_REG0, t);

  bcm2835_delayMicroseconds(ONE_MILLISECOND);
}

void Set_Read_Continous(uint8_t chip_select)
{
    Write_Reg_Byte(chip_select, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
    uint8_t reg_data = Read_Reg_8(chip_select, MAX31856_CONFIG_REG0);

    reg_data |= MAX31856_CONFIG_REG0_AUTOCONVERT;   // turn on continous conversion
    Write_Reg_Byte(chip_select, MAX31856_CONFIG_REG0, reg_data);
}

// Start a one-shot measurement and return internal chip temperature
float Read_CJ_Temp(uint8_t chip_select)
{
  //Write_Reg_Byte(chip_select, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);
  //Set_One_Shot_Measurement(chip_select);
  Set_Read_Continous(chip_select);
  uint8_t DRDY = MAX31856_DRDY_1;
  if(chip_select == MAX31856_CHIP_SELECT_1) DRDY = MAX31856_DRDY_1;
  else if(chip_select == MAX31856_CHIP_SELECT_2) DRDY = MAX31856_DRDY_2;
  else if(chip_select == MAX31856_CHIP_SELECT_3) DRDY = MAX31856_DRDY_3;

  if(!bcm2835_gpio_lev(DRDY)==0){ MAX31856_WaitDRDY(DRDY);  }

  int16_t count16 = Read_Reg_16(chip_select, MAX31856_CJTH_REG);
  float temperature = count16;
  temperature /= 256.0;

  return temperature;
}

float Read_Thrmcpl_Temp(uint8_t chip_select)
{
  //Write_Reg_Byte(chip_select, MAX31856_CJ_TEMP_OFFSET_REG, 0x0);    // Writes 0 to the temp offset register
  //Set_One_Shot_Measurement(chip_select);
  Set_Read_Continous(chip_select);
  uint8_t DRDY = MAX31856_DRDY_1;
  if(chip_select == MAX31856_CHIP_SELECT_1) DRDY = MAX31856_DRDY_1;
  else if(chip_select == MAX31856_CHIP_SELECT_2) DRDY = MAX31856_DRDY_2;
  else if(chip_select == MAX31856_CHIP_SELECT_3) DRDY = MAX31856_DRDY_3;

  if(!bcm2835_gpio_lev(DRDY)==0){ MAX31856_WaitDRDY(DRDY);  }

  int32_t count24 =0;
  count24 = Read_Reg_24(chip_select, MAX31856_LTCBH_REG); // gets 19 bit temp value
  if (count24 & 0x800000) {
    count24 |= 0xFF000000;  // fix sign
  }

  count24 >>= 5;  // bottom 5 bits are unused  (only 19 bit return value)

  float temperature = count24;
  temperature *= TEMP_LSB;
  //printf("%f\n", temperature);

  return temperature;
}

void MAX31856_WaitDRDY(uint8_t DRDY)
{
	uint32_t i;
	for (i = 0; i < 400000; i++)
	{
		delayMicroseconds(1);
		if ( bcm2835_gpio_lev(DRDY)==0 ){ break; }
	}
	if (i >= 400000){ printf("MAX31856_WaitDRDY() Time Out ...DRDY pin%d\r\n", DRDY); }
}

uint8_t Read_Reg_8(uint8_t chip_select, uint8_t register_addr)
{
  uint8_t data = 0;
  Read_Reg_N(chip_select, register_addr, &data, 1);

  return data;
}

uint16_t Read_Reg_16(uint8_t chip_select, uint8_t register_addr)
{
  uint8_t buffer[2] = {0, 0};
  Read_Reg_N(chip_select, register_addr, buffer, 2);

  uint16_t count = buffer[0];
  count <<= 8;
  count |=  buffer[1];

  return count;
}

// Starts at high byte register, reads 3 registers and returns composite value
uint32_t Read_Reg_24(uint8_t chip_select, uint8_t register_addr)
{
  uint8_t values[3] = {0, 0, 0};    // use to store 3 bytes read
  Read_Reg_N(chip_select, register_addr, values, 3);

  uint32_t count = values[0];  // value[0] is MSB
  count <<= 8;
  count |=  values[1];
  count <<= 8;
  count |=  values[2];  // value[2] is LSB

  return count;
}


void Read_Reg_N(uint8_t chip_select, uint8_t register_addr, uint8_t buffer[], uint8_t n)
{
  register_addr &= MAX31856_READ; // MSB must be a 0 in address byte for read operations

  bcm2835_delayMicroseconds(1);
  bcm2835_gpio_write(chip_select,LOW);
  bcm2835_delayMicroseconds(5);
  bcm2835_spi_transfer(register_addr);

  while (n--) {
    bcm2835_delayMicroseconds(1);
    buffer[0] = bcm2835_spi_transfer(0xFF);
    buffer++;   // next address in the array
  }
  bcm2835_delayMicroseconds(1);
  bcm2835_gpio_write(chip_select,HIGH);
}


void Write_Reg_Byte(uint8_t chip_select, uint8_t register_addr, uint8_t data)
{
  register_addr |= MAX31856_WRITE; // MSB must be a 1 for write operations

	bcm2835_delayMicroseconds(1);
  bcm2835_gpio_write(chip_select,LOW);
  bcm2835_delayMicroseconds(1);
  bcm2835_spi_transfer(register_addr);
  bcm2835_delayMicroseconds(1);
	bcm2835_spi_transfer(data);
  bcm2835_delayMicroseconds(1);
  bcm2835_gpio_write(chip_select,HIGH);
}
