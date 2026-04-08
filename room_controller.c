/***********************************************************************
 * Subjet:	A combination of earlier projects "Switch" and "Thermostat".
 * 		plan is to use this to actually control my office. 
 *
 * Author:	Greg Cornell
 * 
 * To build:	make
 *
 * Notes:	To-do:
 * 		1)  Combine "Switch" and "Thermostat" into one application. 
 * 		Light switch controls one relay and then rotary encoder
 * 		controls a thermostat. - done
 * 		1a) Figure out infinite loop for light switch, but with
 * 		some way of exiting. - done
 * 		2)  Add in temperature logging - done
 * 		3)  Integrate web server control
 * 		4)  Add connectivity between two RPi's
 *
 *		10/21/15:  Tonight I merged the two Switch and Thermostat
 *		applications.  Problem I had is that function calls were done
 *		in series, so you couldn't control temperature while also
 *		controlling the switch.  Just started playing around with
 *		pthreads, but it got real late.  Pick up here tomorrow.
 *
 *		10/22/15:  Got pthreads working great.  Able to setup
 *		independent threads and learned about pthreads_join
 *		to block the main thread until required threads exit.
 *
 *		Also started working on the logging.  Able to create and
 *		append records to CSV file.  Created a structure for storing
 *		all the data and then can use fprintf to print to new line
 *		in CSV file.  I started trying to figure out how to pass
 *		a pointer around to the various threads for them to write to
 *		the elements within the structure, but couldn't get it to work.
 *		Started with switch_control.c and commented out my first attemp.
 *
 *		10/23/15:  Passing structure around now working.  This is exciting
 *		for a lot of other applications.  Have logging also working now
 *		with the exception of the small bugs 1 and 2 noted below.  
 *		Very exciting progress!
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
		int fan_status;		//Fan on/off status
		int ac_status;		//AC on/off status
		int heat_status;	//Heat on/off status
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
	pthread_create(&thread[1], NULL, switch_control, (void *)&status_buffer);		//Start the switch control thread
	
	sleep(1);	//Need the sleep here because main thread was executing faster than temp and switch thread

	//Open log file to append next record, print line, and close	
	log_file = fopen("room_controller_log.csv", "a");
	
	while(1)
	{
		//Capture the current system time just before printing status
		current_time = time(NULL);
		strftime(status_buffer.time_stamp, sizeof(status_buffer.time_stamp), "%Y-%m-%d %H:%M:%S", localtime(&current_time));
		
		//Print the data as a single line of comma seperated data
		fprintf(log_file, "%s,", status_buffer.time_stamp);
		fprintf(log_file, "%d,%d,", status_buffer.set_temp, status_buffer.current_temp);
		fprintf(log_file, "%d,%d,%d,%d,", status_buffer.fan_status, status_buffer.ac_status, status_buffer.heat_status, status_buffer.current_mode);
		fprintf(log_file, "%d,%d\n", status_buffer.switch_status, status_buffer.outlet_status);
		
		//This is the interval to log data
		sleep(LOG_INT);

		if(status_buffer.current_mode==4)
			break;
	}
	

	//////////////////////////Shutdown the program///////////////////////////////////////////////////////
	//Wait for threads to exit before shutting down
	pthread_join(thread[0], NULL);
	//pthread_join(thread[1], NULL);	//commenting out so Quit on thermostat kills applicaton
	pthread_cancel(thread[1]);		//Cancel the switch control thread
	
	//Close the log file
	fclose(log_file);
	
	//Shut down PiGPIO
	gpioTerminate();
	
	pthread_exit(NULL);
	
	return(0);
	/////////////////////////////////////////////////////////////////////////////////////////////////////
}
