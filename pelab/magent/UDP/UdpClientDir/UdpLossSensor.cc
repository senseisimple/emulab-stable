/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpLossSensor.h"

UdpLossSensor::UdpLossSensor(UdpPacketSensor *packetHistoryVal, UdpRttSensor *rttSensorVal, UdpState &udpStateVal, ofstream &logStreamVal)
	: packetLoss(0),
	udpStateInfo(udpStateVal),
	logStream(logStreamVal)
{
	packetHistory = packetHistoryVal;
	rttSensor = rttSensorVal;
	totalLoss = 0;
}

UdpLossSensor::~UdpLossSensor()
{

}


void UdpLossSensor::localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{

}

void UdpLossSensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
        if(Len < globalConsts::minAckPacketSize)
        {
                logStream << "ERROR::UDP packet data sent to RttSensor::localAck was less than the "
                        " required minimum "<< globalConsts::minAckPacketSize << " bytes\n";
                return;
        }

	 // This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( udpStateInfo.ackError == true )
		return;

        unsigned short int seqNum = *(unsigned short int *)(packetData + 1);

	list<UdpPacketInfo>& unAckedPackets = packetHistory->getUnAckedPacketList();
        list<UdpPacketInfo>::iterator vecIterator = unAckedPackets.begin();
	list<UdpPacketInfo>::iterator tempIterator;
	unsigned long long ewmaRtt = rttSensor->getRtt();
	unsigned long long ewmaDevRtt = rttSensor->getDevRtt();

	while(vecIterator != unAckedPackets.end())
	{
		if(seqNum > (*vecIterator).seqNum + 10 || ( timeStamp > (*vecIterator).timeStamp + 10*( ewmaRtt + 4*ewmaDevRtt)) )
		{
			tempIterator = vecIterator;
			vecIterator++;
			unAckedPackets.erase(tempIterator);
			packetLoss++;
		}
		else
			vecIterator++;

	}
}

long UdpLossSensor::getPacketLoss()
{
	return packetLoss;
}

void UdpLossSensor::resetLoss()
{
	totalLoss += packetLoss;
	packetLoss = 0;
}
