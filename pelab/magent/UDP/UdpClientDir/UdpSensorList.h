#ifndef _UDP_SENSORLIST_PELAB_H
#define _UDP_SENSORLIST_PELAB_H
#include "UdpLibs.h"
#include "UdpSensor.h"
#include "UdpPacketSensor.h"
#include "UdpThroughputSensor.h"
#include "UdpMinDelaySensor.h"
#include "UdpMaxDelaySensor.h"

class UdpSensor;
class UdpPacketSensor;
class UdpThroughputSensor;
class UdpMinDelaySensor;
class UdpMaxDelaySensor;

class UdpSensorList {
	public:
	explicit UdpSensorList(ofstream &logStreamVal);
	~UdpSensorList();

	void addSensor(int);
	void capturePacket(char *packetData, int Len, int overheadLen, unsigned long long timeStamp, int packetDirection);
	void reset();
	void testFunc();

	private:
	void pushSensor(UdpSensor *);
	void addPacketSensor();
	void addThroughputSensor();
	void addMinDelaySensor();
	void addMaxDelaySensor();

	UdpSensor *sensorListHead;
        UdpSensor *sensorListTail;

	UdpState udpStateInfo;
	ofstream &logStream;

	UdpSensor *depPacketSensor;
	UdpSensor *depThroughputSensor;
	UdpSensor *depMinDelaySensor;
	UdpSensor *depMaxDelaySensor;
};


#endif
