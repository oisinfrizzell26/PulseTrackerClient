CC = gcc
CFLAGS = -Wall -I/opt/homebrew/include
MQTT_LIBS = -L/opt/homebrew/lib -lmosquitto

all: main

main: mqtt_client.c
	$(CC) $(CFLAGS) -o main mqtt_client.c $(MQTT_LIBS)

clean:
	rm -f main
