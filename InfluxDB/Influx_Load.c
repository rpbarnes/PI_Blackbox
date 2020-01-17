#define _USE_XOPEN 700
#define _GNU_SOURCE


#include <fcntl.h>  /* File Control Definitions          */
#include <errno.h>
#include <time.h>
#include <locale.h>
#include <sys/ioctl.h>
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

#define MAX_SAMPLES 100
#define SAVE_FILE "11-09-19_data2.txt"
#define NEW_FILE "11-09-19_data2.txt"
FILE* file_pointer;
FILE* new_file;

struct data_point
{
		char timestamp[50];
    char elapsed_time[30];
		long long elapsed_secs;

    double CCC_V;
    double CCC_raw_V;
		double CCC_raw_A;
    double CCC_A;
    double CCC_W;
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
char time_buffer[100];
char date_buffer[20];
Data_Point Save_Data[MAX_SAMPLES+100]; // room for 100 Data Points saved in memory before being written to file
Data_Point AVG_Data[15]; // room for 100 Data Points saved in memory before being written to file
Data_Point Point;
int save_index; // keeps track of how many data points in memory (not written to file yet)

char* Get_TimeStamp(char* buffer);
char* Get_Date(char* buffer);
void Initialize_Log(char* dut_id);
void Write_Data_to_File();
void Write_Data_To_Influx(Data_Point* Load_Data, int index_to_write);
long int Get_Epoch_Timestamp(int second, int minute, int hour, int day, int month, int year);
void Seconds_To_Elapsed_Time_String(long int total_seconds, char* string_buffer);

void Read_From_File(void)
{
  file_pointer = fopen(SAVE_FILE, "r"); // "r" for read only
  //new_file = fopen(NEW_FILE, "w");

  char line[300];
  char V_CCC[15];
  char I_CCC[15];
  char W_CCC[15];
  char DiodeK[15];
  char DiodeV[15];
  char I_Heat[15];
  char W_Heat[15];
  char THRMCPL1[15];
  char THRMCPL2[15];
  char THRMCPL3[15];
  char TIME[20];
  char DATE[15];
	char START_TIMESTAMP[100];
	char entry_timestamp[50];
	struct tm convert = {0};
	struct tm mod;
	struct tm entry = {0};
	time_t start_time, entry_time, elapsed_time;
	long long elapsed_secs;

	// First Scan the File Header to Get Log Start Time
	bool Read_File_Header = false;
	char* HEADER_ENTRY 			= "##";
	char* START_TIME_ENTRY 	= "###";		// Identifier for start time in log file
	char* LINE_DELIM 				= "-----------";
	char* contains;
	char temp[20];
	char year_timestamp[20];
	int lines_read = 0;
  while( (fgets(line, 256, file_pointer) != NULL) && lines_read <10 && !Read_File_Header)
	{
			++lines_read;
			contains = strstr(line, START_TIME_ENTRY);
			//printf("%s\n", line);
			//printf("contains start: %s\n", contains);
			if(contains)
			{
					sscanf(line, " %s %s %s", temp, START_TIMESTAMP, year_timestamp);

					for(int i=0; i<strlen(year_timestamp); ++i)
					{
						if(year_timestamp[i] == '-') year_timestamp[i] = '/';
					}

					int day, month, year;
					//strcat(START_TIMESTAMP, "11/09/19");
					strcat(START_TIMESTAMP, year_timestamp);
					printf("start string: %s\n", START_TIMESTAMP);
					strptime(START_TIMESTAMP, "%H:%M:%S  %m/%d/%Y", &convert);  // convert the timestap string into time structure
					if(convert.tm_year <0)
					{
							convert.tm_year += 2000;
					}
					printf("start year: %d\n", convert.tm_year);
					printf("start year: %d\n", convert.tm_mon);
					printf("start year: %d\n", convert.tm_mday);
					start_time = mktime(&convert);
					//printf("start time %ld\n", start_time);

			}
			contains = strstr(line, LINE_DELIM);
			if(contains)
			{
				Read_File_Header = true;
				//printf("read file header");
			}
	}

	lines_read = 0;
  while( fgets(line, 256, file_pointer) != NULL)
  {
			++lines_read;
			// Scan a line from the file and parse into the appropriate fields
      sscanf(line, "%s %s %s %s %s %s %s %s %s %s %s %s", V_CCC, I_CCC, W_CCC, DiodeK, DiodeV, I_Heat, W_Heat, THRMCPL1, THRMCPL2, THRMCPL3, TIME, DATE );
			Point.diode_temp = strtod(DiodeK, NULL);
			Point.diode_volts = strtod(DiodeV, NULL);
			Point.CCC_V = strtod(V_CCC, NULL);
			Point.CCC_A = strtod(I_CCC, NULL);
			Point.CCC_W = strtod(W_CCC, NULL);
			Point.Heat_Load_V = 0;
			Point.Heat_Load_mA = strtod(I_Heat, NULL);
			Point.Heat_Load_mW = strtod(W_Heat, NULL);
			Point.thermocouple1_temp = strtod(THRMCPL1, NULL);
			Point.thermocouple2_temp = strtod(THRMCPL2, NULL);
			Point.thermocouple3_temp = strtod(THRMCPL3, NULL);
			strcpy(Point.timestamp, TIME);
			strcat(Point.timestamp, " ");
			strcat(Point.timestamp, DATE);

			strcpy(entry_timestamp, Point.timestamp);
			strptime(entry_timestamp, "%H:%M:%S  %m/%d/%Y", &entry);  // convert the timestap string into time structure
			if(entry.tm_year <0)
			{
					entry.tm_year += 2000; // if only a 2 digit year is in the timestamp
			}
			entry_time = mktime(&entry);

			elapsed_time = entry_time - start_time;
			elapsed_secs = (long long)elapsed_time;
			Point.elapsed_secs = elapsed_secs;
			//printf("elapsed secs %lld\n", elapsed_secs);
			Write_Data_To_Influx(&Point, 0);
      //fprintf(new_file, " %s  %s  %s \t %sK  %sdV \t %smA  %smW \t %sC1  %sC2  %sC3\t %s  %s\n", V_CCC, I_CCC, W_CCC, DiodeK, DiodeV, I_Heat, W_Heat, THRMCPL1, THRMCPL2, THRMCPL3, TIME, DATE );
  }
	printf("Entries Added: %d\n", lines_read);
}

long int Get_Epoch_Timestamp(int second, int minute, int hour, int day, int month, int year)
{
		struct tm t;
		time_t dt_time;

		if(year <2000) year += 2000;
		t.tm_year = year - 1900;
		t.tm_mon = month -1; // where january = 0
		t.tm_mday = day;          // Day of the month
		t.tm_hour = hour;
		t.tm_min = minute;
		t.tm_sec = second;
		t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
		dt_time = mktime(&t);

		return (long long)dt_time;
}

void Seconds_To_Elapsed_Time_String(long int total_seconds, char* string_buffer)
{
		int display_seconds = total_seconds % 60;
		int total_minutes   = total_seconds / 60;
		int display_minutes = total_minutes % 60;
		int display_hours 		= total_minutes / 60;

		sprintf(string_buffer, "%dhr %dmin %dsec", display_hours, display_minutes, display_seconds);
}

void Write_Data_To_Influx(Data_Point* Load_Data, int index_to_write)
{
	int second, minute, hour, day, month, year;
	char elapsed_time[30];
	char time_buffer[30];
	char date_buffer[20];
	struct tm convert = {0};

	strptime(Load_Data[index_to_write].timestamp, "%H:%M:%S  %m/%d/%Y", &convert);  // convert the timestap string into time structure
	strftime (time_buffer,30,"%H:%M:%S",&convert);	// from converted time structure create separate time string
	strftime (date_buffer,30,"%m/%d/%Y",&convert);	// from converted time structure create separate date string

	// Extracting int values for epoch timestamp construction
	hour = convert.tm_hour;
	minute = convert.tm_min;
	second = convert.tm_sec;
	month = convert.tm_mon+1; // time structure month is 0 based, so January == 0. Adding 1 to convert to normal format
	day = convert.tm_mday;
	year = convert.tm_year+1900;	// year counting begins at 1900 for time structure

	// Influxdb uses unix based timestamp. Creating it to timestamp the database entry properly
	long long nanoseconds_since_epoch = 1e9 * Get_Epoch_Timestamp(second, minute, hour, day, month, year);

	// putting elapsed time in a string for browser to display
	Seconds_To_Elapsed_Time_String(Load_Data[index_to_write].elapsed_secs, elapsed_time);
	long long elapsed_secs = Load_Data[index_to_write].elapsed_secs;

	influx_client_t c =
	{
		.host = strdup(HOST),
		.port = PORT,
		.db = strdup(DATABASE)
		//client.usr = strdup("usr");
		//client.pwd = strdup("pwd");
	};

	char Send[500];
	char* sendline_header = "curl -i -XPOST 'http://192.168.250.111:8086/write?db=pi_data' --data-binary ";
	strcpy(Send, sendline_header);
	strcat(Send, "'");
	char* line = NULL;
	int len = 0;
	int used = 0;

	used = format_line(&line, &len, used,
			INFLUX_MEAS("FL100"),
			INFLUX_TAG("location", "burn-in"),
			INFLUX_F_FLT("V_CCC", Load_Data[index_to_write].CCC_V, 3),
			INFLUX_F_FLT("I_CCC", Load_Data[index_to_write].CCC_A, 4),
			INFLUX_F_FLT("W_CCC", Load_Data[index_to_write].CCC_W, 2),
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
			//INFLUX_F_INT("Elapsed_Secs", elapsed_secs),
			INFLUX_TS(nanoseconds_since_epoch),
			INFLUX_END
			);
			printf("%d\n", used);
			printf("%s\n", line);
	strcat(Send, line);
	strcat(Send, "'");
	system(Send);
	post_http_send_line(&c, line, used);
}

int main()
{
	char timestamp[50];
	Point.elapsed_secs = 10;
	Point.diode_temp = 67.061;
	Point.diode_volts = 1.0578;
	Point.CCC_V = 13.055;
	Point.CCC_A = .578;
	Point.CCC_W = 7.544;
	Point.Heat_Load_V = 0;
	Point.Heat_Load_mA = 0;
	Point.Heat_Load_mW = 0;
	Point.thermocouple1_temp = 22.711;
	Point.thermocouple2_temp = 24.891;
	Point.thermocouple3_temp = 22.906;
	strcpy(Point.timestamp, "00:45:27  11/25/19");

	Write_Data_To_Influx(&Point, 0);

	//Write_Data_To_Influx();
	//printf("Source: 20:56:35 11/09/2019\n");
	//long int epoch = Get_Epoch_Timestamp(35, 56, 20, 9, 11, 2019);
	//printf("epoch Secs:\t%ld\n", epoch);
	//epoch *= 1e9;
	//printf("epoch NSecs:\t%ld\n", epoch);

	//struct tm result = {0};

	//strptime("20:56:35 11/09/2019", "%H:%M:%S  %m/%d/%Y", &result);
	//printf("hour: %d\n", result.tm_hour);
	//printf("minute: %d\n", result.tm_min);
	//printf("sec: %d\n", result.tm_sec);
	//printf("month: %d\n", result.tm_mon+1);
	//printf("day: %d\n", result.tm_mday);
	//printf("year: %d\n", result.tm_year+1900);


	char elapsed_time[30];
	Seconds_To_Elapsed_Time_String(27000, elapsed_time);

	//printf("elapsed time: %s\n", elapsed_time);
//Read_From_File();

}
