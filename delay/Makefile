OBJS=delay.o
CFLAGS += -Wall -O2 #-DDEBUG=1 -g
CC=gcc
LIBS=-L. -ldivert

all: delay

delay: ${OBJS}
	$(CC) $(OBJS) ${LIBS} -o delay

libdivert.a: divert.o divert_hash.o
	${AR} cr libdivert.a divert.o divert_hash.o
	${RANLIB} libdivert.a

install: delay
	cp -f delay /tmp/delay

clean:
	/bin/rm -rf $(OBJS) delay
