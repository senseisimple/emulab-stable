#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

EXECUTABLE=UdpServer
all: ${EXECUTABLE}

${EXECUTABLE}: UdpServer.cc UdpServer.h
	g++ -o $@ UdpServer.cc -lpcap

clean:
	rm -f ${EXECUTABLE}
