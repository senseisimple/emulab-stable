CXX=/usr/gnu/bin/g++
CC=/usr/gnu/bin/gcc 
LEDA=/n/paria/z/dga/LEDA-4.0
OBJS=testbed.o parse_top.o parse_ir.o phys.o parse_phys.o parse_ptop.o \
	ptop_to_phys.o
LIBS+=-L${LEDA} -lD3 -lW -lP -lG -lL -L/usr/X11R6/lib -lX11 -lm -L. -lg2c
LDFLAGS+= -O3 -fomit-frame-pointer -m486
CXXFLAGS = -I${LEDA}/incl
# Pick one of the two below.
CXXFLAGS += -Wall -O3 -fomit-frame-pointer -m486
# CXXFLAGS += -O0 -g
DEPLIBS=$(OBJS)

all: assign irtest testptop

assign: assign.o ${DEPLIBS}
	${CXX} assign.o -o assign ${LIBS} $(OBJS) ${LDFLAGS}

assign_p: assign.po $(DEPLIBS) $(POBJS)
	${CXX} assign.po -pg -g -o assign_p ${LIBS} $(POBJS)

assign.po: assign.cc
	${CXX} -c -pg -g -o assign.po assign.cc ${CXXFLAGS}

clean:
	/bin/rm -f *.o assign irtest

irtest: irtest.o ${DEPLIBS}
	${CXX} irtest.o -o irtest ${LIBS} testbed.o parse_ir.o ${LDFLAGS}

testptop: testptop.o ${DEPLIBS}
	${CXX} testptop.o -o testptop ${LIBS} testbed.o parse_ptop.o \
	${LDFLAGS}
