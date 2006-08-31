SRCS=$(wildcard *.cc)
OBJS=$(SRCS:.cc=.o)
HDRS=$(wildcard *.h)
DEPFILE=.depends
CXXFLAGS=-I. -Wall -g
LDFLAGS=-lm -lpcap
# Turn this on for profiling
#CXXFLAGS+=-pg

all: $(DEPFILE) magent

magent: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(DEPFILE): $(SRCS) $(HDRS)
	$(CXX) -MM $(SRCS) > $(DEPFILE)

clean:
	-rm magent $(OBJS) $(DEPFILE)

-include $(DEPFILE)
