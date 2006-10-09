CC = gcc
CFLAGS += -g -lssl

all: sfmiddleman sfsender sfreceiver  cryptic

sfmiddleman: sfmiddleman.c util.c crypto.c
	$(CC) -o sfmiddleman $(CFLAGS) sfmiddleman.c util.c crypto.c

sfsender: sfsender.c util.c crypto.c
	$(CC) -o sfsender $(CFLAGS) sfsender.c util.c crypto.c

sfreceiver: sfreceiver.c util.c
	$(CC) -o sfreceiver $(CFLAGS) sfreceiver.c util.c

cryptic: cryptic.c
	$(CC) -o cryptic $(CFLAGS) cryptic.c

clean:
	rm -f sfmiddleman sfsender sfreceiver
