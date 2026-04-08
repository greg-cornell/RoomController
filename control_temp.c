/***********************************************************************
 * Subjet:	Module for controlling the temperature.  Called by main
 *
 * Author:	Greg Cornell
 * 
 * Notes:	
 *
 * Open bugs:	
 * 		
 *
***********************************************************************/ 


/////////////////Includes///////////////////////////
#include <stdio.h> 
#include <stdlib.h>	//needed for malloc
#include <pigpio.h> 	//library for better GPIO usage
#include <unistd.h>	//Sleep function
#include "rotary_encoder.h"	//For the rotary encoder reading
#include "control_temp.h"
#include "read_temp.h"

////////////////Definitions/////////////////////////
#define OFF 1	//INx pins on relay board are high for off and low for on
#define ON 0	//Inx pins on relay board are high for off and low for on
#define FAN_G 17	//Output to turn on the fan
#define HEAT_W 27	//Output to turn on the heat
#define AC_Y 22		//Output to turn on the AC (cool)
#define READ_INT 30	//Interval to read the input.  
#define ENC_A 24	//Pin 1 of the encoder
#define ENC_B 23	//Pin 2 of the encoder
#define ENC_IN 25	//Input pin of the encoders push button
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


///////////////Global variables////////////////////
int temp_set = 70;	//The temperature (in F) set point, initialized to 70

struct mode_data_s	//Structure for holding the mode data.  Structure needed to keep pigpio happy
{
	int mode;
};


/***************************************************	
 * Function for incrementing the encoder position
 * and printing to the screen
 * 
 * Inputs: +/-1 for which direction the encoder was rotated
 * Outputs: None
***************************************************/	
void callback(int way)
{
   	temp_set += way;

	printf("Temperature set to %d degrees F.\n", temp_set);
}

/***************************************************	
 * Function for cycling through the modes each time
 * the encoder button is pressed.  Called by pigpio
 *
 * Inputs:  GPIO that changed, that GPIO's new level
 * a time tick, and a pointer to user data.
 * Outputs: None
***************************************************/	
static void set_mode(int gpio, int level, uint32_t tick, void *user)
{
	mode_data_t *mode_data;

	mode_data = user;
	
#ifdef DEBUG
	printf("gpio %d level is %d\n", gpio, level);
#endif
	if (level == 0)
	{
		//Way of cycling back to 1 after 4 selected
		if (mode_data->mode == 4)
			mode_data->mode=1;
		else
			mode_data->mode++;

		switch (mode_data->mode)
		{
			case 1:
				printf("Fan selected!\n");
				break;
			case 2:
				printf("AC Selected!\n");
				break;
			case 3:
				printf("Heat Selected!\n");
				break;
			case 4:
				printf("Quit selected!\n");
				break;
		}
	}
	

	time_sleep(0.2);
}

/***************************************************	
 * Main function for thermostat control.  Launches 
 * thread monitoring encoder button.  Then launches 
 * thread monitoring encoder.  Uses data from these
 * threads to then correctly control thermostat output.
 * 
 * Inputs: Program level status buffer to logging
 * Outputs: None
***************************************************/	
void *set_thermostat(void * status_buffer)
{
	struct status_log_line *status_ptr;
	status_ptr = (struct status_log_line *) status_buffer;
	
	float current_temp;	//Current temperature, in degrees F from sensor
	//This seems a bit much to setup the mode, but I needed to do all this to get gpioSetAlertFuncEx to be ok with the user data passing in
	mode_data_t *mode_data_ptr;
	mode_data_ptr = malloc(sizeof(mode_data_t));
	mode_data_ptr->mode=1;


/***************************************************	
 * Initialize GPIO pins
***************************************************/	
#ifdef DEBUG
	printf("Initializing GPIO pins...");
#endif
	gpioSetMode(ENC_IN, PI_INPUT);
	gpioSetPullUpDown(ENC_IN, PI_PUD_UP);
	gpioSetMode(AC_Y, PI_OUTPUT);
	gpioSetMode(FAN_G, PI_OUTPUT);
	gpioSetMode(HEAT_W, PI_OUTPUT);
	//Initialize all thermostat outputs off
	gpioWrite(AC_Y, OFF);
	gpioWrite(FAN_G, OFF);
	gpioWrite(HEAT_W, OFF);	
#ifdef DEBUG
	printf("GPIO pins enabled!\n");
#endif


/***************************************************	
 * Initialize thread for reading the encoder button
***************************************************/	
	printf("Press button to cycle through modes...\n");
	gpioSetAlertFuncEx(ENC_IN, set_mode, mode_data_ptr);


/***************************************************	
 * Initialize thread for reading the encoder dial
***************************************************/	
	Pi_Renc_t *renc;
	renc = Pi_Renc(ENC_A, ENC_B, callback);


/***************************************************	
 * Take an initial current temperature read for logging
***************************************************/	
	ReadTemp(&current_temp);
	status_ptr->current_temp=(int)current_temp;
	status_ptr->set_temp=temp_set;


/***************************************************
 * Main program loop
***************************************************/	
	while (1)
	{

		switch (mode_data_ptr->mode)
		{
			case 1:	//Fan only
				status_ptr->current_mode=mode_data_ptr->mode;
#ifdef DEBUG
				printf("Fan was selected\n");
#endif
				//Turn only fan on
				gpioWrite(HEAT_W, OFF);
				gpioWrite(AC_Y, OFF);
				gpioWrite(FAN_G, ON);
				printf("Fan only on!\n");
				status_ptr->heat_status=0;
				status_ptr->ac_status=0;
				status_ptr->fan_status=1;
				time_sleep(READ_INT);
				continue;
		
			case 2:	//AC
				status_ptr->current_mode=mode_data_ptr->mode;
#ifdef DEBUG
				printf("AC was selected\n");
#endif
				ReadTemp(&current_temp);
				status_ptr->current_temp=(int)current_temp;
				status_ptr->set_temp=temp_set;
#ifdef DEBUG
				printf("current_temp=%d	temp_set=%d\n", (int)current_temp, temp_set);
#endif
				if (current_temp > temp_set)
				{
					//Turn fan and AC on, heat off
					gpioWrite(HEAT_W, OFF);
					gpioWrite(FAN_G, ON);
					gpioWrite(AC_Y, ON);
					status_ptr->heat_status=0;
					status_ptr->ac_status=1;
					status_ptr->fan_status=1;
					printf("%.0f is above set point of %d.  Fan on and AC running!\n", current_temp, temp_set);
				}
				else
				{
					//Turn off
					gpioWrite(HEAT_W, OFF);
					gpioWrite(FAN_G, OFF);
					gpioWrite(AC_Y, OFF);
					status_ptr->heat_status=0;
					status_ptr->ac_status=0;
					status_ptr->fan_status=0;
					printf("%.0f is within comfort zone of %d - Fan and AC off.\n", current_temp, temp_set);
				}
				time_sleep(READ_INT);
				continue;
			case 3:	//Heat
				status_ptr->current_mode=mode_data_ptr->mode;
#ifdef DEBUG
				printf("Heat was selected\n");
#endif
				ReadTemp(&current_temp);
				status_ptr->current_temp=(int)current_temp;
				status_ptr->set_temp=temp_set;
#ifdef DEBUG
				printf("current_temp=%d temp_set=%d\n", (int)current_temp, temp_set);
#endif

				if (current_temp < temp_set)
				{
					//Turn fan and heat on, AC off
					gpioWrite(AC_Y, OFF);
					gpioWrite(FAN_G, ON);
					gpioWrite(HEAT_W, ON);
					status_ptr->heat_status=0;
					status_ptr->ac_status=1;
					status_ptr->fan_status=1;
					printf("%.0f is a below set point of %d.  Fan on and heat running!\n", current_temp, temp_set);
				}
				else
				{
					//Turn off
					gpioWrite(HEAT_W, OFF);
					gpioWrite(FAN_G, OFF);
					gpioWrite(AC_Y, OFF);
					status_ptr->heat_status=0;
					status_ptr->ac_status=0;
					status_ptr->fan_status=0;
					printf("%.0f is within comfort zone of %d - Fan and heat off.\n", current_temp, temp_set);
				}
				time_sleep(READ_INT);
				continue;
				
			case 4:	//Quit program
				printf("Shutting down program...\nPress button to cancel!\n");
				int timer = 5;
				while(timer > 0)
				{
					printf("%d...\n", timer);
					sleep(1);
					timer--;
					if (mode_data_ptr->mode != 4)
						break;
				}
				if (mode_data_ptr->mode == 4)	
				{
					//Turn off thermostate before quitting
					gpioWrite(HEAT_W, OFF);
					gpioWrite(FAN_G, OFF);
					gpioWrite(AC_Y, OFF);
					status_ptr->heat_status=0;
					status_ptr->ac_status=0;
					status_ptr->fan_status=0;
					//This this here to only set mode to 4 if timer expires
					status_ptr->current_mode=mode_data_ptr->mode;
					break;
				}
				else
					continue;	
			
			default:
				printf("Error getting input\n");
				break;
			}
			break;
	}
		
	Pi_Renc_cancel(renc);
	pthread_exit(NULL);
}
