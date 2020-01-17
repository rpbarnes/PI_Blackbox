
#include "Pin_Defs.h"

bool DEBUG = false;
bool WAIT_ENTER = false;

#define	DAC_CS	RPI_GPIO_P1_16	//P4
#define ADS1256_DRDY  		17   // GPIO17, GPIO_GEN0, physical pin 11 .....
#define ADS1256_RST  			18    //    .... physical pin 12,
#define ADS1256_CS  			22     // Physical pin 15, GPIO22, (GPIO_GEN3)
#define ADS1256_SYNC 			27

#define RELAY_RESET				12			// physical pin 32
#define RELAY_SET					1		// physical pin 28

#define TEMP_DIODE_POS		4		// Analog Input 4
#define TEMP_DIOE_NEG			5		//Analog Input 5
#define VOLT_CHANNEL 			2		// analog input 2
#define VOLT_CH_POS 			2		// analog input 2
#define VOLT_CH_NEG 			3		// analog input 3
#define CURRENT_CHANNEL 	0		// analog input 0
#define CURRENT_CH_POS 		0		// analog input 0
#define CURRENT_CH_NEG 		1		// analog input 0

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

#define ONE_MILLISECOND 1000
#define RELAY_TIME_ON	500*ONE_MILLISECOND

#define NUM_CHANNELS 8
#define	SINGLE_ENDED 0
#define DIFFERENTIAL 1
#define   NOP       0xFF  //No operation

static const uint8_t AIN0  = 0b0000;
static const uint8_t AIN1  = 0b0001;
static const uint8_t AIN2  = 0b0010;
static const uint8_t AIN3  = 0b0000;
static const uint8_t AIN4  = 0b0100;
static const uint8_t AIN5  = 0b0101;
static const uint8_t AIN6  = 0b0110;
static const uint8_t AIN7  = 0b0111;
#define AIN_COM (1 << 3)


uint8_t DIFFCH0 = (0b0000 << 4) | 0b0001; /*  AIN0 AIN1 */
uint8_t DIFFCH1 = (0b0010 << 4) | 0b0011; /*  AIN2 AIN3 */
uint8_t DIFFCH2 = (0b0100 << 4) | 0b0101; /*  AIN4 AIN5 */
uint8_t DIFFCH3 = (0b0110 << 4) | 0b0111; /*  AIN6 AIN7 */

/* gain channels */
typedef enum
{
	ADS1256_GAIN_1			= (0),	/* GAIN   1 */
	ADS1256_GAIN_2			= (1),	/*GAIN   2 */
	ADS1256_GAIN_4			= (2),	/*GAIN   4 */
	ADS1256_GAIN_8			= (3),	/*GAIN   8 */
	ADS1256_GAIN_16			= (4),	/* GAIN  16 */
	ADS1256_GAIN_32			= (5),	/*GAIN    32 */
	ADS1256_GAIN_64			= (6),	/*GAIN    64 */
}ADS1256_GAIN_ENUM;

typedef enum
{
	DRATE_30000SPS 	= (0b11110000), //30,000 SPS (default)
	DRATE_15000SPS	= (0b11100000), //15,000 SPS
	DRATE_7500SPS	= (0b11010000), //7,500 SPS
	DRATE_3750SPS	= (0b11000000), //3,750 SPS
	DRATE_2000SPS	= (0b10110000), //2,000 SPS
	DRATE_1000SPS	= (0b10100001), //1,000 SPS
	DRATE_500SPS	= (0b10010010), //500 SPS
	DRATE_100SPS	= (0b10000010), //100 SPS
	DRATE_60SPS	= (0b01110010), //60 SPS
	DRATE_50SPS	= (0b01100011), //50 SPS
	DRATE_30SPS	= (0b01010011), //30 SPS
	DRATE_25SPS	= (0b01000011), //25 SPS
	DRATE_15SPS	= (0b00110011), //15 SPS
	DRATE_10SPS	= (0b00100011), //10 SPS
	DRATE_5SPS	= (0b00010011), //5 SPS
	DRATE_2_5SPS	= (0b00000011), //2.5 SPS
}ADS1256_DRATE_ENUM;

#define ADS1256_DRATE_COUNT = 16;
static const uint8_t ADS1256_DRATE_MAX	= (0b11110000);

typedef struct
{
	ADS1256_GAIN_ENUM Gain;		     /* GAIN  */
	ADS1256_DRATE_ENUM DataRate;	   /* DATA output  speed*/
	int32_t AdcCount[8];	         /* ADC  Count for a given channel */
	uint8_t Channel;			       /* The current channel*/
	uint8_t ScanMode;	           /* Scanning mode,   0=Single-ended input,  1=Differential */
}ADS1256_OBJECT;

/* REGISTER ADDRESS DEFINITIONS --- ADS1256 datasheet Page 30----- */
enum
{
	/*Register address (in hex), followed by each registers default value on startup */
	REG_STATUS = 0x0,	// 0x00
	REG_MUX    = 0x1, // 0x01
	REG_ADCON  = 0x2, // 0x20
	REG_DRATE  = 0x3, // 0xF0
	REG_GPIO   = 0x4, // E0H
	REG_OFC0   = 0x5, // xxH
	REG_OFC1   = 0x6, // xxH
	REG_OFC2   = 0x7, // xxH
	REG_FSC0   = 0x8, // xxH
	REG_FSC1   = 0x9, // xxH
	REG_FSC2   = 0xA, // xxH
};

/* Command definitions --- ADS1256 datasheet Page 34 */
enum
{
	CMD_WAKEUP  = 0x00,	// Completes SYNC and Exits Standby Mode 0000  0000 (00h)
	CMD_READ_DATA   = 0x01, // Read Data 0000  0001 (01h)
	CMD_READ_DATA_CONTINOUSLY  = 0x03, // Read Data Continuously 0000 0011 (03h)
	CMD_SEND_DATA_CONTINOUSLY  = 0x0F, // Stop Read Data Continuously 0000   1111 (0Fh)
	CMD_READ_REG    = 0x10, // Read from REG 0001 | rrrr (0x1 | reg_addr)
	CMD_WRITE_REG    = 0x50, // Write to REG 0101 | rrrr (0x50 | reg_addr)
	CMD_SELF_CAL = 0xF0, // Offset and Gain Self-Calibration 1111    0000 (F0h)
	CMD_SELF_OFFSET_CAL= 0xF1, // Offset Self-Calibration 1111    0001 (F1h)
	CMD_SELF_GAIN_CAL= 0xF2, // Gain Self-Calibration 1111    0010 (F2h)
	CMD_SYSTEM_OFFSET_CAL = 0xF3, // System Offset Calibration 1111   0011 (F3h)
	CMD_SYSTEM_GAIN_CAL = 0xF4, // System Gain Calibration 1111    0100 (F4h)
	CMD_SYNC    = 0xFC, // Synchronize the A/D Conversion 1111   1100 (FCh)
	CMD_STANDBY = 0xFD, // Begin Standby Mode 1111   1101 (FDh)
	CMD_RESET   = 0xFE, // Reset to Power-Up Values 1111   1110 (FEh)
};

static const uint8_t VOLT_DIFF_CHANNEL = 1;
static const uint8_t CURRENT_DIFF_CHANNEL = 0;
static const uint8_t TEMP_DIODE_DIFF_CHANNEL = 2;

#define ENTER_KEY 10

const double V_FULL_SCALE = 5.0;
const int TOTAL_COUNTS = 8388608; // 2^23, this is for (+) voltages, same # of counts for (-) voltages
double VOLTS_PER_COUNT = 0;

int NUM_TABS = 0;
#define TXRX_TABS 6

#define BUS_SPEED_100KHZ  100000
#define BUS_SPEED_300KHZ  300000
#define BUS_SPEED_400KHZ  400000

static const int BUS_SPEED = BUS_SPEED_100KHZ;

#define DAC_CS_HIGH() bcm2835_gpio_write(DAC_CS,HIGH)     // encapsulates library pin write function
#define DAC_CS_LOW()  bcm2835_gpio_write(DAC_CS,LOW)      // encapsulates library pin write function
#define CS_HIGH() bcm2835_gpio_write(ADS1256_CS,HIGH)     // encapsulates library pin write function
#define CS_LOW()  bcm2835_gpio_write(ADS1256_CS,LOW)      // encapsulates library pin write function
#define DRDY_IS_LOW()	((bcm2835_gpio_lev(ADS1256_DRDY)==0))   // reads ADS1256_DRDY pin level via bcm2835 library
#define RST_HIGH() 	bcm2835_gpio_write(ADS1256_RST,HIGH)
#define RST_LOW() 	bcm2835_gpio_write(ADS1256_RST,LOW)

ADS1256_OBJECT ADS1256;

void ADS1256_Change_MUX(uint8_t channel);
void ADS1256_SetChannel(uint8_t _channel);
void ADS1256_Init(void);
void ADS1256_Set_Channel_Mode(uint8_t _ScanMode);
static void ADS1256_Write_Byte(uint8_t _data);
void SendCmd(uint8_t cmd);
static void Hard_Reset(void);
static void ADS1256_Sync();
void ADS1256_Config(ADS1256_GAIN_ENUM _gain, ADS1256_DRATE_ENUM _drate);
static void ADS1256_DelayDATA(void);
static uint8_t ADS1256_ReadByte(void);
static void ADS1256_WriteReg(uint8_t _RegID, uint8_t _RegValue);
static uint8_t ADS1256_ReadReg(uint8_t _RegID);
void ADS1256_Set_Single_Ended_Channel(uint8_t _channel);
static void ADS1256_Set_Differential_Channel(uint8_t _channel);
static void ADS1256_WaitDRDY(void);
static int32_t ADS1256_ReadData(void);
int32_t ADS1256_Get_ADC_Count(uint8_t channel_number);
void Select_Next_Channel();
void Read_Single_Ended(void);
void Read_Differential(void);
uint16_t Voltage_Convert(float Vref, float voltage);
double Corrected_Volt(double raw_volt);
double Counts_to_Volts(int adc_counts);
void Calibrate_Gain_and_Offset();
double Take_Measurement();
void Wait_Enter();
void Relay_ON();
void Relay_OFF();
void Set_I2C_Address(uint8_t address);
void Init_Library(void);


static void ADS1256_Sync()
{
	bcm2835_delayMicroseconds(3);
	ADS1256_Write_Byte(CMD_SYNC);
	bcm2835_delayMicroseconds(5);
	ADS1256_Write_Byte(CMD_WAKEUP);
	bcm2835_delayMicroseconds(6);
}

void ADS1256_Change_MUX(uint8_t channel)
{
		if (channel == 0) ADS1256_WriteReg(REG_MUX, DIFFCH0);	/*  AIN0 AIN1 */
		else if (channel == 1) ADS1256_WriteReg(REG_MUX, DIFFCH1);	/*  AIN2 AIN3 */
		else if (channel == 2) ADS1256_WriteReg(REG_MUX, DIFFCH2);	/*  AIN4 AIN5 */
		else if (channel == 3)	ADS1256_WriteReg(REG_MUX, DIFFCH3);	/*  AIN6 AIN7 */
}

void ADS1256_Init(void)
{
  	if (!bcm2835_init())
  	{
    	printf("bcm2835_init failed. Are you running as root??\n");
    	return;
  	}
  	if (!bcm2835_spi_begin())
  	{
    	printf("bcm2835_i2c_begin failed. Are you running as root??\n");
    	return;
  	}

  	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
  	bcm2835_spi_setDataMode(BCM2835_SPI_MODE1);
  	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);

		bcm2835_gpio_fsel(ADS1256_CS, BCM2835_GPIO_FSEL_OUTP);			// Set Chip Select as output pin
		bcm2835_gpio_write(ADS1256_CS, HIGH);												// Write Chip Select High

		bcm2835_gpio_fsel(ADS1256_DRDY, BCM2835_GPIO_FSEL_INPT);		// Set ADS1256_DRDY as input pin
		bcm2835_gpio_set_pud(ADS1256_DRDY, BCM2835_GPIO_PUD_UP);		// Pull up the ADS1256_DRDY pin

		bcm2835_gpio_fsel(ADS1256_SYNC, BCM2835_GPIO_FSEL_OUTP);		// Set Chip Select as output pin
		bcm2835_gpio_write(ADS1256_SYNC, HIGH);											// Write Sync High (low == power down)

		bcm2835_gpio_fsel(ADS1256_RST, BCM2835_GPIO_FSEL_OUTP);			// Set Reset as output pin
		bcm2835_gpio_write(ADS1256_RST, HIGH);											// Write Reset High (low == stay in reset)
/*
		bcm2835_gpio_fsel(RELAY_RESET, BCM2835_GPIO_FSEL_OUTP);			// Set Reset as output pin
		bcm2835_gpio_write(RELAY_RESET, LOW);

		bcm2835_gpio_fsel(RELAY_SET, BCM2835_GPIO_FSEL_OUTP);			// Set Reset as output pin
		bcm2835_gpio_write(RELAY_SET, LOW);

		bcm2835_gpio_fsel(DAC_CS, BCM2835_GPIO_FSEL_OUTP);		// declare chip select pin as an output
		bcm2835_gpio_write(DAC_CS, HIGH); 				// write chip select high (should always be high unless sending data)
*/
		delayMicroseconds(10);
		Calibrate_Gain_and_Offset();
		delayMicroseconds(100*ONE_MILLISECOND);
		double VOLT_QUANTA = V_FULL_SCALE / TOTAL_COUNTS;  // 1 LSB
		VOLTS_PER_COUNT = VOLT_QUANTA;

}

void Print_Tabs(int num_to_print)
{
	for(int i =0; i< num_to_print; i++) printf("\t");
	//if(DEBUG) printf("%d", num_to_print);
}

double Counts_to_Volts(int32_t adc_counts)
{
		double volts = adc_counts * VOLTS_PER_COUNT;

		return volts;
}

void Calibrate_Gain_and_Offset()
{
	delayMicroseconds(ONE_MILLISECOND);
	SendCmd(CMD_SELF_CAL);
	while( !DRDY_IS_LOW() ){}
}

// Write a byte to ADS1256
static void ADS1256_Write_Byte(uint8_t _data)
{
	bcm2835_delayMicroseconds(1);
	bcm2835_spi_transfer(_data);
}

static void Hard_Reset() {
  bcm2835_gpio_write(ADS1256_RST,LOW); //perform hardware reset
  delayMicroseconds(30); // LOW at least 4 clock cycles of onboard clock. 10 microseconds is enough
  bcm2835_gpio_write(ADS1256_RST,HIGH);
  delayMicroseconds(50);
}

// Configure Gain and Data Rate
void ADS1256_Config(ADS1256_GAIN_ENUM _gain, ADS1256_DRATE_ENUM _drate)
{
	if(DEBUG) printf("ADS1256_CONFIG\n");
	Hard_Reset();
	delayMicroseconds(ONE_MILLISECOND);
	if(DEBUG){ printf("\n"); Print_Tabs(NUM_TABS); }
	if(DEBUG) printf("Set DRATE_10SPS:\n");
	++NUM_TABS;
	if(DEBUG)Print_Tabs(NUM_TABS);
	ADS1256_WriteReg(REG_DRATE, DRATE_10SPS);
	--NUM_TABS;
//	if(DEBUG)Wait_Enter();

	ADS1256.Gain = _gain;
	ADS1256.DataRate = _drate;

	ADS1256_WaitDRDY(); // Wait until a conversion has completed before sending data

		uint8_t buf[4];		/* ads1256 register configuration parameters */
		// buf[0] = STATUS register parameters
		// buf[1] = Mux Register parameters
		// buf[2] = ADCON register parameters
		// buf[3] = Data Rate parameters

		/*Status register define
			Bits 7-4 ID3, ID2, ID1, ID0  Factory Programmed Identification Bits (Read Only)

			Bit 3 ORDER: Data Output Bit Order
				0 = Most Significant Bit First (default)
				1 = Least Significant Bit First
			Input data  is always shifted in most significant byte and bit first. Output data is always shifted out most significant
			byte first. The ORDER bit only controls the bit order of the output data within the byte.

			Bit 2 ACAL : Auto-Calibration
				0 = Auto-Calibration Disabled (default)
				1 = Auto-Calibration Enabled
			When Auto-Calibration is enabled, self-calibration begins at the completion of the WREG command that changes
			the PGA (bits 0-2 of ADCON register), DR (bits 7-0 in the DRATE register) or BUFEN (bit 1 in the STATUS register)
			values.

			Bit 1 BUFEN: Analog Input Buffer Enable
				0 = Buffer Disabled (default)
				1 = Buffer Enabled

			Bit 0 DRDY :  Data Ready (Read Only)
				This bit duplicates the state of the DRDY pin.

			ACAL=1  enable  calibration
		*/
		//buf[0] = (0 << 3) | (1 << 2) | (1 << 1);//enable the internal buffer
        //buf[0] = (0 << 3) | (1 << 2) | (0 << 1);  // The internal buffer is prohibited
				buf[0] = 0b0000 | 0b0100 | 0b0000;  // sets buffer off, autocal enabled, MSB first

        //ADS1256_WriteReg(REG_STATUS, (0 << 3) | (1 << 2) | (1 << 1));

		buf[1] = 0x08; // Mux register, setting positive input to AIN0, negative input to AINCOM

		/*	ADCON: A/D Control Register (Address 02h)
			Bit 7 Reserved, always 0 (Read Only)
			Bits 6-5 CLK1, CLK0 : D0/CLKOUT Clock Out Rate Setting
				00 = Clock Out OFF
				01 = Clock Out Frequency = fCLKIN (default)
				10 = Clock Out Frequency = fCLKIN/2
				11 = Clock Out Frequency = fCLKIN/4
				When not using CLKOUT, it is recommended that it be turned off. These bits can only be reset using the RESET pin.

			Bits 4-3 SDCS1, SCDS0: Sensor Detect Current Sources
				00 = Sensor Detect OFF (default)
				01 = Sensor Detect Current = 0.5 A
				10 = Sensor Detect Current = 2 A
				11 = Sensor Detect Current = 10 A
				The Sensor Detect Current Sources can be activated to verify  the integrity of an external sensor supplying a signal to the
				ADS1255/6. A shorted sensor produces a very small signal while an open-circuit sensor produces a very large signal.

			Bits 2-0 PGA2, PGA1, PGA0: Programmable Gain Amplifier Setting
				000 = 1 (default)
				001 = 2
				010 = 4
				011 = 8
				100 = 16
				101 = 32
				110 = 64
				111 = 64
		*/
		buf[2] = (0 << 5) | (0 << 3) | (_gain << 0); // sets clock out off, sensor detect off, no gain from PGA
		buf[3] = _drate;	// DRATE_10SPS;

		if(DEBUG)printf("Setting Parameters for 4 registers:\n");
		CS_LOW();	// take chip select low to begin sending data
				if(DEBUG)printf("\tWRTCMD, NUM_WRT_REGS\tTX: 0x50 ");
		    ADS1256_Write_Byte(CMD_WRITE_REG | 0x00);	/* Write command, OR'd with the register address to write*/
					bcm2835_delayMicroseconds(5);						// 0x00 is status register
				if(DEBUG)printf("0x03\n");
		    ADS1256_Write_Byte(0x03);			/* Writing to 4 registers,send number of regs minus one */

				if(DEBUG)printf("\t\tSTATUS REG:\t 0x%02X\n", buf[0]);
		    ADS1256_Write_Byte(buf[0]);	/* Set the status register */
					bcm2835_delayMicroseconds(5);
						if(DEBUG)printf("\t\tMUX REG:\t 0x%02X\n", buf[1]);
		    ADS1256_Write_Byte(buf[1]);	/* Set the input channel parameters (MUX register) */
					bcm2835_delayMicroseconds(5);
						if(DEBUG)printf("\t\tADCON REG:\t 0x%02X\n", buf[2]);
		    ADS1256_Write_Byte(buf[2]);	/* Set the ADCON control register,gain */
					bcm2835_delayMicroseconds(5);
						if(DEBUG)printf("\t\tDRATE REG:\t 0x%02X\n", buf[3]);
		    ADS1256_Write_Byte(buf[3]);	/* Set the output data rate (DRATE register)*/
		CS_HIGH();	/* Take Chip Select High to end the transfer */
	//if(DEBUG) Wait_Enter();
		if(DEBUG)
		{
			printf("\t\t\tREADBACK\n");
			++NUM_TABS;
			uint8_t Read_Back_Val=0;
			Read_Back_Val=ADS1256_ReadReg(REG_STATUS);
			printf("STATUS REG:  0x%02X\n", Read_Back_Val);

			Read_Back_Val=ADS1256_ReadReg(REG_MUX);
			printf("MUX REG:  0x%02X\n", Read_Back_Val);

			Read_Back_Val=ADS1256_ReadReg(REG_ADCON);
			printf("ADCON REG:  0x%02X\n", Read_Back_Val);

			Read_Back_Val=ADS1256_ReadReg(REG_DRATE);
			printf("DRATE REG:  0x%02X\n", Read_Back_Val);
			--NUM_TABS;
		}


	bcm2835_delayMicroseconds(50);
}


static void ADS1256_DelayDATA(void)
{
	/*
		Delay from last SCLK edge for clocking data DIN to first SCLK rising edge for clocking data DOUT:
		min time = 50*master_clock_period
		master_clock_period = 1/7.68 Mhz = .13uS
		min_time = 50*.13uS = 6.5uS
		RDATA, RDATAC,RREG Commands min  50 clock cycles = 50 * 0.13uS = 6.5uS
	*/
	bcm2835_delayMicroseconds(8);	/* The minimum time delay 6.5us, using 10 to be safe */
}

// Assumes register to read has already been sent, only performing the read operation here
static uint8_t ADS1256_ReadByte(void)
{
	if(DEBUG)
	{
		//printf("\nnt_%d \r", NUM_TABS);
		Print_Tabs(NUM_TABS);
		printf("READBYTE\r");
		++NUM_TABS;
		Print_Tabs(TXRX_TABS);
	}

	uint8_t read_byte = 0;                    // storage container for byte to read
  ADS1256_DelayDATA();	               			/*delay time */

	read_byte = bcm2835_spi_transfer(NOP);    // must "send" data to receive data
	if(DEBUG)
	{
		printf("[ TX: 0xFF  RX: ");
		printf("0x%02X ] \n", (read_byte | 0x00));
		--NUM_TABS;
	}
	return read_byte;
}

// Write to a register
static void ADS1256_WriteReg(uint8_t RegID, uint8_t RegValue)
{
	uint8_t byte = 0;
	byte = (CMD_WRITE_REG | RegID);

	if(DEBUG)
	{
		printf("WRITE_REG\r");
		++NUM_TABS;
		Print_Tabs(TXRX_TABS);
		printf("[ TX: 0x%02X ", byte);
	}

	CS_LOW();																		/* bring Chip Select low to start a transfer */
	   ADS1256_Write_Byte(CMD_WRITE_REG | RegID);	 	/* Send 1st byte, address of register to read */
		 bcm2835_delayMicroseconds(1);

	   ADS1256_Write_Byte(0x00);		           	/* Send second byte, writing only 1 register, so second byte is 0 */
		 bcm2835_delayMicroseconds(1);

	   ADS1256_Write_Byte(RegValue);	         	/* write value to the register */
	CS_HIGH();														 			/* take Chip Select high to end the transfer*/
	if(DEBUG)printf(" 0x00 0x%02X ]", RegValue | 0x00);
	//if(WAIT_ENTER) Wait_Enter();
	bcm2835_delayMicroseconds(10);

	uint8_t Read_Back_Val=ADS1256_ReadReg(RegID);

	//Print_Tabs(NUM_TABS);
  if ( RegValue != Read_Back_Val){
			printf("Write to Register %02X failed.\n", RegID);
			printf("Value read back was %d .\n", Read_Back_Val);
			printf("Value assigned was %d . ]\n", RegValue);
			//if(DEBUG) Wait_Enter();
	}
	// else printf(" Write Succeeded ||\n");
	if(DEBUG) --NUM_TABS;
}

static uint8_t ADS1256_ReadReg(uint8_t _RegID)
{
	uint8_t read_byte;
	uint8_t send_byte = (CMD_READ_REG | _RegID);
	if(DEBUG)
	{
			//printf("nt_%d \r", NUM_TABS);
			Print_Tabs(NUM_TABS);
			printf("READ_REG\r");
			++NUM_TABS;
			Print_Tabs(TXRX_TABS);
      printf("[ TX: 0x%02X ]\t", send_byte | 0x00);
      printf(" \t( reading reg: %02X )\n", _RegID);
      Print_Tabs(TXRX_TABS);
      printf("[ TX: 0x00 ] \n");
	}

	CS_LOW();	/* write Chip Select low to start a transfer */
	  ADS1256_Write_Byte(CMD_READ_REG | _RegID);	  /* Write register ID to the command register */
		bcm2835_delayMicroseconds(2);

		ADS1256_Write_Byte(0x00);	               	/* Write the number of regs to read */
		delayMicroseconds(8); 											// min delay: t6 = 50 * 1/freq.clkin = 50 * 1 / 7,68 Mhz = 6.5 micro sec
    read_byte = ADS1256_ReadByte();	    			/* Read the register byte value */
	CS_HIGH();										/* take Chip Select high to end the transfer*/
	if(DEBUG)
	{
		Print_Tabs(TXRX_TABS);
		printf("return val: 0x%02X\n", read_byte);
		--NUM_TABS;
	}
	return read_byte;
}

void SendCmd(uint8_t _cmd)
{
	if(DEBUG) printf("SENDCMD:\t\t0x%02X\n", _cmd);
	CS_LOW();
      delayMicroseconds(5);
	    ADS1256_Write_Byte(_cmd);
      delayMicroseconds(5);
	CS_HIGH();
}

// For this mode the negative terminal will always be N_Common
// _channel can be 0, 1, 2, 3, 4, 5, 6, or 7
void ADS1256_Set_Single_Ended_Channel(uint8_t _channel)
{
  if(DEBUG) printf("\nCHANGING TO CH %d\n", _channel);
	if (_channel > 7) // there are only 8 channels (0-7), larger than 7 is out of range
	{ return; }
	uint8_t pos_channel = _channel;
	uint8_t neg_channel = AIN_COM;
	uint8_t channel_byte = (pos_channel << 4) | neg_channel;  // first four bits=pos channel, last 4 = neg channel
	ADS1256_WriteReg(REG_MUX, channel_byte);	// write to MUX register to configure channel select
  ADS1256.Channel = _channel;
}

/*
  configure differential channel to read
	channel number can be  0, 1, 2, or 3
	first four bits are positive pin, last 4 bits are negative pin
*/
static void ADS1256_Set_Differential_Channel(uint8_t channel)
{
	     if (channel == 0) ADS1256_WriteReg(REG_MUX, DIFFCH0);	/*  AIN0 AIN1 */
	else if (channel == 1) ADS1256_WriteReg(REG_MUX, DIFFCH1);	/*  AIN2 AIN3 */
	else if (channel == 2) ADS1256_WriteReg(REG_MUX, DIFFCH2);	/*  AIN4 AIN5 */
	else if (channel == 3)	ADS1256_WriteReg(REG_MUX, DIFFCH3);	/*  AIN6 AIN7 */
}

 // wait until ADS1256 finishes performing conversion and data is ready for reading.
 // interrupts are slow on the PI so just occupying the CPU with for loop while waiting
 // DRDY pin goes low when data is available for reading
static void ADS1256_WaitDRDY(void)
{
	uint32_t i;
	for (i = 0; i < 400000; i++)
	{
		delayMicroseconds(1);
		if ( DRDY_IS_LOW() ){ break; }
	}
	if (i >= 400000){ printf("\n\t----ADS1256_WaitDRDY() Time Out---- ...\r\n"); }
}

// read ADC 32 bit value
static int32_t ADS1256_ReadData(void)
{
	uint32_t counts = 0;      // 32 bit storage to hold 24 bit return value
  static uint8_t read[3];  // Array to hold (3) byte reads

	if(DEBUG)
	{
		printf("READ_DATA\r");
		++NUM_TABS;
		Print_Tabs(TXRX_TABS);
		printf("[ TX: 0x%02X ]\n", CMD_READ_DATA);
	}

	CS_LOW();	/* Start Transfer by bringing Chip Select low */
	    ADS1256_Write_Byte(CMD_READ_DATA);	/* send ADC read command  */
	    ADS1256_DelayDATA();	/* delay time mandated before reading */
	    ADS1256_WaitDRDY();  // wait until ADS1256 signals there is data ready for reading
			/*Read the 24bit value in counts */
      read[0] = ADS1256_ReadByte();				// MSB
      delayMicroseconds(10);
      read[1] = ADS1256_ReadByte();
      delayMicroseconds(10);
      read[2] = ADS1256_ReadByte();				// LSB

			// combine 3 bytes into one 24 bit number
      counts = ((uint32_t)read[0] << 16) & 0x00FF0000; // uint32 is 32 bits, this byte is bits 16-24, shift 16 places
      counts |= ((uint32_t)read[1] << 8); // shift the second byte over 8 bits, then OR with result
      counts |= read[2]; // last 8 bits (LSB)

	CS_HIGH();	/* End the transfer by bringing Chip Select High*/


	// If negative number, fill leading zeroes with 1's to configure the "sign" as negative
	// If the MSB of the 24 bit number is 1, that means the result is negative (2's complement)
	// If the MSB is 1, then an AND with 1000 (8 in hex) will produce true in the if statement
  if (counts & 0x800000)
  {
		//printf("negative number");
	    counts |= 0xFF000000; // Fill the rest of the leading bits with 1
  }
	if(DEBUG)
	{
		Print_Tabs(NUM_TABS);
		printf("Count:\t%d", counts);
    --NUM_TABS;
		//Wait_Enter();
	}
	return (int32_t)counts;
}


void Set_I2C_Address(uint8_t address)
{
    bcm2835_i2c_setSlaveAddress(address);
    return;
}

void Init_Library(void)
{
    if (!bcm2835_init())
    {
      printf("bcm2835_init failed. Are you running as root??\n");
      return;
    }
    if (!bcm2835_i2c_begin())
    {
      printf("bcm2835_i2c_begin failed. Are you running as root??\n");
      return;
    }
    bcm2835_i2c_set_baudrate(BUS_SPEED_100KHZ);
    bcm2835_i2c_setSlaveAddress(INA233_HEAT_LOAD_ADDR); // bits 1-7 is the address, bit 0 is read or write bit
    printf("I2C has been initialized...\n\n");
}
