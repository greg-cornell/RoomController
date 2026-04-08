/***********************************************************************
 * Subjet:	Called as a function from main program.  Used for reading
 * 		light switch gpio and controlling a relay based on it.
 *
 * Author:	Greg Cornell
 * 
 * Notes:	
 * 		
 * Open bugs:
 * 		(1)  Set output function on line 171 needs work.
 *
***********************************************************************/ 

/////////////////Includes///////////////////////////
#include <stdio.h> 
#include <pigpio.h> //library for better GPIO usage
#include <unistd.h> //sleep

////////////////Definitions/////////////////////////
#define OFF 1	//INx pins on relay board are high for off and low for on
#define ON 0	//Inx pins on relay board are high for off and low for on
#define PIN 12		//Input pin to read
#define POUT 18		//Output relay 1 to control
#define ACTIVE 0	//For input, active low
#define IN_ACTIVE 1	//For input, active high
#define READ_INT 0.1	//Interval to read the input.  Tested as low as 0.02!
//#define DEBUG

struct status_log_line {
	char time_stamp[20];
	int set_temp;
	int current_temp;
	int fan_status;
	int ac_status;
	int heat_status;
	int current_mode;
	int switch_status;
	int outlet_status;
};

void *switch_control(void * status_buffer)
{
	struct status_log_line *status_ptr;
	status_ptr = (struct status_log_line *) status_buffer;

	//////////////////////////Initialization of the program///////////////////////////////////////////////
	//Initialise GPIO Pins
#ifdef DEBUG
	printf("Initializing switch and output GPIO pins...");
#endif
	gpioSetMode(PIN, PI_INPUT);
	gpioSetPullUpDown(PIN, PI_PUD_UP);
	gpioSetMode(POUT, PI_OUTPUT);
#ifdef DEBUG
	printf("Switch and output GPIO pins enabled!\n");
#endif
	
	
	//////////////////////////Main program loop//////////////////////////////////////////////////////////
	while (1)
	{
		if (gpioRead(PIN) == IN_ACTIVE)		
		{
			//Write GPIO value to off for both Outlets
			gpioWrite(POUT, OFF); 
			status_ptr->switch_status = 0;
			status_ptr->outlet_status = 0;
#ifdef DEBUG
			printf("Switch gpioRead is %d so turning outlets off!\n",gpioRead(PIN));
#endif
		}
		else
		{
			//Write GPIO value to on for both LEDs
			gpioWrite(POUT, ON);
			status_ptr->switch_status = 1;
			status_ptr->outlet_status = 1;
#ifdef DEBUG
			printf("Switch gpioRead is %d so turning outlets on!\n",gpioRead(PIN));
#endif
		}
		
		time_sleep(READ_INT);
	}


	//////////////////////////Shutdown the program///////////////////////////////////////////////////////
	//Turn off the LEDs
	gpioWrite(POUT, OFF);
	/////////////////////////////////////////////////////////////////////////////////////////////////////
	pthread_exit(NULL);
}
