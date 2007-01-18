#include "UdpMinDelaySensor.h"

UdpMinDelaySensor::UdpMinDelaySensor(UdpPacketSensor *udpPacketSensorVal)
	: minDelay(ULONG_LONG_MAX),
	packetHistory(udpPacketSensorVal)
{

}

UdpMinDelaySensor::~UdpMinDelaySensor()
{

}

unsigned long long UdpMinDelaySensor::getMinDelay()
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
        if( ( ntohs(packet->udp->len) - 8 ) < global::udpMinAckPacketSize )
        {
                logWrite(ERROR, "UDP packet data sent to UdpMinDelaySensor::localAck was less than the "
                        " required minimum %d bytes",global::udpMinAckPacketSize);
		ackValid = false;
		sendValid = false;
                return;
        }

	 // This is a re-ordered ACK or an incorrect packet - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
	{
		ackValid = false;
		sendValid = false;
		return;
	}
	ackValid = true;
	sendValid = false;

	int overheadLen = 14 + 4 + 8 + packet->ip->ip_hl*4;
	unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);
	unsigned short int echoedPacketSize = *(unsigned short int *)(packet->payload + 1 + global::USHORT_INT_SIZE);
	unsigned long long echoedTimestamp = *(unsigned long long *)(packet->payload + 1 + 2*global::USHORT_INT_SIZE + global::ULONG_LONG_SIZE);

	unsigned long long oneWayDelay;
	bool eventFlag = false;

	vector<UdpPacketInfo >::iterator vecIterator;
	vecIterator = find_if(packetHistory->ackedPackets.begin(), packetHistory->ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Calculate the one way delay as half of RTT.

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();
	// We lost this packet send time due to loss in libpcap, use the 
	// time echoed in the ACK packet.
	if(packetHistory->isAckFake() == true)
		oneWayDelay = (timeStamp - echoedTimestamp)/2;
	else
		oneWayDelay = (timeStamp - (*vecIterator).timeStamp)/2;

	// Calculate the delay for the maximum sized packet.

	// We lost this packet size details due to loss in libpcap, use the 
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	if(packetHistory->isAckFake() == true)
		oneWayDelay = ( oneWayDelay ) * 1518 / (overheadLen + echoedPacketSize);
	else
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
		logWrite(SENSOR,"VALUE::New Min delay = %llu",minDelay);
	}
	logWrite(SENSOR,"MIND:TIME=%llu,MIND=%llu",timeStamp,minDelay);
}
