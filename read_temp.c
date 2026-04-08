
/***********************************************************************
 * Subjet:	Module for reading the temperature sensor.  Originally,
 * 		this was right in the main program, but broke it out
 * 		to keep things cleaner.
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
#include <dirent.h>	//For temp sensor reading
#include <string.h>	//For temp sensor reading
#include <fcntl.h>	//For temp sensor reading
#include <stdlib.h>	//For temp sensor reading
#include <unistd.h>	//For temp sensor reading

////////////////Definitions/////////////////////////
//#define DEBUG

///////////////Body/////////////////////////////////
/***************************************************	
 * Function for reading the current temperature
 * 
 * Inputs: none
 * Outputs: Float temperature in Degrees F
***************************************************/	
void ReadTemp (float *current_temp_ptr)
{
	DIR *dir;
	struct dirent *dirent;
	char dev[16];      // Dev ID
	char devPath[128]; // Path to device
	char buf[256];     // Data from device
	char tmpData[6];   // Temp C * 1000 reported by device 
	char path[] = "/sys/bus/w1/devices";
	ssize_t numRead;
	int fd; 	//Result of file open
	char crc_valid[5];	//Variable to hold if crc is valid, yes or no

	dir = opendir (path);
	if (dir != NULL)
	{
		while ((dirent = readdir (dir)))
		
		// 1-wire devices are links beginning with 28-
		//Confirming the folder that starts with 28- which is the temp sensors  
		//Per man page, this is checking that the folder is a symbolic link
		if (dirent->d_type == DT_LNK && strstr(dirent->d_name, "28-") != NULL) 
		{   			
			strcpy(dev, dirent->d_name);        //Copies the path name as dev
#ifdef DEBUG
			printf("The temperature device: %s\n", dev);
#endif
		}
		
		(void) closedir (dir);
		
	}
	else
	{
	perror ("Error: Couldn't open the w1 devices directory");
	}

	// Assemble path to OneWire device
	sprintf(devPath, "%s/%s/w1_slave", path, dev);

	//Opening the device's file triggers new reading
	fd = open(devPath, O_RDONLY);
	if(fd == -1)
	{
		perror ("Error: Couldn't open the w1 device.");
	}
	while((numRead = read(fd, buf, 256)) > 0)
	{
		strncpy(tmpData, strstr(buf, "t=") + 2, 5);          //Find the "t=" in all the data the sensor prints
#ifdef DEBUG
		printf("Temperature from the sensor as a string is %s\n", tmpData);
#endif
		*current_temp_ptr = strtof(tmpData, NULL) / 1000 * 9 / 5 + 32;	//Converts string to a float, divides by 1000 to adjust for how sensor sends data, converts from C to F
		strncpy(crc_valid, strstr(buf, "crc=") + 6, 4);      //Grab the CRC Yes/No validty value
#ifdef DEBUG
		printf("Temperature device is: %s  - ", dev);
		printf("%.3f F\n", *current_temp_ptr);
		printf("Was the temperature reading's CRC valid: %s\n", crc_valid);
#endif
	}
		close(fd);

}



