#include <bcm2835.h>
#include "Pin_Defs.h"

#include <stdio.h>
#include <math.h>


//static const uint8_t INA233_HEAT_LOAD_ADDR = 0x45;
//static const uint8_t INA233_CCC_ADDR = 0x40;



#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_BROWN "\033[22;33m"
#define COLOR_GRAY "\033[22;37m"
#define COLOR_LIGHT_RED "\033[01;31m"
#define COLOR_LIGHT_GREEN "\033[01;32m"
#define COLOR_LIGHT_CYAN "\033[01;36m"
#define COLOR_LIGHT_MAGENTA "\033[01;35m"
#define COLOR_LIGHT_BLUE "\033[01;34m"
#define COLOR_RESET "\x1b[0m"


  //  PMBus Commands
#define CLEAR_FAULTS			         (0x03)	 // clears status registers
#define RESTORE_DEFAULTS			     (0x12)	// restores registers to the defaults

#define STATUS_BYTE				         (0x78)  // not very helpful, doesnt give much info
#define STATUS_WORD				         (0x79)  // not helpful
#define STATUS_IOUT				         (0x7B) // not helpful
#define STATUS_INPUT			         (0x7C)  // contains info about overvoltage, undervoltage, and overcurrent
#define STATUS_CML				         (0x7E) // contains info about com errors

#define READ_ENERGY   				     (0x86)	// read energy calculation
#define READ_VBUS				           (0x88)	// read VBUS voltage
#define READ_CURRENT				       (0x89)	// read current measurement (both + and - currents)
#define READ_POWER  				       (0x97)	// read power calculation
#define ADC_CONFIG			           (0xD0)	// configures ADC averaging modes, conversion times, and operating modes
#define READ_VSHUNT			           (0xD1)	// read shunt voltage

#define CALIBRATION_REG			       (0xD4)	//Allows the value of the current-sense resistor calibration value to be input
#define CLEAR_ENERGY_ACCUM				 (0xD6)	//Clears the energy accumulator
#define MFR_ADC_CONFIG             (0xD0) // sets averaging mode, bus conversion time, shunt conversion time, and operating mode
#define STATUS_MFR_SPECIFIC        (0x80)   // reg containging flag for new conversion ready

#define RESERVED_BITS                     (0100000000000000)
#define MODE_CONTINUOUS_VBUS              (0b110)
#define MODE_CONTINUOUS_VSHUNT            (0b101)
#define MODE_CONTINUOUS_VSHUNT_AND_VBUS   (0b111)

#define AVERAGE_ONE_READING     (0b000000000000)
#define AVERAGE_READINGS_4      (0b001000000000)    // 001 << 9
#define AVERAGE_READINGS_16     (0b010000000000)    // 010 << 9
#define AVERAGE_READINGS_64     (0b011000000000)    // 011 << 9
#define AVERAGE_READINGS_128    (0b100000000000)    // 100 << 9

#define VBUS_CONVERSION_140us       (0b000000000)       // 000 << 6
#define VBUS_CONVERSION_204us       (0b001000000)       // 001 << 6
#define VBUS_CONVERSION_332us       (0b010000000)       // 010 << 6
#define VBUS_CONVERSION_588us       (0b011000000)       // 011 << 6
#define VBUS_CONVERSION_1_1ms       (0b100000000)       // 100 << 6   (default)
#define VBUS_CONVERSION_2_116_ms    (0b101000000)       // 101 << 6
#define VBUS_CONVERSION_4_156_ms    (0b110000000)       // 110 << 6
#define VBUS_CONVERSION_8_244_ms    (0b111000000)       // 111 << 6

#define VSHUNT_CONVERSION_140_us       (0b000000)       // 000 << 3
#define VSHUNT_CONVERSION_204_us       (0b001000)       // 001 << 3
#define VSHUNT_CONVERSION_332_us       (0b010000)       // 010 << 3
#define VSHUNT_CONVERSION_588_us       (0b011000)       // 011 << 3
#define VSHUNT_CONVERSION_1_1_ms       (0b100000)       // 100 << 3   (default)
#define VSHUNT_CONVERSION_2_116_ms    (0b101000)       // 101 << 3
#define VSHUNT_CONVERSION_4_156_ms    (0b110000)       // 110 << 3
#define VSHUNT_CONVERSION_8_244_ms    (0b111000)       // 111 << 3

#define MODE_CONTINUOUS_VSHUNT             (0b101)
#define MODE_CONTINUOUS_VBUS               (0b110)
#define MODE_CONTINUOUS_VSHUNT_AND_VBUS    (0b111)     // default

static const float MAX_CURRENT = 3.0;  // 3 Amps
static const float INTERNAL_CONSTANT= .00512;  // page 16 of datasheet
static const float VBUS_LSB = .00125;   // 1.25mV per bit on bus voltage
static const float VSHUNT_LSB = .0000025; // 2.5uV per bit across shunt resistor

static const float HEAT_LOAD_SHUNT = .1;
static const float CCC_SHUNT = .05;

float HEAT_LOAD_POWER_LSB = 0;
float CCC_POWER_LSB = 0;
float HEAT_LOAD_CURRENT_LSB = 0;
uint16_t HEAT_LOAD_CAL_REGISTER_SETTING = 0;
float CCC_CURRENT_LSB = 0;
uint16_t CCC_CAL_REGISTER_SETTING = 0;

uint8_t read_buffer[150];

// Calculates Calibration Settings (but doesn not set them)
void INA233_Init()
{
  if (!bcm2835_i2c_begin())
  {
    printf("bcm2835_i2c_begin failed. Are you running as root??\n");
    return;
  }
  bcm2835_i2c_set_baudrate(BUS_SPEED_100KHZ);
  bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR); // bits 1-7 is the address, bit 0 is read or write bit

}

void INA233_Reg_Write_Byte(uint8_t RegAddr, uint8_t data)
{
  uint8_t send_data[2];
  send_data[0] = RegAddr;
  send_data[1] = data;
  printf("Sending %02X to register %02X\n", RegAddr, data);
  bcm2835_i2c_write(send_data, 2); // send two bytes
}

uint8_t INA233_Reg_Read_Byte(uint8_t RegAddr)
{
  uint8_t reg_addr[2];
  reg_addr[0] = RegAddr;
  bcm2835_i2c_write(reg_addr, 1);                               // Send byte of register address to read
  delayMicroseconds(50);
  bcm2835_i2c_read(read_buffer, 1);                            // ready from RegAddr register and store in read_buffer
  return read_buffer[0];                                       // return data value
}

int INA233_Read_2_Bytes(uint8_t RegAddr)
{
  uint8_t reg_addr[2];
  reg_addr[0] = RegAddr;
  int return_val = 0;
  bcm2835_i2c_write(reg_addr, 1);                               // Send byte of register address to read
  delayMicroseconds(50);
  bcm2835_i2c_read(read_buffer, 2);     // the read is structured so least significant byte is sent first

  return_val = (read_buffer[1] << 8) | return_val; // Most significant byte
  return_val = (read_buffer[0]) | return_val; // Least significant byte

  return return_val;   // return data value
}

void INA233_Write_2_Bytes(uint8_t RegAddr, uint16_t value)
{
    uint8_t write_data[3];
    write_data[0] = RegAddr;
    write_data[1] = (value & 0x00FF);
    write_data[2] = ((value >> 8) & 0x00FF);

    bcm2835_i2c_write(write_data, 3);
}

// Sets Calibration register parameters for both motr load and heat load
void INA233_Set_Calibration()
{
    CCC_CURRENT_LSB = MAX_CURRENT/pow(2,15);    // MAX_CURRENT / 2^15   (p16 in datasheet)
    CCC_POWER_LSB = 25*CCC_CURRENT_LSB; // p16 in datasheet
    CCC_CAL_REGISTER_SETTING = (uint16_t)(INTERNAL_CONSTANT / (CCC_CURRENT_LSB * CCC_SHUNT));

    HEAT_LOAD_CURRENT_LSB = MAX_CURRENT/pow(2,15);    // MAX_CURRENT / 2^15   (p16 in datasheet)
    HEAT_LOAD_POWER_LSB = 25*CCC_CURRENT_LSB; // p16 in datasheet
    HEAT_LOAD_CAL_REGISTER_SETTING = (uint16_t)(INTERNAL_CONSTANT / (HEAT_LOAD_CURRENT_LSB * HEAT_LOAD_SHUNT));

    bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR);
    INA233_Write_2_Bytes(CALIBRATION_REG, HEAT_LOAD_CAL_REGISTER_SETTING);
    delayMicroseconds(10);

    //printf("READING CAL SETTING: ....\t");
    //int setting = INA233_Read_2_Bytes(CALIBRATION_REG);
    //printf(" %d\n", setting);

    return;
}

void INA233_Set_Config_Register(uint8_t operating_mode, uint8_t shunt_time, uint8_t bus_time, uint16_t averaging_mode)
{
    uint8_t write_data[3];
    uint16_t send_data = operating_mode | shunt_time | bus_time | averaging_mode | RESERVED_BITS;
    write_data[0] = MFR_ADC_CONFIG;

    // split send data into two data bytes
    write_data[2] = ((send_data >> 8));
    write_data[1] = (send_data & 0x00FF);

    //printf("SENDING: ....\tLSB:");
    //print_byte(write_data[1]);
    //printf(", MSB: ");
    //print_byte(write_data[2]);
    //printf("\n");
    bcm2835_i2c_write(write_data, 3);

    return;
}

bool INA233_New_Conversion_Ready()
{
    uint8_t status_byte = 0;
    status_byte = INA233_Reg_Read_Byte(STATUS_MFR_SPECIFIC);

    uint8_t CMD[2] = {CLEAR_FAULTS,0};
    if((status_byte & 0b10000000))  // a 0 is in bit position 7 if a new reading is ready
    {
        bcm2835_i2c_write(CMD, 1); // clears the flag for next read
        return true;
    }
    else
    {
      //printf(".");
      return false;
    }
}

float INA233_Read_Heat_Load_Volts()
{
  bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR);
  while(!INA233_New_Conversion_Ready()){ delayMicroseconds(1); }
  int16_t count;
  count = INA233_Read_2_Bytes(READ_VBUS);   // this is returned as two's complement, so will have leading 1 if negative number
  float vbus = ((float)count)*VBUS_LSB;

  return vbus;
}

float INA233_Read_Heat_Load_Shunt_V()
{
  bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR);
  while(!INA233_New_Conversion_Ready()){ delayMicroseconds(1); }
  int16_t count;
  count = INA233_Read_2_Bytes(READ_VSHUNT);   // this is returned as two's complement, so will have leading 1 if negative number
  float vshunt = ((float)count)*VSHUNT_LSB;

  return vshunt;
}

float INA233_Read_Heat_Load_Amps()
{
  bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR);
  while(!INA233_New_Conversion_Ready()){ delayMicroseconds(1); }
  int16_t count;
  count = INA233_Read_2_Bytes(READ_CURRENT);
  float amps = ((float)count)*HEAT_LOAD_CURRENT_LSB;

  return amps;
}

float INA233_Read_Heat_Load_Watts()
{
  bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR);
  while(!INA233_New_Conversion_Ready()){ delayMicroseconds(1); }
  int16_t count;
  count = INA233_Read_2_Bytes(READ_POWER);
  float watts = ((float)count)*HEAT_LOAD_POWER_LSB;

  return watts;
}
