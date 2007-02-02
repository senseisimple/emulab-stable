#ifndef UDP_AVG_THROUGHPUT_SENSOR_PELAB_H
#define UDP_AVG_THROUGHPUT_SENSOR_PELAB_H

#include "lib.h"
#include "Sensor.h"
#include "UdpPacketSensor.h"
#include "UdpLossSensor.h"

class Sensor;
class UdpPacketSensor;
class UdpLossSensor;

struct UdpAck {
	unsigned long long timeTaken;
	long packetSize;
	unsigned short seqNum;
	bool isRedun;
};


class UdpAvgThroughputSensor:public Sensor{
	public:

		explicit UdpAvgThroughputSensor(UdpPacketSensor const *packetHistoryVal, UdpLossSensor const *lossSensorVal);
		~UdpAvgThroughputSensor();
		void localSend(PacketInfo *packet);
		void localAck(PacketInfo *packet);

	private:
		void calculateTput(unsigned long long timeStamp);

		UdpPacketSensor const *packetHistory;
		UdpLossSensor const *lossSensor;

		const static int MIN_SAMPLES = 5;
		const static int MAX_SAMPLES = 100;
		const static unsigned long long MAX_TIME_PERIOD = 500000;

		unsigned long long lastAckTime;
		double throughputKbps;

		UdpAck ackList[100];
		int numSamples;
		int queuePtr;
};

#endif
