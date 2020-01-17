#define _USE_XOPEN 700
//#define _USE_XOPEN_SOURCE
#define _GNU_SOURCE
#include <time.h>
#include <bcm2835.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "ADS1256.h"
#include "DMM_CAL.h"
#include "Save2.h"
#include "INA.h"
#include "MAX31856.h"
#include "temp_diode.h"
#include "Display.c"

// ALERT
// When debugging SPI bus, make sure SCLK is set as edge select falling on scope

static const int NUM_VOLT_SAMPLES = 3;
static const int NUM_AMP_SAMPLES = 5;
static const int NUM_DIODE_SAMPLES = 2;
static const double POWER_FACTOR =0.807;
static const double AMP_OFFSET = .0045; // value read when relay is open

static const uint8_t INA233_CONVERSION_TIME = (0b100000);

static const int PULSE_ON = 50*ONE_MILLISECOND;


#define ONE_MINUTE				60		// 60 seconds
#define FIFTEEN_MINUTES  	15*ONE_MINUTE
#define THIRTY_MINUTES		30*ONE_MINUTE
#define ONE_MILLISECOND 1000
#define	CS	RPI_GPIO_P1_16	//P4

static const uint8_t channel_A = 0x30;
static const uint8_t channel_B = 0x34;
static const float VREF = 5.0;

static const float FULL_SCALE = 5.0455;		// REQUIRE MEASUREMENT
static const float V_OFFSET = .00146;			// REQUIRE MEASUREMENT

static const int DAC_MAX_COUNT = 65536; // 2^16
double IDEAL_VOLT_QUANTA;
double ACTUAL_VOLT_QUANTA;

int Load_Index = 0;
//corresponds with mW of: 100, 125, 150, 175, 200, 225, 250, 275, 300, 350, 400
double Load_Points[] = {0.1, .111, .123, .134, .145, .168, .187, .2, .216, .245, .284};

void Relay_ON()
{
	//printf("--RELAY ON--\n");
	bcm2835_gpio_write(RELAY_RESET, LOW); 		// verifies other latching coil isnt being driven
  delayMicroseconds(100);
	//printf("\tRELAY SET HIGH [pin 28]");
	bcm2835_gpio_write(RELAY_SET, HIGH);
  //printf("changed pin %d to HI\n", RELAY_SET);
	//printf("\t ....\n");
	delayMicroseconds(PULSE_ON);
	bcm2835_gpio_write(RELAY_SET, LOW);
  Record_Relay_ON();
	//printf("\t[pin 28 HI, pin 32 LOW].... relay should be on\n");
}

void Relay_OFF()
{
	//printf("--RELAY OFF--\n");
  delayMicroseconds(10);
	bcm2835_gpio_write(RELAY_SET, LOW); 		// verifies other latching coil isnt being driven
  delayMicroseconds(100);
	//printf("\tRELAY RESET HIGH [pin 32]");
	bcm2835_gpio_write(RELAY_RESET, HIGH);
  //printf("changed pin %d to HI\n", RELAY_RESET);
	//printf("\t ....\n");
	delayMicroseconds(PULSE_ON);
	bcm2835_gpio_write(RELAY_RESET, LOW);
	//printf("\t[pin 28 LOW, pin 32 HI]\n");
}

void DAC_Set_Count(uint8_t channel, uint16_t Counts)
{
delayMicroseconds(10);
  //printf("set count to %d\n", Counts);
	uint8_t byte[3];	// container for bytes to send
	byte[0] = channel;	// first byte is the DAC channel to set (A or B)
	byte[1] = (Counts>>8);	// send MSB byte first by shifting to right 8 bits, and storing result in 8 bit variable
	byte[2] = (Counts&0x00FF); // sending LSB by AND-ing counts with 0x00FF, which clears the MSB, then store result in 8 bit variable

 DAC_CS_LOW() ;
      bcm2835_spi_transfer(byte[0]);
      delayMicroseconds(1);
      bcm2835_spi_transfer(byte[1]);
        delayMicroseconds(1);
      bcm2835_spi_transfer(byte[2]);
 DAC_CS_HIGH() ;
 delayMicroseconds(10);
}

void  DelayMicroseconds(uint64_t micros)
{
	bcm2835_delayMicroseconds (micros);
}

void Take_Current_Measurement()
{
      ADS1256_WaitDRDY();
      ADS1256_ReadData();
      delayMicroseconds(10); // reading any sooner after changing MUX seems to screw up first measurement
      for(int i=0; i<NUM_AMP_SAMPLES; i++)
      {
          ADS1256_WaitDRDY();
          ADS1256_Sync();
          delayMicroseconds(1);
          Amp_Metrics[index_now.amp_sample].data_raw = ( 0.4 * Counts_to_Volts( ADS1256_ReadData() ) ) - AMP_OFFSET;
          Amp_Metrics[index_now.amp_sample].data_corrected = get_Corrected_Amp( Amp_Metrics[index_now.amp_sample].data_raw  * POWER_FACTOR) ;
          ++index_now.amp_sample;
      }
}

void Take_Volt_Measurement()
{
      ADS1256_WaitDRDY();
      ADS1256_ReadData();
      delayMicroseconds(10);
      for(int i=0; i<NUM_VOLT_SAMPLES; i++)
      {
          ADS1256_WaitDRDY();
          ADS1256_Sync();
          delayMicroseconds(1);
          Volt_Metrics[index_now.volt_sample].data_raw = 4*Counts_to_Volts( ADS1256_ReadData() );
          //printf("Raw V: %f\n", Volt_Metrics[index_now.volt_sample].data_raw );
          Volt_Metrics[index_now.volt_sample].data_corrected = get_Corrected_Volt( Volt_Metrics[index_now.volt_sample].data_raw  );
          ++index_now.volt_sample;
      }
}

void Take_Diode_Measurement()
{
    ADS1256_WaitDRDY();
    ADS1256_ReadData();
    delayMicroseconds(10);
    for(int i=0; i<NUM_DIODE_SAMPLES; i++)
    {
      ADS1256_WaitDRDY();
      ADS1256_Sync();
      delayMicroseconds(1);
      Diode_Metrics[index_now.diode_sample].data_raw = Counts_to_Volts( ADS1256_ReadData() );
      Diode_Metrics[index_now.diode_sample].data_corrected = get_Temp_K( Diode_Metrics[index_now.diode_sample].data_raw );
      ++index_now.diode_sample;
    }
}

void Read_Thermocouples()
{
  Read_CJ_Temp(MAX31856_CHIP_SELECT_1);
  Save_Data[save_index].thermocouple1_temp = Read_Thrmcpl_Temp(MAX31856_CHIP_SELECT_1);

  Read_CJ_Temp(MAX31856_CHIP_SELECT_2);
  Save_Data[save_index].thermocouple2_temp= Read_Thrmcpl_Temp(MAX31856_CHIP_SELECT_2);

  Read_CJ_Temp(MAX31856_CHIP_SELECT_3);
  Save_Data[save_index].thermocouple3_temp= Read_Thrmcpl_Temp(MAX31856_CHIP_SELECT_3);
	delayMicroseconds(20);
}

void Measure_Heat_Load()
{
  Set_I2C_Address(INA233_HEAT_LOAD_ADDR);
  Save_Data[save_index].Heat_Load_mA = 1000*INA233_Read_Heat_Load_Amps();
  Save_Data[save_index].Heat_Load_V = INA233_Read_Heat_Load_Volts();
  Save_Data[save_index].Heat_Load_mW = 1000*INA233_Read_Heat_Load_Watts();
}

int Count_Adjust(int counts)
{
	double factor  = counts/12970.0;
	int adjust = (int)(factor*72);
	int adjusted_counts = counts-adjust;
	return adjusted_counts;
}

uint16_t Volts_to_Counts(float volts)
{
	uint16_t counts;
	double offset_volts = volts - V_OFFSET;
  if( offset_volts < 0) offset_volts = 0;
	counts = (uint16_t)(DAC_MAX_COUNT * offset_volts / FULL_SCALE);
	int corrected_counts = Count_Adjust(counts);
   	//printf("Converted %f V to %d counts\n", volts, corrected_counts);

	return corrected_counts;
}

void Set_Next_Load_Point()
{
    delayMicroseconds(20);
    DAC_Set_Count(channel_A, 0);
    if(Load_Index<7)
    {
      double Load_Point = Load_Points[Load_Index];
      uint16_t counts = Volts_to_Counts(Load_Point);
      delayMicroseconds(100);
      DAC_Set_Count(channel_A,counts+1);
      delayMicroseconds(20);
    }

    ++Load_Index;
    if(Load_Index >= 7)
    {
      //Relay_OFF();
      Calc_Averages();
      Display_Data();
      Write_Data_To_Influx();
      Write_Data_To_File();
			Load_Index = 0;
      //bcm2835_close();
      //exit(0);
    }
}

void main()
{
  delayMicroseconds(10);
  ADS1256_Init();

  INA233_Init();
  INA233_Set_Calibration();
  INA233_Set_Config_Register(MODE_CONTINUOUS_VSHUNT_AND_VBUS, INA233_CONVERSION_TIME, INA233_CONVERSION_TIME, AVERAGE_READINGS_16);

  ADS1256_Config(ADS1256_GAIN_1, DRATE_15SPS);
  ADS1256_ReadData(); // take a reading to initialize ADC

  delayMicroseconds(100);
  MAX31856_Begin();

	//Initialize_Log("");

	Calibrate_Gain_and_Offset();
  bcm2835_delayMicroseconds(50);

  Clear_Console();
  Print_Console_Header();

  ADS1256_Change_MUX(VOLT_DIFF_CHANNEL);
  ADS1256_Sync();
  ADS1256_ReadData();
  bcm2835_delayMicroseconds(50);


  struct timespec recipe_start_time;
  time_t recipe_start, time_now, reset_start;

  clock_gettime(CLOCK_REALTIME, &recipe_start_time);
  long recipe_start_time_secs = recipe_start_time.tv_sec;
  time(&recipe_start);
	time(&reset_start);
	char buffer[50];

//  Relay_ON();
  int cal_count = 0;
  while(1)
  {
    // MAIN LOOP
        ++cal_count;
        if(cal_count >= 240 )   // Do a calibration every 120 loop iterations. 1 loop iteration takes 1 sec approx. So every 2 minutes
        {
            Calibrate_Gain_and_Offset();    // doing a calibration of gain and offset takes approx. 200 ms. Use sparingly
            cal_count = 0;
        }
      time(&time_now);
      long long elapsed_secs = difftime(time_now, reset_start);

      if(elapsed_secs >= THIRTY_MINUTES)
      {
					printf("time reached setpoint\n");

					Seconds_To_Elapsed_Time_String(elapsed_secs, buffer);
					printf("\tInterval:\t%s\n", buffer);

					elapsed_secs = difftime(time_now, recipe_start);
					Seconds_To_Elapsed_Time_String(elapsed_secs, buffer);
					printf("\tTotal Elapsed:\t%s\n", buffer);
          //Set_Next_Load_Point();
          time(&reset_start); // reset the timer
      }
/*
      Loop_TimeStamp(DATA_START);

      Take_Volt_Measurement();
      ADS1256_Change_MUX(CURRENT_DIFF_CHANNEL); // Changing Mux for next measurement Current Differential Channel
      ADS1256_Sync();

      Take_Current_Measurement();
      ADS1256_Change_MUX(TEMP_DIODE_DIFF_CHANNEL);
      ADS1256_Sync();

      Read_Thermocouples();

      Take_Diode_Measurement();
      ADS1256_Change_MUX(CURRENT_DIFF_CHANNEL);
      ADS1256_Sync();

      Take_Current_Measurement();
      ADS1256_Change_MUX(VOLT_DIFF_CHANNEL);
      ADS1256_Sync();
      ADS1256_ReadData();

      Measure_Heat_Load();

      Calc_Averages();
      Display_Data();

      ++save_index;
      if(save_index >= 5)
      {
          Write_Data_To_Influx();
          Write_Data_To_File();
          //Clear_Console();
          //Print_Console_Header();
      }
      printf("\n");
      printf("\33[%dA", (int)1); // brings cursor up one line to over-write previous values on console
			*/
  }
  bcm2835_close();
}

/*




#define VREF (5.000)     // reference voltage
#define SAMPLES (50)   // how many samples to group together for stats


unsigned int cnt;    // simple loop counter

void loop() {  // **** main loop

double datSum = 0;  // reset our accumulated sum of input values to zero
double sMax;
double sMin;
long n;            // count of how many readings so far
double x,mean,delta,sumsq,m2,variance,stdev;  // to calculate standard deviation

 sMax = -VREF;  // set max to minimum possible reading
 sMin = 2*VREF;   // set min to max possible reading
 sumsq = 0; // initialize running squared sum of differences
 n = 0;     // have not made any ADC readings yet
 mean = 0; // start off with running mean at zero
 m2 = 0;

 for (int i=0;i<SAMPLES;i++) {
   x = readWord() * VREF / (1<<28);  // analog reading in units of Volts
   datSum += x;
   if (x > sMax) sMax = x;
   if (x < sMin) sMin = x;
                     // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
   n++;
   delta = x - mean;
   mean += delta/n;
   m2 += (delta * (x - mean));
 }
 variance = m2/(n-1);  // (n-1):Sample Variance  (n): Population Variance
 stdev = 1E6*sqrt(variance);  // Calculate standard deviation in microvolts

 float tsec = millis()/1000.0;  // elapsed time in seconds
 Serial.print(cnt++);
 Serial.print(",");
 Serial.print(tsec,2);      // elapsed time, seconds
 Serial.print(",");
 Serial.print(mean,7);      // average reading, volts
 Serial.print(",");
 Serial.print(stdev,2);     // standard deviation, microvolts
 Serial.println();

} // end loop()



*/
