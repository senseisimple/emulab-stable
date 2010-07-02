/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpRttSensor.h"

using namespace std;

UdpRttSensor::UdpRttSensor(UdpPacketSensor const *packetHistoryVal)
	: ewmaRtt(0),
	ewmaDevRtt(0),
	packetHistory(packetHistoryVal)
{

}

UdpRttSensor::~UdpRttSensor()
{

}


void UdpRttSensor::localSend(PacketInfo *packet)
{

}

void UdpRttSensor::localAck(PacketInfo *packet)
{
	 // This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
	{
		ackValid = false;
		return;
	}

	// We lost information about this packet's send time due to loss
	// in libpcap - don't use this packet for calculating RTT.
	if(packetHistory->isAckFake() == true)
		return;

        unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);
        unsigned long long currentRtt;
	vector<UdpPacketInfo> ackedPackets = packetHistory->getAckedPackets();

        vector<UdpPacketInfo>::iterator vecIterator;
        vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Find the RTT for this packet.

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();
	currentRtt = (timeStamp - (*vecIterator).timeStamp)/2;

	// Scale the value of one way RTT, so that it is correct for a transmission
	// size of 1518 bytes.

	// We lost this packet size details due to loss in libpcap, use the
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	currentRtt = ( currentRtt )*1518 / ((*vecIterator).packetSize);

	// Avoid conversion to double and back to long long.
	//double alpha = 0.25, beta = 0.125;

	if(ewmaRtt == 0)
	{
		ewmaRtt = currentRtt;
	}
	else
	{
		if(ewmaDevRtt == 0)
		{
			if(currentRtt > ewmaRtt)
				ewmaDevRtt = currentRtt - ewmaRtt ;
			else
				ewmaDevRtt = ewmaRtt - currentRtt ;
		}
		else
		{
			if(currentRtt > ewmaRtt)
			{
				// Avoid conversion to double and back to long long.
				//ewmaDevRtt = ewmaDevRtt*(1 - beta) + beta*(currentRtt - ewmaRtt);
				ewmaDevRtt = ewmaDevRtt*7/8 + (currentRtt - ewmaRtt)/8;
			}
			else
			{
				// Avoid conversion to double and back to long long.
				//ewmaDevRtt = ewmaDevRtt*(1 - beta) + beta*(ewmaRtt - currentRtt );
				ewmaDevRtt = ewmaDevRtt*7/8 + (ewmaRtt - currentRtt )/8;
			}
		}

			// Avoid conversion to double and back to long long.
		//ewmaRtt = ewmaRtt*(1 - alpha) + currentRtt*alpha;
		ewmaRtt = ewmaRtt*3/4 + currentRtt/4;
	}
}

unsigned long long UdpRttSensor::getRtt() const
{
	return ewmaRtt;
}

unsigned long long UdpRttSensor::getDevRtt() const
{
	return ewmaDevRtt;
}
