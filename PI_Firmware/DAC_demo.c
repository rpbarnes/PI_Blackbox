#include <math.h>
#include <bcm2835.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>
#include "mA_Cal.h"

#define ONE_MILLISECOND 1000
#define	CS	RPI_GPIO_P1_16	//P4
#define CS_HIGH() bcm2835_gpio_write(CS,HIGH)
#define CS_LOW()  bcm2835_gpio_write(CS,LOW)

static const uint8_t channel_A = 0x30;
static const uint8_t channel_B = 0x34;
static const float VREF = 5.0;

static const float V_FULL_SCALE = 5.0455;		// REQUIRE MEASUREMENT
static const float V_OFFSET = .00146;			// REQUIRE MEASUREMENT

static const int DAC_MAX_COUNT = 65536; // 2^16
double IDEAL_VOLT_QUANTA;
double ACTUAL_VOLT_QUANTA;

void  DelayMicroseconds(uint64_t micros);
void Write_DAC(uint8_t channel, uint16_t Data);
uint16_t Volts_to_Counts(float volts);
int Count_Adjust(int counts);

char Get_Input_Method_From_Serial()
{
    printf("\nEnter:\n\t'i' for milliAmps\t\t'w' for milliWatts	");
    printf("\n>> ");

    int ascii;
    char ch;
    int valid_selection = 0;

    while(!valid_selection)
    {
	        ch = getchar();
          if( ch == 'i' || ch == 'w')
          {
		             printf("received: %c\n", ch);
                 valid_selection = 1;
                 break;
          }
    }
    return ch;
}

double Get_mA_Setpoint_From_Serial()
{
    printf("Enter mA Setpoint> ");
		double mA;
		scanf("%lf", &mA);

	  printf("setting to: %f mA\n", mA);

    return mA;
}

int Get_mW_Setpoint_From_Serial()
{
    printf("Enter mW Setpoint> ");
		double mW;
		scanf("%lf", &mW);

	printf("setting to: %f mW\n\n", mW);

    return mW;
}

void DAC_Set_Count(uint8_t channel, uint16_t Counts)
{
	uint8_t byte[3];	// container for bytes to send
	byte[0] = channel;	// first byte is the DAC channel to set (A or B)
	byte[1] = (Counts>>8);	// send MSB byte first by shifting to right 8 bits, and storing result in 8 bit variable
	byte[2] = (Counts&0x00FF); // sending LSB by AND-ing counts with 0x00FF, which clears the MSB, then store result in 8 bit variable
//printf("Corr1: %f\n", (Counts*72.0*ACTUAL_VOLT_QUANTA)/12970);
	//printf("Should be approximately %f V \n", 1.0003*(Counts*ACTUAL_VOLT_QUANTA+V_OFFSET + (Counts*72*ACTUAL_VOLT_QUANTA)/12940));
 CS_LOW() ;
      bcm2835_spi_transfer(byte[0]);
      bcm2835_spi_transfer(byte[1]);
      bcm2835_spi_transfer(byte[2]);
 CS_HIGH() ;
}

void  DelayMicroseconds(uint64_t micros)
{
	bcm2835_delayMicroseconds (micros);
}

void SPI_Init(void)
{
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   	// Set SPI bus to send the most significant bit first
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);                	// set clock polarity and idle state
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);	// set clock speed
    bcm2835_gpio_fsel(CS, BCM2835_GPIO_FSEL_OUTP);		// declare chip select pin as an output
    bcm2835_gpio_write(CS, HIGH); 				// write chip select high (should always be high unless sending data)
}

uint16_t Volts_to_Counts(float volts)
{
	uint16_t counts;
	double offset_volts = volts - V_OFFSET;
  if( offset_volts < 0) offset_volts = 0;
	counts = (uint16_t)(DAC_MAX_COUNT * offset_volts / V_FULL_SCALE);
	int corrected_counts = Count_Adjust(counts);
   	//printf("Converted %f V to %d counts\n", volts, corrected_counts);

	return corrected_counts;
}

int Count_Adjust(int counts)
{
	double factor  = counts/12970.0;
	int adjust = (int)(factor*72);
	int adjusted_counts = counts-adjust;
	return adjusted_counts;
}


int  main()
{
  IDEAL_VOLT_QUANTA = VREF/((double)DAC_MAX_COUNT);
  ACTUAL_VOLT_QUANTA = (V_FULL_SCALE-V_OFFSET)/((double)DAC_MAX_COUNT);
  uint16_t   i,tmp;
  if (!bcm2835_init())
     return 1;

	SPI_Init();
      printf( "\033[0m\033[2J\033[H");  // clears the terminal and brings cursor to first line
      printf("\n");
			printf("\t\t\tDAC8552 PROGRAMMABLE Current Source");
			printf("\n\t\t\tVolt Quanta(LSB size): %.7f\n ", IDEAL_VOLT_QUANTA);
			printf("\t\t\t( %f uV )\n\n", IDEAL_VOLT_QUANTA*1e6);
		while(1)
		{


			          double MW = Get_mW_Setpoint_From_Serial();
                if(MW == 0)
                {
                    DAC_Set_Count(channel_A,0);
                }
                else
                {
                  double MA = mW_Setting_to_mA(MW);
                  double volts = Calc_mA_to_Volt(MA);
                  uint16_t counts = Volts_to_Counts(volts);
                  delayMicroseconds(200*ONE_MILLISECOND);
  			          DAC_Set_Count(channel_A,counts+1);
                }




			      DelayMicroseconds(1e4);

		}
    bcm2835_spi_end();
    bcm2835_close();

    return 0;
}
