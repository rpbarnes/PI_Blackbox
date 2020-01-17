#include <fcntl.h>  /* File Control Definitions          */
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>


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



void Open_Log_File()
{
    //Check to see if we have a thumbdrive inserted by performing ls /dev/sda.  If so we will datalog to USB drive
    //If not, datalog is written to /home/pi/Logs/
    char fileName[256];
    int thumb = system("ls /dev/sda"); //thumb = 0 if thumbdrive detected or 512 if none available
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
    if(thumb==0)sprintf(fileName, "%s/%s%s", USB_FILEPATH, date_buffer, SAVE_FILE_SUFFIX);         // save logfile on USB Drive
    else sprintf(fileName, "%s/%s%s", SD_FILEPATH, date_buffer, SAVE_FILE_SUFFIX);     // else on SD card
    strcpy(SAVE_FILE, fileName);
    printf("save file:  %s\n", SAVE_FILE);
    file_pointer = fopen(SAVE_FILE, "a");
    fclose( file_pointer );
}


void Calc_Averages()
{

  char buffer[50];
  time_t elapsed_time;
  time_t loop_time;

  elapsed_time = loop_start - log_tm_start;
  long long elapsed_secs = (long long)elapsed_time;

  strftime (buffer,30,"%H:%M:%S", loop_tm_start);   // create a string from time structure
  strncpy(Save_Data[save_index].time, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] timestamp string

  strftime (buffer,20,"%m/%d/%y", loop_tm_start);   // create a string from time structure
  strncpy(Save_Data[save_index].date, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] timestamp string


  Seconds_To_Elapsed_Time_String(elapsed_secs, buffer); // returns an hours min sec string for the elapsed secs, stores the string in buffer
  strncpy(Save_Data[save_index].elapsed_time, buffer, (int)strlen(buffer));        // copy the string to the Save_Data[] elapsed_time string
  Save_Data[save_index] .elapsed_secs = elapsed_secs;
  Save_Data[save_index].CCC_raw_V = Calc_Metric_Raw_Average(Volt_Metrics, index_now.volt_sample);
  Save_Data[save_index].CCC_V = Calc_Metric_Corrected_Average(Volt_Metrics, index_now.volt_sample);
  Save_Data[save_index].CCC_raw_A = Calc_Metric_Raw_Average(Amp_Metrics, index_now.amp_sample);
  Save_Data[save_index].CCC_A = Calc_Metric_Corrected_Average(Amp_Metrics, index_now.amp_sample);
  Save_Data[save_index].CCC_W = Save_Data[save_index].CCC_A * Save_Data[save_index].CCC_V;
  //Save_Data[save_index].diode_volts = Calc_Metric_Raw_Average(Diode_Metrics, index_now.diode_sample);
  //Save_Data[save_index].diode_temp = Calc_Metric_Corrected_Average(Diode_Metrics, index_now.diode_sample);

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
    char DUT_ID[30];
    char Blackbox_SN[30];
    char Displacer_SN[30];
    char CCC_SN[30];
    char Cooler_SN[30];
    char Profile_SN[30];

    char* DUT = "\t\t DUT_ID: XXXX \n";
    char* Blackbox_Header = "\t\t\t\t Blackbox SN: ";
    char* Displacer_Header = "\t\t\t\t Displacer SN: ";
    char* CCC_Header = "\t\t\t\t CCC SN: ";
    char* Cooler_Header = "\t\t\t\t Cooler SN: ";
    char* Profile_Header = "\t\t\t\t Profile SN: ";

		char* file_header1 = "  CCC \t   CCC \t    CCC      Dewar \t\t RLoad \t RLoad\t   R_PWR \t    THRMCPL";
    char* file_header2 = "   V \t    A \t     W \t      degK    diodeV\t  V\t   mA\t     mW \t C \t C \t C \t  Time\t Date\t\t  Elapsed_Time";
    char* line = "-------------------------------------------------------------------------------------------------------------------------------------------\n";

    Get_Date(date_buffer);
    date_buffer[strlen(date_buffer)] = '\0';	// Add null terminating character


		Get_TimeStamp(timestamp_str);
    timestamp_str[strlen(timestamp_str)] = '\0';

		for(int i=0; i<strlen(date_buffer); i++)
		{
			if(date_buffer[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
			{	date_buffer[i] = '-';	}
		}


	for(int i=0; i<strlen(timestamp_str); i++)
	{
		if(timestamp_str[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
		{	timestamp_str[i] = '-';	}
	}
	timestamp_str[strlen(timestamp_str)] = '\0';	// Add null terminating character

    Blackbox_SN[0] = '\0';
    strcpy(Blackbox_SN, Blackbox_Header);
    strcat(Blackbox_SN, "XXX");
    Blackbox_SN[strlen(Blackbox_SN)] = '\0';

    Displacer_SN[0] = '\0';
    strcpy(Displacer_SN, Displacer_Header);
    strcat(Displacer_SN, "XXX");
    Displacer_SN[strlen(Displacer_SN)] = '\0';

    Cooler_SN[0] = '\0';
    strcpy(Cooler_SN, Cooler_Header);
    strcat(Cooler_SN, "XXX");
    Cooler_SN[strlen(Cooler_SN)] = '\0';

    CCC_SN[0] = '\0';
    strcpy(CCC_SN, CCC_Header);
    strcat(CCC_SN, "XXX");
    CCC_SN[strlen(CCC_SN)] = '\0';

    Profile_SN[0] = '\0';
    strcpy(Profile_SN, Profile_Header);
    strcat(Profile_SN, "XXX");
    Profile_SN[strlen(Profile_SN)] = '\0';
	  file_pointer = fopen(SAVE_FILE, "a"); // open for writing, 'w' means overwrite any existing file, "a" means to append to file
    fprintf(file_pointer, "\n\t\t%s\n", Blackbox_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", Displacer_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", Cooler_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n", CCC_SN); // write date to file
    fprintf(file_pointer, "\t\t%s\n\n", Profile_SN); // write date to file
		fprintf(file_pointer, "\t\t\t\t\t\t%s\n\n", timestamp_str); // write date to file
    fprintf(file_pointer, "%s\n", file_header1); // write the header1 to file
		fprintf(file_pointer, "%s\n", file_header2); // write the header2 to file
    fprintf(file_pointer, "%s\n", line); // write the current divider line (-------) to file
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
/*
      sprintf(Rload_V_string, " %2.3fV  ", Save_Data[i].Heat_Load_V);
      sprintf(Rload_mA_string, " %3.1fmA  ", Save_Data[i].Heat_Load_mA);
    	sprintf(Rload_mW_string, " %3.1fmW  ", Save_Data[i].Heat_Load_mW);
*/
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

      strcat(write_line, Rload_V_string);
      strcat(write_line, Rload_mA_string);
      strcat(write_line, Rload_mW_string);

      strcat(write_line, timestamp_string);
			strcat(write_line, elapsed_time_string);
			write_line[strlen(write_line)] = '\0';	// Add terminating character to the string

	 	  fprintf(file_pointer, "%s\n", write_line); // write the current line to file
	}
	fclose( file_pointer );
  save_index = 0;
}
