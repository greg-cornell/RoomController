#----------------------------------------#
#	Makefile for thermostat_P2	 #
#----------------------------------------#
CC=gcc
CFLAGS=-g -Wall -lpigpio -lrt -lpthread

default:
	$(CC) $(CFLAGS) -o light_switch light_switch_main.c rotary_encoder.c read_temp.c room_controls.c 

clean:
	rm -f light_switch
