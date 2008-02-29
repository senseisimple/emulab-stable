.PHONY: all clean

all: delay-agent

COMMON_OBJS	+= main.o
LINUX_OBJS	+= LinuxPipe.o
FREEBSD_OBJS	+= FreeBSDPipe.o
EVENT_OBJS	+= EventPipe.o

OBJS	+= $(COMMON_OBJS)
LIBS	+= $(COMMON_LIBS)
CXXFLAGS = -Wall

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
	$(CXX) $(CXXFLAGS) -c $< $@

delay-agent: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

event-delay-agent: $(COMMON_OBJS) $(EVENT_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f *.o delay-agent event-delay-agent
