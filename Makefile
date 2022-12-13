CC = gcc

hub: hub.c
	$(CC) hub.c -o hub

clean:
	rm -f hub