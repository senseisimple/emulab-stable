/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDP_MIN_DELAY_SENSOR_PELAB_H
#define UDP_MIN_DELAY_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"

class UdpSensor;

class UdpMinDelaySensor:public UdpSensor{
	public:

		explicit UdpMinDelaySensor(UdpState &udpStateVal, ofstream &logStreamVal);
		~UdpMinDelaySensor();


		void localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
		void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);

	private:
		unsigned long long minDelay;
		UdpState &udpStateInfo;
		ofstream &logStream;
};

#endif
