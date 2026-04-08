/***********************************************************************
 * Subjet:	This is the main module for the light switch program. 
 *		The light switch in this program is responsible for:
 *			- 0-100% light dimming controls
 *			- Room temperature control: reading temperature
 *			and sending open/close commands to vent or on/off
 *			commands to the thermostat
 *			- WiFi communication with thermostat
 *			- Some form of RF communction with the vent (TBD)
 *
 * Author:	Greg Cornell
 * 
 * To build:	make
 *
 * Notes:	10/31/15
 * 		This is the start of bringing together all the prototype and
 * 		and test modules I've been working on into one comprehensive
 * 		system.  'RoomController' project is my main starting point.	
 *		
 *		To-do:
 *		(1)  Rework room_controls.c for the new light and 
 *		temperature control 
 *		(2)  Rework all for the new I/O variables, functions, etc
 *
 * Open bugs:
 *
***********************************************************************/ 


/////////////////Includes///////////////////////////
#include <stdio.h> 
#include <pigpio.h> 		//library for better GPIO usage
#include <unistd.h>		//Sleep function
#include <pthread.h>		//For running threads
#include <time.h>		//For getting system time
#include "rotary_encoder.h"	//For the rotary encoder reading
#include "control_temp.h"	//For calling the temp control functions
#include "switch_outlet.h"	//For calling switch and relay control functions
////////////////Definitions/////////////////////////
#define LOG_INT 60	//Interval for capturing data in log
//#define DEBUG

/***************************************************	
 * Main function
****************************************************/ 
int main(int argc, char *argv[])
{
	//////////////////////////Initialization of the program///////////////////////////////////////////////
	pthread_t thread[2]; 	//Index for threads running
	time_t current_time;	//Variable for the current system time
	FILE *log_file;		//Log file for data
	struct status_log_line {
		char time_stamp[20];	//time stamp for record
		int set_temp;		//setpoint temperature 
		int current_temp;	//current temperature 
		int vent_status;	//Fan on/off status
		int current_mode;	//Current mode of the system
		int switch_status;	//Light switch status
		int outlet_status;	//Outlet status
	} status_buffer;

#ifdef DEBUG
	printf("Running in debug mode\n");
#else 
	printf("Running in production mode\n");
#endif

        //Initialise PiGPIO
#ifdef DEBUG
	printf("Initializing PiGPIO...");
#endif
	if (gpioInitialise() < 0)
	{
		fprintf(stderr, "pigpio initialisation failed\n");
	        return 1;
	}
#ifdef DEBUG
	else
	{
		printf("PiGPIO initialized.\n");
	}
#endif

	///////////////////////////Main program loop//////////////////////////////
	pthread_create(&thread[0], NULL, set_thermostat, (void *)&status_buffer);  	//Start the thermostat control thread
	
	sleep(1);	//Need the sleep here because main thread was executing faster than temp and switch thread

	//Open log file to append next record, print line, and close	
	log_file = fopen("light_switch_log.csv", "a");
	
	while(1)
	{
		//Capture the current system time just before printing status
		current_time = time(NULL);
		strftime(status_buffer.time_stamp, sizeof(status_buffer.time_stamp), "%Y-%m-%d %H:%M:%S", localtime(&current_time));
		
		//Print the data as a single line of comma seperated data
		fprintf(log_file, "%s,", status_buffer.time_stamp);
		fprintf(log_file, "%d,%d,", status_buffer.set_temp, status_buffer.current_temp);
		fprintf(log_file, "%d,%d,%d,%d,", status_buffer.vent_status, , status_buffer.current_mode);
		fprintf(log_file, "%d,%d\n", status_buffer.switch_status, status_buffer.outlet_status);
		
		//This is the interval to log data
		sleep(LOG_INT);

		if(status_buffer.current_mode==3)
			break;
	}
	

	//////////////////////////Shutdown the program///////////////////////////////////////////////////////
	//Wait for threads to exit before shutting down
	pthread_join(thread[0], NULL);
	
	//Close the log file
	fclose(log_file);
	
	//Shut down PiGPIO
	gpioTerminate();
	
	pthread_exit(NULL);
	
	return(0);
	/////////////////////////////////////////////////////////////////////////////////////////////////////
}
