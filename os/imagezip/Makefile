CC = gcc
CFLAGS = -O2 -g
LIBS = -lz

imagezip: imagezip.c
	$(CC) $(CFLAGS) imagezip.c $(LIBS) -o imagezip

install: imagezip
	install -c imagezip /usr/testbed/bin
