
CC?=gcc
CFLAGS+= -Wall
CPPFLAGS+= -I.
LDFLAGS=
LIBS=

.c.o:
	$(CC) -c $(CFLAGS) $<

all: hmcd hmcdgrab

hmcd: hmcd.o hmcd.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ hmcd.o $(LIBS)

hmcdgrab: hmcdgrab.o hmcd.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ hmcd.o $(LIBS)

clean:
	rm -f *.o hmcd hmcdgrab *~
