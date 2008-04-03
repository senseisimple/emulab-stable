.PHONY: all clean

all: delay-agent

SRCS	:= $(wildcard *.cc)
OBJS	:= $(SRCS:%.cc=%.o)

LIBS	= -lcrypto -lpubsub
CXXFLAGS = -DELVIN_COMPAT -Wall -I../../lib/ -I../../../../
LDFLAGS = -L../../../../pubsub

OS = $(shell uname -s)

ifeq ($(OS), FreeBSD)
CXXFLAGS += -DFREEBSD
endif

ifeq ($(OS), Linux)
CXXFLAGS += -DLINUX
endif

CFLAGS = $(CXXFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

delay-agent: $(OBJS) $(COMMON_OBJS) $(EVENT_OBJS) libevent.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.a delay-agent

libevent.a: event.o util.o
	$(AR) crv libevent.a event.o util.o
	ranlib libevent.a

event.o: ../../lib/event.c
	$(CC) $(CFLAGS) -c $< -o $@

util.o: ../../lib/util.c
	$(CC) $(CFLAGS) -c $< -o $@

