/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef UDP_RTT_SENSOR_PELAB_H
#define UDP_RTT_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"

class UdpSensor;

class UdpRttSensor:public UdpSensor{
	public:

		explicit UdpRttSensor(UdpState &udpStateVal, ofstream &logStreamVal);
		~UdpRttSensor();


		void localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
		void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
		unsigned long long getRtt();
		unsigned long long getDevRtt();

	private:
		unsigned long long ewmaRtt, ewmaDevRtt;
		UdpState &udpStateInfo;
		ofstream &logStream;
};

#endif
