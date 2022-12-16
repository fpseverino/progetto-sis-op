CC = gcc

all: hub device accessory

hub: hub.c
	$(CC) hub.c -o hub

device: device.c
	$(CC) device.c -o device

accessory: accessory.c
	$(CC) accessory.c -o accessory

.PHONY: clean
clean:
	rm -f hub device