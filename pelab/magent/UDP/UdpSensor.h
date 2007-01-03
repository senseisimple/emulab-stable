#ifndef _UDP_SENSOR_PELAB_H
#define _UDP_SENSOR_PELAB_H

#include "UdpLibs.h"

class UdpSensor{
	public:
		UdpSensor();
		virtual void localSend(char *packetData, int Len,unsigned long long timeStamp)=0;
		virtual void localAck(char *packetData, int Len,unsigned long long timeStamp)=0;
		void capturePacket(char *packetData, int Len,unsigned long long timeStamp);
};

#endif
