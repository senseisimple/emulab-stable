#
# Insert Copyright Here.
#
include ../Makeconf
include ../GNUmakerules

CXX=/usr/gnu/bin/g++
CC=/usr/gnu/bin/gcc 
LEDA=/n/paria/z/dga/LEDA-4.0
OBJS=score.o parse_top.o parse_ptop.o
LIBS+=-L${LEDA} -lD3 -lW -lP -lG -lL -L/usr/X11R6/lib -lX11 -lm -L.
LDFLAGS+= -O3 -fomit-frame-pointer -m486
CXXFLAGS = -I${LEDA}/incl
# Pick one of the two below.
CXXFLAGS += -Wall -O3 -fomit-frame-pointer -m486
#CXXFLAGS += -O0 -g -Wall -DSCORE_DEBUG -DVERBOSE -DSCORE_DEBUG_MORE
DEPLIBS=$(OBJS)

all: assign 

assign: ${DEPLIBS} ${OBJS} assign.o
	${CXX} assign.o -o assign ${LIBS} $(OBJS) ${LDFLAGS}

assign_p: assign.po $(DEPLIBS) $(POBJS) 
	${CXX} assign.po -pg -g -o assign_p ${LIBS} $(POBJS)

assign.po: assign.cc
	${CXX} -c -pg -g -o assign.po assign.cc ${CXXFLAGS}

install: $(INSTALL_BINDIR)/assign $(INSTALL_LIBDIR)/assign 

clean:
	/bin/rm -f *.o assign 

