/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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

UdpPacketInfo::~UdpPacketInfo()
{

}
