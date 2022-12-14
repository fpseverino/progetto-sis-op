CC = gcc

all: hub device

hub: hub.c
	$(CC) hub.c -o hub

device: device.c
	$(CC) device.c -o device

.PHONY: clean
clean:
	rm -f hub device