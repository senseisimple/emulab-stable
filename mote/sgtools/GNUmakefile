#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#

#
# These are the paths to the ARM cross-compiler on ibapah.flux.utah.edu -
# should put something more general here
#
CXX=/usr/local/arm/3.4.1/bin/arm-linux-g++
CXXFLAGS=-static
STRIP=/usr/local/arm/3.4.1/bin/arm-linux-strip
CC=/usr/local/arm/3.4.1/bin/arm-linux-gcc

OBJS=SGGPIO.o

all: motepower moteleds

motepower: motepower.C $(OBJS)
	$(CXX) $(CXXFLAGS) -o motepower motepower.C $(OBJS)
	$(STRIP) motepower

moteleds: moteleds.C $(OBJS)
	$(CXX) $(CXXFLAGS) -o moteleds moteleds.C $(OBJS)
	$(STRIP) moteleds



clean:
	rm -f $(OBJS) motepower moteleds
