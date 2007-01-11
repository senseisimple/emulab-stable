#include "UdpPacketInfo.h"

UdpPacketInfo::UdpPacketInfo()
{

}
UdpPacketInfo::UdpPacketInfo(unsigned short int seqVal,unsigned short int packetSizeVal, unsigned long long timeStampVal)
	:seqNum(seqVal),
	packetSize(packetSizeVal),
	timeStamp(timeStampVal)
{

}
