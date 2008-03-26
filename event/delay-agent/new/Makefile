.PHONY: all clean

all: delay-agent

COMMON_OBJS	+= main.o Pipe.o Parameter.o lib.o RootPipe.o linkCallback.o resetCallback.o
LINUX_OBJS	+= LinuxPipe.o
FREEBSD_OBJS	+= FreeBSDPipe.o
EVENT_OBJS	+= EventPipe.o

OBJS	+= $(COMMON_OBJS)
LIBS	+= $(COMMON_LIBS) -lcrypto -lpubsub
CXXFLAGS = -DELVIN_COMPAT -Wall -I../../lib/ -I../../../../
CFLAGS = $(CXXFLAGS)
LDFLAGS = -L../../../../pubsub

OS =? `uname -s`
ifeq ($(OS), "FreeBSD")
	LIBS += $(FREEBSD_LIBS)
	LDFLAGS += $(FREEBSD_LDFLAGS)
	CXXFLAGS += $(FREEBSD_CXXFLAGS)
	OBJS += $(FREEBSD_OBJS)
endif

ifeq ($(OS), "Linux")
	LIBS += $(LINUX_LIBS)
	LDFLAGS += $(LINUX_LDFLAGS)
	CXXFLAGS += $(LINUX_CXXFLAGS)
	OBJS += $(LINUX_OBJS)
endif

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

delay-agent: $(OBJS) $(COMMON_OBJS) $(EVENT_OBJS) libevent.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

event-delay-agent: $(COMMON_OBJS) $(EVENT_OBJS) libevent.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o *.a delay-agent event-delay-agent

libevent.a: event.o util.o
	$(AR) crv libevent.a event.o util.o
	ranlib libevent.a

event.o: ../../lib/event.c


util.o: ../../lib/util.c

