#----------------------------------------#
#	Makefile for thermostat_P2	 #
#----------------------------------------#
CC=gcc
CFLAGS=-g -Wall -lpigpio -lrt -lpthread

default:
	$(CC) $(CFLAGS) -o room_controller room_controller.c rotary_encoder.c read_temp.c control_temp.c switch_outlet.c

clean:
	rm -f room_controller
