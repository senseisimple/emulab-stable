CC = gcc
CFLAGS = -O2 -g
LIBS = -lz

all:	imagezip imageunzip

imagezip: imagezip.c
	$(CC) $(CFLAGS) imagezip.c $(LIBS) -o imagezip

imageunzip: imageunzip.c
	$(CC) $(CFLAGS) imageunzip.c $(LIBS) -o imageunzip

install: imagezip imageunzip
	install -c imagezip /usr/testbed/bin
	install -c imageunzip /usr/testbed/bin

clean:
	/bin/rm -f *.o imagezip imageunzip
