/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _UDP_SENSOR_PELAB_H
#define _UDP_SENSOR_PELAB_H

#include "UdpLibs.h"

class UdpSensor{
	public:
		UdpSensor();
		virtual ~UdpSensor();
		void capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp, int packetDirection);


		UdpSensor *next;
	protected:
		virtual void localSend(char *packetData, int Len, int overheadLen,unsigned long long timeStamp)=0;
		virtual void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)=0;

};

#endif
