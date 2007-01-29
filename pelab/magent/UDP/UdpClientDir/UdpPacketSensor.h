#ifndef UDP_PACKET_SENSOR_PELAB_H
#define UDP_PACKET_SENSOR_PELAB_H

#include "UdpLibs.h"
#include "UdpState.h"
#include "UdpSensor.h"
#include "UdpPacketInfo.h"

using namespace std;

class UdpPacketInfo;
class equalSeqNum;
class UdpSensor;

class UdpPacketSensor:public UdpSensor{

	public:

	explicit UdpPacketSensor(UdpState &udpStateVal, ofstream &logStreamVal);
	~UdpPacketSensor();
	void localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp);
	void localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp);
	list<UdpPacketInfo>& getUnAckedPacketList();

	private:

	bool handleReorderedAck(char *packetData, int Len, int overheadLen, unsigned long long timeStamp);

	list<UdpPacketInfo> sentPacketList;
	list<UdpPacketInfo> unAckedPacketList;
	UdpState & udpStateInfo;
	long lastSeenSeqNum;
	ofstream &logStream;
	long statReorderedPackets;
};


#endif
