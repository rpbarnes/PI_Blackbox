#include <fcntl.h>  /* File Control Definitions          */
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define SAVE_FILE_SUFFIX "_data.txt"  // file to store reading data

#define MAX_SAMPLES 100
char SAVE_FILE[50];
char DUT_ID[20];
FILE* file_pointer;

struct data_point
{
		char timestamp[50];
		long int diff_micros;
		long int diff_millis;
    double diode_temp;
    double INA233_CCC_V;
    double INA233_CCC_A;
    double INA233_CCC_W;
    double ADS1256_temp_diode_volts;
		double INA233_heat_load_V;
		double INA233_heat_load_A;
		double INA233_heat_load_W;
    double thermocouple_temp;
};

typedef struct data_point Data_Point; // making an instance of the data_point struct
char time_buffer[100];
char date_buffer[20];
Data_Point Save_Data[MAX_SAMPLES +100]; // room for 100 Data Points saved in memory before being written to file
int save_index = 0; // keeps track of how many data points in memory (not written to file yet)


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

// Must be called before storing any data points
// Calling this will overwrite any file with same filename  (same data and unitID)
// the argument is a string identifying this specific cooler under test
void Initialize_Log(char* dut_id)
{

//	TODO:
//	Possibly Check if file exists already for todays date, if so dont overwrite file
//
    char filename[30];
		printf("Enter Log File Name:\n >> ");
		scanf("%s", filename);

		char* file_header1 = " REF200\t\tREF200 \t\t CCC\t CCC\t CCC\t\t RLoad\t RLoad\t R_PWR\t\t THRMCPL";
    char* file_header2 = " Diode_Temp\tDiode_V \t   V\t   A\t   W\t\t   V\t   mA\t  W\t\t   C   \t\t Time\t   Date\t\telapsed ms";
    char* line = "----------------------------------------------------------------------------------------------------------------------------------------\n";

    Get_Date(date_buffer);
    date_buffer[strlen(date_buffer)] = '\0';	// Add null terminating character
		for(int i=0; i<strlen(date_buffer); i++)
		{
			if(date_buffer[i] == '/') // remove slashes from date and replace with dashes so compatible with filename
			{	date_buffer[i] = '-';	}
		}

		strcpy(DUT_ID, dut_id);
    DUT_ID[strlen(DUT_ID)] = '\0';	// Add null terminating character

    //strcpy(filename, DUT_ID); // first operation is a copy since filename is empty
    strcat(filename, "_");
		strcat(filename, date_buffer);

		printf("filename: %s\n", filename);



    strcat(filename, SAVE_FILE_SUFFIX);
		printf("save file: %s\n", filename);
    strcpy(SAVE_FILE, filename);

	  file_pointer = fopen(SAVE_FILE, "w"); // open for writing, 'w' means overwrite any existing file, "a" means to append to file
    fprintf(file_pointer, "%s\n", date_buffer); // write date to file
    fprintf(file_pointer, "%s\n", file_header1); // write the header1 to file
		fprintf(file_pointer, "%s\n", file_header2); // write the header2 to file
    fprintf(file_pointer, "%s\n", line); // write the current divider line (-------) to file
	  fclose( file_pointer );
}


char* Get_TimeStamp(char* buffer)
{
	 time_t rawtime;
	 struct tm* timeinfo;
	 time (&rawtime);
	 timeinfo = localtime(&rawtime);
	 strftime (time_buffer,30,"%H:%M:%S  %m/%d/%y",timeinfo);
	 strncpy(buffer, time_buffer, (int)strlen(time_buffer));

	 return buffer;
}



void Write_Data_to_File()
{
	char write_line[150];
  char diode_temp_string[15];
	char INA233_CCC_V_string[15];
  char INA233_CCC_A_string[15];
  char INA233_CCC_W_string[15];;
  char ADS1256_diodeV_string[15];
  char Rload_voltage_string[15];
  char Rload_current_string[15];
  char Rload_power_string[15];
  char thermocouple_string[15];
	char timestamp_string[50];
	char elapse_time_string[15];
	FILE* file_pointer;

	file_pointer = fopen(SAVE_FILE, "a"); // 'w' means overwrite any existing file, "a" means to append to file

  // Loop through all saved data points
	for (int i =0; i<save_index; ++i)
	{

      // Initialize all strings to "empty" strings:
      diode_temp_string[0] = '\0';
    	INA233_CCC_V_string[0] = '\0';
      INA233_CCC_A_string[0] = '\0';
      INA233_CCC_W_string[0] = '\0';
      ADS1256_diodeV_string[0] = '\0';
      Rload_voltage_string[0] = '\0';
      Rload_current_string[0] = '\0';
      Rload_power_string[0] = '\0';
      thermocouple_string[0] = '\0';
    	timestamp_string[0] = '\0';
			elapse_time_string[0] = '\0';

      // Convert all float values to strings, to be written to file
			sprintf(diode_temp_string, " %3.3f\t\t", Save_Data[i].diode_temp);
      sprintf(INA233_CCC_V_string, " %2.3f\t", Save_Data[i].INA233_CCC_V);
      sprintf(INA233_CCC_A_string, "%1.3f\t", Save_Data[i].INA233_CCC_A);
      sprintf(INA233_CCC_W_string, " %2.3f\t\t", Save_Data[i].INA233_CCC_W);
    	sprintf(ADS1256_diodeV_string, " %2.5f\t", Save_Data[i].ADS1256_temp_diode_volts);
      sprintf(Rload_voltage_string, " %2.3f\t", Save_Data[i].INA233_heat_load_V);
      sprintf(Rload_current_string, " %1.3f\t", Save_Data[i].INA233_heat_load_A);
    	sprintf(Rload_power_string, " %1.3f\t", Save_Data[i].INA233_heat_load_W);
      sprintf(thermocouple_string, "\t %2.3f\t", Save_Data[i].thermocouple_temp);
			sprintf(elapse_time_string, "\t %d ms", Save_Data[i].diff_millis);

			strcpy(timestamp_string, Save_Data[i].timestamp);
      timestamp_string[strlen(timestamp_string)] = '\0';	// Add null terminating character
			//timestamp_string[18] = '\0'; // terminate the end of the timestamp string
			write_line[0] = '\0'; // initialize to empty string (remove any existing characters)

      // Concatenate all the strings together into one line to write to the file
			strcpy(write_line, diode_temp_string); // first operation is a copy since write_line is empty
			strcat(write_line, ADS1256_diodeV_string);
			strcat(write_line, INA233_CCC_V_string);
			strcat(write_line, INA233_CCC_A_string);
      strcat(write_line, INA233_CCC_W_string);
      strcat(write_line, Rload_voltage_string);
      strcat(write_line, Rload_current_string);
      strcat(write_line, Rload_power_string);
      strcat(write_line, thermocouple_string);
      strcat(write_line, timestamp_string);
			strcat(write_line, elapse_time_string);
			write_line[strlen(write_line)] = '\0';	// Add terminating character to the string
//printf("file writeline: %s\n", write_line); // write the current line to file
	 	  fprintf(file_pointer, "%s\n", write_line); // write the current line to file
	}
	fclose( file_pointer );
}
