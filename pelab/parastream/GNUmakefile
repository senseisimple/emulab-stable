CC = gcc
CFLAGS += -g

all: server client

client: parastream.c
	$(CC) -o parastream $(CFLAGS) parastream.c

server: parastreamd.c
	$(CC) -o parastreamd $(CFLAGS) parastreamd.c

clean:
	rm -f parastream parastreamd
