/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpLossSensor.h"

using namespace std;

UdpLossSensor::UdpLossSensor(UdpPacketSensor const *packetHistoryVal, UdpRttSensor const *rttSensorVal)
	: packetLoss(0),
	totalLoss(0),
	packetHistory(packetHistoryVal),
	rttSensor(rttSensorVal)
{
}

UdpLossSensor::~UdpLossSensor()
{

}


void UdpLossSensor::localSend(PacketInfo *packet)
{

}

void UdpLossSensor::localAck(PacketInfo *packet)
{
	 // This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
	{
		ackValid = false;
		return;
	}

        unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);

	list<UdpPacketInfo>& unAckedPackets = (const_cast<UdpPacketSensor *>(packetHistory))->getUnAckedPacketList();
        list<UdpPacketInfo>::iterator vecIterator = unAckedPackets.begin();
	list<UdpPacketInfo>::iterator tempIterator;
	unsigned long long ewmaRtt = rttSensor->getRtt();
	unsigned long long ewmaDevRtt = rttSensor->getDevRtt();

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();

	while(vecIterator != unAckedPackets.end())
	{
		if(seqNum > ((*vecIterator).seqNum + 10) || ( timeStamp > (*vecIterator).timeStamp + 10*( ewmaRtt + 4*ewmaDevRtt)) )
		{
			logWrite(SENSOR,"UDP_LOSS_SENSOR: Lost packet seqNum=%d", (*vecIterator).seqNum);
			tempIterator = vecIterator;
			vecIterator++;
			unAckedPackets.erase(tempIterator);
			packetLoss++;
		}
		else
			vecIterator++;

	}
}

long UdpLossSensor::getPacketLoss() const
{
	return packetLoss;
}

long UdpLossSensor::getTotalPacketLoss() const
{
	return totalLoss;
}

void UdpLossSensor::resetLoss()
{
	totalLoss += packetLoss;
	packetLoss = 0;
}
