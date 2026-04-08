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
#include "room_controls.h"
#include "read_temp.h"

////////////////Definitions/////////////////////////
#define OFF 1		//INx pins on relay board are high for off and low for on
#define ON 0		//INx pins on relay board are high for off and low for on
#define READ_INT 30	//Read interval (seconds)  
#define ENC_A 24	//Pin 1 of the encoder
#define ENC_B 23	//Pin 2 of the encoder
#define ENC_IN 25	//Input pin of the encoders push button
#define SWITCH_OUT 18	//GPIO to be used for P1 swith output to light (change to digi pot)
#define FAKE_VENT 17	//GPIO to be used for P1 on/off control of vent (change to RF msg)
#define DEBUG
/****************************************	
 * The following defines are used for thermostat.
 * Keeping them in here so that I remember which pins
 * are used for that application to make switching 
 * between Light Switch and Thermostat on same RPi easier
#define FAN_G 17	//Output to turn on the fan
#define HEAT_W 27	//Output to turn on the heat
#define AC_Y 22		//Output to turn on the AC (cool)
****************************************/	

struct status_log_line {
	char time_stamp[20];
	int set_temp;
	int current_temp;
	int vent_status;
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

#ifdef DEBUG
	printf("Temperature set to %d degrees F.\n", temp_set);
#endif
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
	
	if (level == 0)
	{
		//Way of cycling back to 1 after 4 selected
		if (mode_data->mode == 3)
			mode_data->mode=1;
		else
			mode_data->mode++;

		switch (mode_data->mode)
		{
			case 1:
#ifdef DEBUG
				printf("Light control selected!\n");
#endif
				break;
			case 2:
#ifdef DEBUG
				printf("Temperature control selected!\n");
#endif
				break;
			case 3:
#ifdef DEBUG
				printf("Quit selected!!\n");
#endif
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
	gpioSetMode(ENC_IN, PI_INPUT);
	gpioSetPullUpDown(ENC_IN, PI_PUD_UP);
	gpioSetMode(SWITCH_OUT, PI_OUTPUT);
	gpioSetMode(FAKE_VENT, PI_OUTPUT);

	//Initialize all thermostat outputs off
	gpioWrite(SWITCH_OUT, OFF);
	gpioWrite(FAKE_VENT, OFF);


/***************************************************	
 * Initialize thread for reading the encoder button
***************************************************/	
#ifdef DEBUG
	printf("Press button to cycle through modes...\n");
#endif
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
			case 1:	//Light control
				status_ptr->current_mode=mode_data_ptr->mode;
#ifdef DEBUG
				printf("Light control mode selected\n");
#endif
/****************************************	
 * Need to rework this for light control
****************************************/	
				gpioWrite(HEAT_W, OFF);
				gpioWrite(AC_Y, OFF);
				gpioWrite(FAN_G, ON);
				printf("Fan only on!\n");
				status_ptr->heat_status=0;
				status_ptr->ac_status=0;
				status_ptr->vent_status=1;
				time_sleep(READ_INT);
				continue;
/****************************************	
****************************************/	
		
			case 2:	//Temperature control mode
				status_ptr->current_mode=mode_data_ptr->mode;
#ifdef DEBUG
				printf("Temperature control mode was selected\n");
#endif
				ReadTemp(&current_temp);
				status_ptr->current_temp=(int)current_temp;
				status_ptr->set_temp=temp_set;
#ifdef DEBUG
				printf("current_temp=%d temp_set=%d\n", (int)current_temp, temp_set);
#endif

/****************************************
 * Need to rework this for light switch/vent/thermostat temp control
****************************************/	
				if (current_temp < temp_set)
				{
					//Turn fan and heat on, AC off
					gpioWrite(AC_Y, OFF);
					gpioWrite(FAN_G, ON);
					gpioWrite(HEAT_W, ON);
					status_ptr->heat_status=0;
					status_ptr->ac_status=1;
					status_ptr->vent_status=1;
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
					status_ptr->vent_status=0;
					printf("%.0f is within comfort zone of %d - Fan and heat off.\n", current_temp, temp_set);
				}
				time_sleep(READ_INT);
				continue;
/****************************************
****************************************/	
				
			case 3:	//Quit program
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
					gpioWrite(SWITCH_OUT, OFF);
					gpioWrite(FAKE_VENT, OFF);
					status_ptr->vent_status=0;
					//This this here to only set mode to 3 if timer expires
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
