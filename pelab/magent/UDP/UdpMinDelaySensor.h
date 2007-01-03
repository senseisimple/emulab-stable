#ifndef UDP_MIN_DELAY_SENSOR_PELAB_H
#define UDP_MIN_DELAY_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"

class UdpSensor;

class UdpMinDelaySensor:public UdpSensor{
	public:

		explicit UdpMinDelaySensor(UdpState &udpStateVal);
		void localSend(char *packetData, int Len, unsigned long long timeStamp);
		void localAck(char *packetData, int Len, unsigned long long timeStamp);

	private:
		unsigned long long minDelay;
		UdpState &udpStateInfo;
};

#endif
