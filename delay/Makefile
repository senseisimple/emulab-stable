OBJS=delay.o
POBJS=delay.po
CFLAGS += -Wall -O2 -DSPIN_OK #-DDEBUG=1 -g
CC=gcc
LIBS=-L. -ldivert
PLIBS=-L. -ldivert_p

all: delay

delay.po: delay.c
	${CC} delay.c -pg -c -o $@

delay: ${OBJS}
	$(CC) $(OBJS) ${LIBS} -o delay

delay_p: ${POBJS}
	$(CC) -pg $(POBJS) ${PLIBS} -o $@

libdivert.a: divert.o divert_hash.o
	${AR} cr libdivert.a divert.o divert_hash.o
	${RANLIB} libdivert.a

install: delay
	cp -f delay /tmp/delay

clean:
	/bin/rm -rf $(OBJS) delay
