#ifndef READ_TEMP_H

void ReadTemp(float *);
/*
 * This function is for reading the temperature sensor.
 *
 * It passes a float pointer for the current temperature
 * used by the main program.  ReadTemp reads the sensor
 * and and updates the value in the pointer with the 
 * read temperature.
*/

#define READ_TEMP_H
#endif
