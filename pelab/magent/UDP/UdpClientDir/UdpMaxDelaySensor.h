#ifndef UDP_MAX_DELAY_SENSOR_PELAB_H
#define UDP_MAX_DELAY_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"

class UdpSensor;

class UdpMaxDelaySensor:public UdpSensor{
	public:

		explicit UdpMaxDelaySensor(UdpState &udpStateVal, ofstream &logStreamVal);
		~UdpMaxDelaySensor();


		void localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
		void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);

	private:
		unsigned long long maxDelay;
		UdpState &udpStateInfo;
		ofstream &logStream;
};

#endif
