#ifndef UDP_AVG_THROUGHPUT_SENSOR_PELAB_H
#define UDP_AVG_THROUGHPUT_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"
#include "UdpLossSensor.h"

class UdpSensor;
class UdpLossSensor;

class UdpAvgThroughputSensor:public UdpSensor{
	public:

		explicit UdpAvgThroughputSensor(UdpLossSensor *lossSensorVal, UdpState &udpStateVal, ofstream &logStreamVal);
		~UdpAvgThroughputSensor();
		void localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp);
		void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);

	private:
		void calculateTput(unsigned long long timeStamp);

		unsigned long long lastAckTime;
		UdpLossSensor *lossSensor;
		double throughputKbps;
		UdpState &udpStateInfo;
		ofstream &logStream;

		UdpAck ackList[100];
		int minSamples;
		int queuePtr;
};

#endif
