#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2006 University of Utah and the Flux Group.
# All rights reserved.
#

CC=g++
LD=g++
CFLAGS= -Wall -c -g -I../DataqSDK_Linux/DataqSDK 
LDFLAGS=  #-L../DataqSDK_Linux/DataqSDK -ldataqsdk -lpthread
SOURCES= main.cpp powerMeasure.cpp exceptions.cpp
OBJS = main.o powerMeasure.o exceptions.o
EXECUTABLE=main



all: $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJS)
	(cd ../DataqSDK_Linux/DataqSDK/ && $(MAKE) static)
	$(LD) $(LDFLAGS) $(OBJS) -o $(EXECUTABLE) -L../DataqSDK_Linux/DataqSDK -ldataqsdk -lpthread

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

#debug: $(SOURCES)
#	$(CC) $(LDFLAGS) $(SOURCES) -g -o $(EXECUTABLE) -L../DataqSDK_Linux/DataqSDK -ldataqsdk
	
clean:
	rm -f *.o $(EXECUTABLE) *stackdump
	(cd ../DataqSDK_Linux/DataqSDK/ && $(MAKE) clean)
