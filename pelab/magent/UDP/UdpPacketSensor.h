#ifndef UDP_PACKET_SENSOR_PELAB_H
#define UDP_PACKET_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"
#include "UdpPacketInfo.h"

using namespace std;

class UdpPacketInfo;
//class equalSeqNum;
class UdpSensor;

class UdpPacketSensor:public UdpSensor{

	public:

	explicit UdpPacketSensor(UdpState &udpStateVal);
	void localSend(char *packetData, int Len,unsigned long long timeStamp);
	void localAck(char *packetData, int Len,unsigned long long timeStamp);

	private:

	list<UdpPacketInfo *> sentPacketList;
	UdpState & udpStateInfo;
	unsigned long long lastPacketTime;
};


#endif
