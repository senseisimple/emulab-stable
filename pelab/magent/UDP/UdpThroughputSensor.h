#ifndef UDP_THROUGHPUT_SENSOR_PELAB_H
#define UDP_THROUGHPUT_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"

class UdpSensor;

class UdpThroughputSensor:public UdpSensor{
	public:

		explicit UdpThroughputSensor(UdpState &udpStateVal, ofstream &outStreamVal);
		void localSend(char *packetData, int Len,unsigned long long timeStamp);
		void localAck(char *packetData, int Len,unsigned long long timeStamp);

	private:
		long long lastAckTime;
		double throughputKbps;
		UdpState &udpStateInfo;
		ofstream &outStream;
};

#endif
