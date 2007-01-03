#include "UdpPacketInfo.h"

UdpPacketInfo::UdpPacketInfo(unsigned int seqVal, unsigned int packetSizeVal, unsigned long long timeStampVal, bool isFastPacketVal)
	:seqNum(seqVal),
	packetSize(packetSizeVal),
	timeStamp(timeStampVal),
	isFastPacket(isFastPacketVal),
	lastTimeDiff(0)
{

}
