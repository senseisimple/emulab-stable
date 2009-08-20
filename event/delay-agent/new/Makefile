.PHONY: all clean

all: delay-agent

SRCS	:= $(wildcard *.cc)

LIBS	= -lcrypto -lpubsub
CXXFLAGS = -DELVIN_COMPAT -Wall -I../../lib/ -I../../../../ -I../../../lib/libtb/
LDFLAGS = -L../../../../pubsub

OS = $(shell uname -s)

ifeq ($(OS), FreeBSD)
CXXFLAGS += -DFREEBSD
endif

ifeq ($(OS), Linux)
CXXFLAGS += -DLINUX -I../../../../libnl/include
#LDFLAGS += -L../../../../libnl/lib -lnl
endif

OBJS	:= $(SRCS:%.cc=%.o)

CFLAGS = $(CXXFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

delay-agent: $(OBJS) libevent.a ../../../../libnl/lib/libnl.a ../../../../obj/lib/libtb/log.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

../../../../obj/lib/libtb/log.o:
	@echo "build testbed tree first to get log.o!"

clean:
	rm -f *.a delay-agent $(OBJS)

libevent.a: event.o util.o
	$(AR) crv libevent.a event.o util.o
	ranlib libevent.a

event.o: ../../lib/event.c
	$(CC) $(CFLAGS) -c $< -o $@

util.o: ../../lib/util.c
	$(CC) $(CFLAGS) -c $< -o $@

