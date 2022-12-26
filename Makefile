CC = gcc
CFLAGS = -I.

all: hub device accessory

hub: hub.c libraries.c libraries.h
	$(CC) hub.c libraries.c -o hub

device: device.c libraries.c libraries.h
	$(CC) device.c libraries.c -o device

accessory: accessory.c libraries.c libraries.h
	$(CC) accessory.c libraries.c -o accessory

.PHONY: clean all
clean:
	rm -f hub device accessory