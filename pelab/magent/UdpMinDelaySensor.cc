/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpMinDelaySensor.h"
#include "CommandOutput.h"

using namespace std;

UdpMinDelaySensor::UdpMinDelaySensor(UdpPacketSensor const *udpPacketSensorVal)
	: minDelay(ULONG_LONG_MAX),
	packetHistory(udpPacketSensorVal)
{

}

UdpMinDelaySensor::~UdpMinDelaySensor()
{

}

unsigned long long UdpMinDelaySensor::getMinDelay() const
{
	return minDelay;
}	

void UdpMinDelaySensor::localSend(PacketInfo *packet)
{
	// Do nothing.
	sendValid = true;
	ackValid = false;

}


void UdpMinDelaySensor::localAck(PacketInfo *packet)
{
	 // This is a re-ordered ACK or an incorrect packet - don't do anything
	// with it - just return.
	// If this packet is ACKing a packet that we lost due to libpcap send loss,
	// dont use this packet timestamp for RTT calculations.
	if( packetHistory->isAckValid() == false || packetHistory->isAckFake() == true )
	{
		ackValid = false;
		sendValid = false;
		return;
	}
	ackValid = true;
	sendValid = false;

	unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);

	unsigned long long oneWayDelay;
	bool eventFlag = false;

	vector<UdpPacketInfo >::iterator vecIterator;
	vector<UdpPacketInfo > ackedPackets = packetHistory->getAckedPackets();
	vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();

	oneWayDelay = (timeStamp - (*vecIterator).timeStamp)/2;

	// Calculate the delay for the maximum sized packet.

	oneWayDelay = ( oneWayDelay ) * 1518 / ( (*vecIterator).packetSize);

	// Set this as the new minimum one way delay.
	if(oneWayDelay < minDelay)
	{
		eventFlag = true;
		minDelay = oneWayDelay;
	}

	// We should not be calculating the minimum delay based on the
	// redundant ACKs - because we cannot exactly calculate their
	// RTT values, from just the receiver timestamps.

	// Send an event message to the monitor to change the value of minimum one way delay.
	if(eventFlag == true)
	{
		ostringstream messageBuffer;
		messageBuffer << "delay="<<(minDelay)/1000;
		global::output->eventMessage(messageBuffer.str(), packet->elab, CommandOutput::FORWARD_PATH);
		global::output->eventMessage(messageBuffer.str(), packet->elab, CommandOutput::BACKWARD_PATH);
		logWrite(SENSOR,"VALUE::New Min delay = %llu",minDelay);
	}
	logWrite(SENSOR,"MIND:TIME=%llu,MIND=%llu",timeStamp,minDelay);
}
