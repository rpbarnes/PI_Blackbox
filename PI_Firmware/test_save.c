
#define _USE_XOPEN 700
#define _GNU_SOURCE
#ifndef _SAVE_FILE_
#define _SAVE_FILE_

#include <fcntl.h>  /* File Control Definitions          */
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include "influxdb.h"

#define DATABASE "pi_data"
#define HOST "192.168.250.111"
#define PORT 8086

char BLACKBOX_ID[20];
char DISPLACER_ID[20];
char CCC_ID[20];
const char COMPRESSOR_ID[20];
char DEWAR_ID[20];
char PROFILE_ID[20];

const char* SENDLINE_HEADER = "curl -i -XPOST 'http://192.168.250.111:8086/write?db=pi_data' --data-binary ";


static const char DUT_CONFIG_FILEPATH[] = "/home/pi/DUT_CONFIG/DUT_Config.txt";
static const char USB_FILEPATH[] = "/home/pi/USB_drive/Logs";
static const char SD_FILEPATH[] = "home/pi/Logs";
#define SAVE_FILE_SUFFIX "_data.txt"  // file to store reading data
#define POWER_SAVE_FILE_SUFFIX "_power.txt"  // file to store reading data
#define DATA_START 5
#define DATA_STOP 6

#define MAX_SAMPLES 100
#define MAX_LOOP_SAMPLES 100
char SAVE_FILE[100];
char DUT_ID[20];
FILE* file_pointer;
bool Relay_State = false;
long INITIAL_COOLER_ON_TIME_SECONDS = 0;

struct timespec log_start_time;
time_t log_tm_start;
long log_start_time_secs = 0;
struct timeval timer_start, timer_stop;

time_t loop_start;
struct tm* loop_tm_start;

struct data_point
{
    struct timespec t_stamp;
    time_t raw_time;
    struct tm* tm_info;
    double data_raw;
		double data_corrected;
};

struct index_track
{
		int volt_sample;
		int amp_sample;
		int watt_sample;
		int diode_sample;
		int data_time_series;
};

struct time_interval
{
    struct timespec tspec_start;
    time_t t_start;
    struct tm* tm_start;

};

struct all_metrics
{
		char time[30];
    char date[20];
    char elapsed_time[30];
		long long elapsed_secs;
    char relay_state[10]; // ON or OFF

    double CCC_V;
    double CCC_raw_V;
    double CCC_A;
    double CCC_W;
    double CCC_raw_A;
    double diode_volts;
		double diode_temp;
		double Heat_Load_V;
		double Heat_Load_mA;
		double Heat_Load_mW;
    double thermocouple1_temp;
    double thermocouple2_temp;
    double thermocouple3_temp;
};

typedef struct data_point Data_Point; // making an instance of the data_point struct
struct index_track index_now; // making an instance of the data_point struct
typedef struct time_interval Time_Track; // making an instance of the data_point struct
typedef struct all_metrics Loop_Point; // making an instance of the data_point struct

char time_buffer[100];
char date_buffer[30];


Data_Point Volt_Metrics[MAX_LOOP_SAMPLES]; // room for 100 Data Points saved in memory before being written to file
Data_Point Amp_Metrics[MAX_LOOP_SAMPLES]; // room for 100 Data Points saved in memory before being written to file
Data_Point Watt_Metrics[MAX_LOOP_SAMPLES]; // room for 100 Data Points saved in memory before being written to file
Data_Point Diode_Metrics[MAX_LOOP_SAMPLES]; // room for 100 Data Points saved in memory before being written to file
Loop_Point Save_Data[MAX_SAMPLES+100]; // only used for elapse time right now

Data_Point AVG_Data[15]; // room for 100 Data Points saved in memory before being written to file
int save_index = 0; // keeps track of how many data points in memory (not written to file yet)

char* Get_TimeStamp(char* buffer);
void Calc_Averages();
void Seconds_To_Elapsed_Time_String(long long total_seconds, char* string_buffer);
double Calc_Metric_Corrected_Average(Data_Point* Metric, int num_data_points);
double Calc_Metric_Raw_Average(Data_Point* Metric, int num_data_points);
void Clear_Loop_Index();
char* Get_Date(char* buffer);
void Write_Data_To_DB(Loop_Point* Load_Data, int index_to_write);

/*
// Reads whether ADC sense voltage output from PI DAQ to determine initial relay state
bool Get_Relay_State()
{
    // Take first reading after MUX change and throw away
    ADS1256_Change_MUX(VOLT_DIFF_CHANNEL);
    ADS1256_Sync();
    ADS1256_ReadData();
    bcm2835_delayMicroseconds(50);

    double count_sum = 0;
    int NUM_RELAY_SAMPLES = 3;
    for(int i=0; i<NUM_RELAY_SAMPLES; i++)
    {
        ADS1256_WaitDRDY();
        ADS1256_Write_Byte(CMD_SYNC);
        bcm2835_delayMicroseconds(5);
        ADS1256_Write_Byte(CMD_WAKEUP);
        bcm2835_delayMicroseconds(5);

        count_sum += ADS1256_ReadData();
    }
    double avg_volts = count_sum / NUM_RELAY_SAMPLES;
    avg_volts = get_Corrected_Volt( avg_volts  );
    if(avg_volts >= 1.0)
    {
        Relay_State = true;
    }
    else {  Relay_State = false; }


    return Relay_State;
}
*/

bool File_Exists(const char* filename)
{
    FILE *file;
    if ((file = fopen(filename, "r")))
    {
        fclose(file);
        return true;
    }
    return false;
}

long int Parse_Compressor_On_Time(const char* COMPRESSOR)
{
    long int on_time = 0;
    // scan for all files with same "compressor ID"
    // scan for same filename today for running time


return 0;

}


void Parse_DUT_Config_File()
{
  file_pointer = fopen(DUT_CONFIG_FILEPATH, "r"); // "r" for read only
  //new_file = fopen(NEW_FILE, "w");

  printf("Parsing DUT ID FILE. File open result: %d\n", file_pointer);


  char line[300];
  char temp[30];

	// First Scan the File Header to Get Log Start Time
	bool Last_Item = false;
	char* FILE_START_TAG 	= "####";
  char* BLACKBOX_TAG   = "Blackbox_ID: ";
  char* DISPLACER_TAG  = "Displacer_ID: ";
  char* CCC_TAG        = "CCC_ID: ";
  char* COMPRESSOR_TAG = "Compressor_ID: ";
  char* DEWAR_TAG      = "Dewar_ID: ";
  char* PROFILE_TAG    = "Profile_ID: ";


	char* contains;

	int lines_read = 0;
  while( (fgets(line, 256, file_pointer) != NULL) && lines_read <10 && !Last_Item)
	{
			++lines_read;
      printf("line: \t%s", line);
			contains = strstr(line, FILE_START_TAG);
			if(contains)
			{  continue;	printf("contains1\n"); }

      contains = strstr(line, BLACKBOX_TAG);
			if(contains)
			{
        sscanf(line, "%s %s ", temp, BLACKBOX_ID );  // store the second string token in BLACKBOX
        printf("contains2\n");
        continue;
			}

      contains = strstr(line, DISPLACER_TAG);
			if(contains)
			{
        sscanf(line, "%s %s ", temp, DISPLACER_ID );  // store the second string token in DISPLACER, first token is discarded
        printf("contains3\n");
        continue;
			}

      contains = strstr(line, CCC_TAG);
			if(contains)
			{
        sscanf(line, "%s %s ", temp, CCC_ID );  // store the second string token in CCC first token is discarded
        printf("contains4\n");
        continue;
			}

      contains = strstr(line, COMPRESSOR_TAG);
			if(contains)
			{
        sscanf(line, "%s %s ", temp, COMPRESSOR_ID );  // store the second string token in COMPRESSOR, first token is discarded
        printf("contains5\n");
        continue;
			}

      contains = strstr(line, DEWAR_TAG);
			if(contains)
			{
        sscanf(line, "%s %s ", temp, DEWAR_ID );  // store the second string token in DEWAR, first token is discarded
        printf("contains6\n");
        continue;
			}

			contains = strstr(line, PROFILE_TAG);
			if(contains)
			{
				Last_Item = true;
        sscanf(line, "%s %s ", temp, PROFILE_ID );  // store the second string token in PROFILE
        printf("contains7\n");
        continue;
			}
	 }
   fclose(file_pointer);

   printf(" Blackbox: \t%s\n", BLACKBOX_ID);
   printf(" Displacer: \t%s\n", DISPLACER_ID);
   printf(" CCC: \t\t%s\n", CCC_ID);
   printf(" Compressor: \t%s\n", COMPRESSOR_ID);
   printf(" Dewar: \t%s\n", DEWAR_ID);
   printf(" Profile: \t%s\n", PROFILE_ID);

   return;
}



void Open_Log_File()
{
    //Check to see if we have a thumbdrive inserted by performing ls /dev/sda.  If so we will datalog to USB drive
    //If not, datalog is written to /home/pi/Logs/
    char fileName[256];
    int thumb_drive_exists = system("ls /dev/sda"); //thumb = 0 if thumbdrive detected or 512 if none available
                                       //If drive available, it is mounted to /home/pi/USB_drive
    Get_Date(date_buffer);
    date_buffer[strlen(date_buffer)] = '\0';	// Add null terminating character
    for(int i=0; i<strlen(date_buffer); i++)
    {
     		if(date_buffer[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
     		{	date_buffer[i] = '-';	}
    }
     //FOR REFERENCE see modified file in  /etc/FSTAB:
     // /dev/sda  /home/pi/USB_drive  auto  uid=pi,gid=pi,umask=0022,sync,noauto,nosuid,rw,user 0   0
    if(thumb_drive_exists==0)sprintf(fileName, "%s/%s%s", USB_FILEPATH, date_buffer, SAVE_FILE_SUFFIX);         // save logfile on USB Drive
    else sprintf(fileName, "%s/%s%s", SD_FILEPATH, date_buffer, SAVE_FILE_SUFFIX);                // else on SD card
    strcpy(SAVE_FILE, fileName);
    printf("save file:  %s\n", SAVE_FILE);

    file_pointer = fopen(SAVE_FILE, "a");
    fclose( file_pointer );
}


void Calc_Averages()
{
  char buffer[50];
  time_t elapsed_time;

  elapsed_time = loop_start - log_tm_start;
  long long elapsed_secs = (long long)elapsed_time;
  elapsed_secs += INITIAL_COOLER_ON_TIME_SECONDS;

  strftime (buffer,30,"%H:%M:%S", loop_tm_start);   // create a string from time structure
  strncpy(Save_Data[save_index].time, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] timestamp string

  strftime (buffer,20,"%m/%d/%y", loop_tm_start);   // create a string from time structure
  strncpy(Save_Data[save_index].date, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] timestamp string

  memset(Save_Data[save_index].elapsed_time,0,strlen(Save_Data[save_index].elapsed_time));
  Seconds_To_Elapsed_Time_String(elapsed_secs, buffer); // returns an hours min sec string for the elapsed secs, stores the string in buffer
  strncpy(Save_Data[save_index].elapsed_time, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] elapsed_time string
  strcat(Save_Data[save_index].elapsed_time, " ");
  Save_Data[save_index].elapsed_time[strlen(Save_Data[save_index].elapsed_time)] = '\0';
  Save_Data[save_index] .elapsed_secs = elapsed_secs;
  Save_Data[save_index].CCC_raw_V = Calc_Metric_Raw_Average(Volt_Metrics, index_now.volt_sample);
  Save_Data[save_index].CCC_V = Calc_Metric_Corrected_Average(Volt_Metrics, index_now.volt_sample);
  Save_Data[save_index].CCC_raw_A = Calc_Metric_Raw_Average(Amp_Metrics, index_now.amp_sample);
  Save_Data[save_index].CCC_A = Calc_Metric_Corrected_Average(Amp_Metrics, index_now.amp_sample);
  Save_Data[save_index].CCC_W = Save_Data[save_index].CCC_A * Save_Data[save_index].CCC_V;
  Save_Data[save_index].diode_volts = Calc_Metric_Raw_Average(Diode_Metrics, index_now.diode_sample);
  Save_Data[save_index].diode_temp = Calc_Metric_Corrected_Average(Diode_Metrics, index_now.diode_sample);
  Save_Data[save_index].diode_temp = Calc_Metric_Corrected_Average(Diode_Metrics, index_now.diode_sample);
  if(Relay_State)
  {
      strcpy(Save_Data[save_index].relay_state, "ON");
      Save_Data[save_index].relay_state[strlen("ON")] = '\0';
  }
  else
  {
      strcpy(Save_Data[save_index].relay_state, "OFF");
      Save_Data[save_index].relay_state[strlen("OFF")] = '\0';
  }
  Clear_Loop_Index();
}

void Clear_Loop_Index()
{
    index_now.volt_sample = 0;
    index_now.amp_sample = 0;
    index_now.watt_sample = 0;
    index_now.diode_sample = 0;
    index_now.data_time_series = 0;
}

void Seconds_To_Elapsed_Time_String(long long total_seconds, char* string_buffer)
{
		int display_seconds = total_seconds % 60;
		int total_minutes   = total_seconds / 60;
		int display_minutes = total_minutes % 60;
		int display_hours 		= total_minutes / 60;

		sprintf(string_buffer, "%dhr %dmin %dsec", display_hours, display_minutes, display_seconds);
}

double Calc_Metric_Corrected_Average(Data_Point* Metric, int num_data_points)
{
  double sum = 0;
  for(int i =0; i< num_data_points; ++i)
  {
      sum += Metric[i].data_corrected;
  }

  double average = sum/( (float)num_data_points );
  return average;
}

double Calc_Metric_Raw_Average(Data_Point* Metric, int num_data_points)
{
  double sum = 0;
  for(int i =0; i< num_data_points; ++i)
  {
      sum += Metric[i].data_raw;
  }

  double average = sum/( (float)num_data_points );
  return average;
}

char* Get_TimeStamp(char* buffer)
{
	 time_t rawtime;
	 struct tm* timeinfo;
	 time (&rawtime);
	 timeinfo = localtime(&rawtime);
   buffer[0] = '\0';
	 strftime (time_buffer,30,"%H:%M:%S  %m/%d/%Y  ",timeinfo);
	 strncpy(buffer, time_buffer, (int)strlen(time_buffer));
	 buffer[strlen(buffer)-1] = '\0';
	 return buffer;
}

void Save_TimeStamp(Data_Point* Metric, int index)
{
    clock_gettime(CLOCK_REALTIME, &Metric[index].t_stamp);
    time (&Metric[index].raw_time);
    Metric[index].tm_info = localtime(&Metric[index].raw_time);
}

char* Get_Date(char* buffer)
{
	 time_t rawtime;
	 struct tm* timeinfo;
	 time (&rawtime);
	 timeinfo = localtime(&rawtime);
	 strftime (time_buffer,30,"%m/%d/%y",timeinfo);
	 strncpy(buffer, time_buffer, (int)strlen(time_buffer));

	 return buffer;
}


void Loop_TimeStamp(int START_STOP)
{
    if(START_STOP == DATA_START)
    {
        time (&loop_start);
        loop_tm_start = localtime(&loop_start);
        gettimeofday(&timer_start, NULL);
    }
    if(START_STOP == DATA_STOP)
    {
      //gettimeofday(&timer_stop, NULL);
    }
}

// Must be called before storing any data points
// Calling this will overwrite any file with same filename  (same data and unitID)
// the argument is a string identifying this specific cooler under test
void Initialize_Log(char* dut_id)
{
//	Possibly Check if file exists already for todays date, if so dont overwrite file
		index_now.volt_sample = 0;  		// Initialize data storage indices to 0
		index_now.amp_sample = 0;
		index_now.watt_sample = 0;
		index_now.diode_sample = 0;
    index_now.data_time_series = 0;

    clock_gettime(CLOCK_REALTIME, &log_start_time);
    time(&log_tm_start);
    log_start_time_secs = log_start_time.tv_sec;

    Open_Log_File();

    char filename[30];
		char timestamp_str[30];
    char DUT_ID[40];
    char Blackbox_SN[40];
    char Displacer_SN[40];
    char CCC_SN[40];
    char Cooler_SN[40];
    char Dewar_SN[40];
    char Profile_SN[40];
    char ON_Time[40];

    Parse_DUT_Config_File();

    const char* DUT = "\t\t DUT_ID: XXXX \n";
    const char* Blackbox_Header = "\t\t\t\t   Blackbox SN: ";
    const char* Displacer_Header = "\t\t\t\t  Displacer SN: ";
    const char* CCC_Header = "\t\t\t\t        CCC SN: ";
    const char* Cooler_Header = "\t\t\t\t Compressor SN: ";
    const char* Dewar_Header = "\t\t\t\t      Dewar SN: ";
    const char* Profile_Header = "\t\t\t\t    Profile SN: ";
    const char* Cooler_Time_Header = "\t\t\t\t       Cooler On Time: ";

		const char* file_header1 = "  CCC \t   CCC \t    CCC      Dewar \t\t RLoad \t RLoad\t   R_PWR \t    THRMCPL";
    const char* file_header2 = "   V \t    A \t     W \t      degK    diodeV\t  V\t   mA\t     mW \t C \t C \t C \t  Time\t Date\t\t  Elapsed_Time \t      Relay";
    const char* line = "-----------------------------------------------------------------------------------------------------------------------------------------------------------\n";

    memset(date_buffer,0,sizeof(date_buffer));
    memset(timestamp_str,0,sizeof(timestamp_str));

    Get_Date(date_buffer);
    date_buffer[strlen(date_buffer)] = '\0';	// Add null terminating character
    for(int i=0; i<strlen(date_buffer); i++)
		{
			if(date_buffer[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
			{	date_buffer[i] = '-';	}
		}

		Get_TimeStamp(timestamp_str);
    timestamp_str[strlen(timestamp_str)] = '\0';
    for(int i=0; i<strlen(timestamp_str); i++)
  	{
  		if(timestamp_str[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
  		{	timestamp_str[i] = '-';	}     // filename cant have "/" slashes in it
  	}

    memset(Blackbox_SN,0,sizeof(Blackbox_SN));
    strcpy(Blackbox_SN, Blackbox_Header);
    strcat(Blackbox_SN, BLACKBOX_ID);
    Blackbox_SN[strlen(Blackbox_SN)] = '\0';

    memset(Displacer_SN,0,sizeof(Displacer_SN));
    strcpy(Displacer_SN, Displacer_Header);
    strcat(Displacer_SN, DISPLACER_ID);
    Displacer_SN[strlen(Displacer_SN)] = '\0';

    Cooler_SN[0] = '\0';
    strcpy(Cooler_SN, Cooler_Header);
    strcat(Cooler_SN, COMPRESSOR_ID);
    Cooler_SN[strlen(Cooler_SN)] = '\0';

    CCC_SN[0] = '\0';
    strcpy(CCC_SN, CCC_Header);
    strcat(CCC_SN, CCC_ID);
    CCC_SN[strlen(CCC_SN)] = '\0';

    Profile_SN[0] = '\0';
    strcpy(Profile_SN, Profile_Header);
    strcat(Profile_SN, PROFILE_ID);
    Profile_SN[strlen(Profile_SN)] = '\0';

    ON_Time[0] = '\0';
    strcpy(ON_Time, Cooler_Time_Header);
    long int cool_on_time = Parse_Compressor_On_Time(COMPRESSOR_ID);
    char on_time_string[20];
    sprintf(on_time_string, "%ld ", cool_on_time );
    strcat(ON_Time, on_time_string);
    ON_Time[strlen(ON_Time)] = '\0';

    Dewar_SN[0] = '\0';
    strcpy(Dewar_SN, Dewar_Header);
    strcat(Dewar_SN, DEWAR_ID);
    Dewar_SN[strlen(Dewar_SN)] = '\0';

	  file_pointer = fopen(SAVE_FILE, "a"); // open for writing, 'w' means overwrite any existing file, "a" means to append to file

    fprintf(file_pointer, "\n\t\t%s\n", Blackbox_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", Displacer_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", Cooler_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", Dewar_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", CCC_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n\n", Profile_SN); // write date to file
		fprintf(file_pointer, "\t\t\t\t\t\t%s\n\n", timestamp_str); // write date to file
    fprintf(file_pointer, "%s\n", file_header1); // write the header1 to file
		fprintf(file_pointer, "%s\n", file_header2); // write the header2 to file
    fprintf(file_pointer, "%s\n", line); // write the current divider line (-------) to file

	  fclose( file_pointer );

}

long int Get_Epoch_Timestamp(int second, int minute, int hour, int day, int month, int year)
{
		struct tm t;
		time_t dt_time;

		if(year <2000) year += 2000;
		t.tm_year = year - 1900;
		t.tm_mon = month -1;      // where january = 0
		t.tm_mday = day;          // Day of the month
		t.tm_hour = hour;
		t.tm_min = minute;
		t.tm_sec = second;
		t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
		dt_time = mktime(&t);

		return (long long)dt_time;
}

void Write_Data_To_Influx()
{
  for(int i=0; i<save_index; ++i)
  {
    Write_Data_To_DB(Save_Data, i);
  }
}

void Write_Data_To_DB(Loop_Point* Load_Data, int index_to_write)
{
	int second, minute, hour, day, month, year;
	char elapsed_time[30];
	char time_buffer[30];
	char date_buffer[20];
  char print_buffer[40];
  char epoch[25];
	struct tm convert = {0};

  char timestamp[50];
  strcpy(timestamp,Load_Data[index_to_write].time);
  strcat(timestamp, " ");
  strcat(timestamp, Load_Data[index_to_write].date);
	strptime(timestamp, "%H:%M:%S  %m/%d/%Y", &convert);  // convert the timestap string into time structure
	strftime (time_buffer,30,"%H:%M:%S",&convert);	// from converted time structure create separate time string
	strftime (date_buffer,30,"%m/%d/%Y",&convert);	// from converted time structure create separate date string
  strftime (print_buffer,40,"%H:%M:%S %m/%d/%Y",&convert);
	// Extracting int values for epoch timestamp construction
	hour = convert.tm_hour;
	minute = convert.tm_min;
	second = convert.tm_sec;
	month = convert.tm_mon+1; // time structure month is 0 based, so January == 0. Adding 1 to convert to normal format
	day = convert.tm_mday;
	year = convert.tm_year+1900+2000;	// year counting begins at 1900 for time structure

  //printf("date_str %s \n",  print_buffer);
  //printf(" %d \n", year);
	// Influxdb uses unix based timestamp. Creating it to timestamp the database entry properly
	long long nanoseconds_since_epoch = 1e9 * Get_Epoch_Timestamp(second, minute, hour, day, month, year);
  //printf("ns since epoch: %lld \n", nanoseconds_since_epoch);
  sprintf(epoch, "%lld", nanoseconds_since_epoch);
  //printf("epoch string %s \n", epoch);
	// putting elapsed time in a string for browser to display
	Seconds_To_Elapsed_Time_String(Load_Data[index_to_write].elapsed_secs, elapsed_time);
	long long elapsed_secs = Load_Data[index_to_write].elapsed_secs;

	influx_client_t c =
	{
		.host = strdup(HOST),
		.port = 8086,
		.db = strdup(DATABASE)
		//client.usr = strdup("usr");
		//client.pwd = strdup("pwd");
	};

  char Send[500];
  Send [0] = '\0';
	strcpy(Send, SENDLINE_HEADER);
	strcat(Send, "'");
	char* line = NULL;
	int len = 0;
	int used = 0;

	used = format_line(&line, &len, used,
			INFLUX_MEAS("FL100"),
			INFLUX_TAG("location", "burn-in"),
			INFLUX_F_FLT("V_CCC", Load_Data[index_to_write].CCC_V, 3),
			INFLUX_F_FLT("I_CCC", Load_Data[index_to_write].CCC_A, 4),
			INFLUX_F_FLT("W_CCC", Load_Data[index_to_write].CCC_W, 3),
			INFLUX_F_FLT("Dewar_K", Load_Data[index_to_write].diode_temp, 2),
			INFLUX_F_FLT("Diode_V", Load_Data[index_to_write].diode_volts, 4),
			INFLUX_F_FLT("V_Heat", Load_Data[index_to_write].Heat_Load_V, 2),
			INFLUX_F_FLT("I_Heat_mA", Load_Data[index_to_write].Heat_Load_mA, 2),
			INFLUX_F_FLT("I_Heat_mW", Load_Data[index_to_write].Heat_Load_mW, 2),
			INFLUX_F_FLT("THRMCPL1_C", Load_Data[index_to_write].thermocouple1_temp, 2),
			INFLUX_F_FLT("THRMCPL2_C", Load_Data[index_to_write].thermocouple2_temp, 2),
			INFLUX_F_FLT("THRMCPL3_C", Load_Data[index_to_write].thermocouple3_temp, 2),
			INFLUX_F_STR("TIME", time_buffer),
			INFLUX_F_STR("DATE", date_buffer),
			INFLUX_F_STR("Elapsed_Time", elapsed_time),
			//INFLUX_TS(nanoseconds_since_epoch),
			INFLUX_END
			);
      //printf("Amp  %f  \n", Load_Data[index_to_write].CCC_A);
        //    printf("Watt  %f  \n", Load_Data[index_to_write].CCC_W);
      strcat(Send, line);
      strcat(Send, " ");
      strcat(Send, epoch);
      //strcat(Send, "'");
      strcat(Send, "' >/dev/null 2>&1");
      //printf("  %s  \n", Send);
      system(Send);
      Send [0] = '\0';

	//post_http_send_line(&c, line, used);
}

void Record_Relay_ON()
{
	char write_line[] = "----------RELAY ON---------------------";
	file_pointer = fopen(SAVE_FILE, "a"); // 'w' means overwrite any existing file, "a" means to append to file
  fprintf(file_pointer, "%s\n", write_line); // write the current line to file
  fclose( file_pointer );
}

void Write_Data_To_File()
{
	char write_line[150];
	char buff_string[20];

	char CCC_V_string[15];
  char CCC_A_string[15];
  char CCC_W_string[15];;
  char diodeV_string[15];
  char diode_temp_string[15];
  char Rload_V_string[15];
  char Rload_mA_string[15];
  char Rload_mW_string[15];
  char thermocouple1_string[15];
  char thermocouple2_string[15];
  char thermocouple3_string[15];
	char timestamp_string[50];
	char elapsed_time_string[40];
  char relay_string[10];
	FILE* file_pointer;

  char time[30];
  char date[20];
  char elapsed_time[30];
  long long elapsed_secs;

	file_pointer = fopen(SAVE_FILE, "a"); // 'w' means overwrite any existing file, "a" means to append to file

  // Loop through all saved data points
	for (int i =0; i<save_index; ++i)
	{
      // Initialize all strings to "empty" strings:
    	CCC_V_string[0] = '\0';
      CCC_A_string[0] = '\0';
      CCC_W_string[0] = '\0';
      diodeV_string[0] = '\0';
      diode_temp_string[0] = '\0';
      Rload_V_string[0] = '\0';
      Rload_mA_string[0] = '\0';
      Rload_mW_string[0] = '\0';
      thermocouple1_string[0] = '\0';
      thermocouple2_string[0] = '\0';
      thermocouple3_string[0] = '\0';
    	timestamp_string[0] = '\0';
			elapsed_time_string[0] = '\0';
      write_line[0] = '\0'; // initialize to empty string (remove any existing characters)
      buff_string[0] = '\0';
      relay_string[0] = '\0';

      // Convert all float values to strings, to be written to file
      sprintf(CCC_V_string, " %2.3fV  ", Save_Data[i].CCC_V);
      if(Save_Data[i].CCC_V < 10)
      {
        buff_string[0] = ' '; // padding for less digits
        buff_string[1] = '\0'; // padding for less digits
        strcat(buff_string, CCC_V_string);
        strcpy(CCC_V_string, buff_string);
        CCC_V_string[strlen(buff_string)] = '\0';
      }
      sprintf(CCC_A_string, "%1.4fA  ", Save_Data[i].CCC_A);
      sprintf(CCC_W_string, " %2.3fW  ", Save_Data[i].CCC_W);
			sprintf(diode_temp_string, " %3.2fK  ", Save_Data[i].diode_temp);
    	sprintf(diodeV_string, " %1.4fV  ", Save_Data[i].diode_volts);
			sprintf(thermocouple1_string, "\t%2.2f  ", Save_Data[i].thermocouple1_temp);
			sprintf(thermocouple2_string, "  %2.2f  ", Save_Data[i].thermocouple2_temp);
			sprintf(thermocouple3_string, "  %2.2f   ", Save_Data[i].thermocouple3_temp);
      sprintf(Rload_V_string, " %2.3fV  ", Save_Data[i].Heat_Load_V);
      sprintf(Rload_mA_string, " %3.1fmA  ", Save_Data[i].Heat_Load_mA);
    	sprintf(Rload_mW_string, " %3.1fmW  ", Save_Data[i].Heat_Load_mW);

      strcpy(relay_string, "   \t");
			strcat(relay_string, Save_Data[i].relay_state);
      relay_string[strlen(relay_string)] = '\0';


			strcpy(timestamp_string, Save_Data[i].time);
      strcat(timestamp_string, " ");
      strcat(timestamp_string, Save_Data[i].date);
      strcat(timestamp_string, " \t");
      timestamp_string[strlen(timestamp_string)] = '\0';	// Add null terminating character

	    strcpy(elapsed_time_string, Save_Data[i].elapsed_time);
      elapsed_time_string[strlen(elapsed_time_string)] = '\0';	// Add null terminating character

      // Concatenate all the strings together into one line to write to the file
			strcpy(write_line, CCC_V_string);
			strcat(write_line, CCC_A_string);
      strcat(write_line, CCC_W_string);
			strcat(write_line, diode_temp_string); // first operation is a copy since write_line is empty
			strcat(write_line, diodeV_string);
      strcat(write_line, Rload_V_string);
      strcat(write_line, Rload_mA_string);
      strcat(write_line, Rload_mW_string);
      strcat(write_line, thermocouple1_string);
      strcat(write_line, thermocouple2_string);
      strcat(write_line, thermocouple3_string);
      strcat(write_line, timestamp_string);
			strcat(write_line, elapsed_time_string);
      strcat(write_line, relay_string);
			write_line[strlen(write_line)] = '\0';	// Add terminating character to the string

	 	  fprintf(file_pointer, "%s\n", write_line); // write the current line to file
	}
	fclose( file_pointer );
  save_index = 0;
}

void main()
{

	Initialize_Log("");

}
#endif
