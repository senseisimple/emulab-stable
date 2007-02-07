/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDPLIBS_PELAB_H
#define UDPLIBS_PELAB_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <cstdlib>
#include <algorithm>
#include <functional>
#include <climits>
#include <limits.h>

namespace globalConsts {

	const short int USHORT_INT_SIZE = sizeof(unsigned short int);
	const short int ULONG_LONG_SIZE = sizeof(unsigned long long);
	const short int UCHAR_SIZE = sizeof(unsigned char);

	const static int redunAckSize = 2*USHORT_INT_SIZE + ULONG_LONG_SIZE;
	const static int seqNumSize = USHORT_INT_SIZE;
	const static int minAckPacketSize = 2*USHORT_INT_SIZE + 2*ULONG_LONG_SIZE;
}

enum {UDP_PACKET_SENSOR, UDP_THROUGHPUT_SENSOR, UDP_MINDELAY_SENSOR, UDP_MAXDELAY_SENSOR, UDP_RTT_SENSOR, UDP_LOSS_SENSOR, UDP_AVG_THROUGHPUT_SENSOR};

struct UdpAck {

	unsigned long long timeTaken;
	long packetSize;

};

#endif
