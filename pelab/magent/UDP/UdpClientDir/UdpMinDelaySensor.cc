#include "UdpMinDelaySensor.h"

UdpMinDelaySensor::UdpMinDelaySensor(UdpState &udpStateVal, ofstream &logStreamVal)
	: minDelay(ULONG_LONG_MAX),
	udpStateInfo(udpStateVal),
	logStream(logStreamVal)
{

}

UdpMinDelaySensor::~UdpMinDelaySensor()
{

}


void UdpMinDelaySensor::localSend(char *packetData, int Len,int overheadLen,unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpMinDelaySensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
        if(Len < globalConsts::minAckPacketSize)
        {
                logStream << "ERROR::UDP packet data sent to MinDelaySensor::localAck was less than the "
                        " required minimum "<< globalConsts::minAckPacketSize << " bytes\n";
                return;
        }

	 // This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( udpStateInfo.ackError == true )
		return;

	unsigned short int seqNum = *(unsigned short int *)(packetData + 1);
	unsigned short int echoedPacketSize = *(unsigned short int *)(packetData + 1 + globalConsts::USHORT_INT_SIZE);
	unsigned long long echoedTimestamp = *(unsigned long long *)(packetData + 1 + 2*globalConsts::USHORT_INT_SIZE + globalConsts::ULONG_LONG_SIZE);

	unsigned long long oneWayDelay;
	bool eventFlag = false;

	vector<UdpPacketInfo >::iterator vecIterator;
	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Calculate the one way delay as half of RTT.

	// We lost this packet send time due to loss in libpcap, use the 
	// time echoed in the ACK packet.
	if(udpStateInfo.isAckFake == true)
		oneWayDelay = (timeStamp - echoedTimestamp)/2;
	else
		oneWayDelay = (timeStamp - (*vecIterator).timeStamp)/2;

	// Calculate the delay for the maximum sized packet.

	// We lost this packet size details due to loss in libpcap, use the 
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	if(udpStateInfo.isAckFake == true)
		oneWayDelay = ( oneWayDelay ) * 1518 / (echoedPacketSize);
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
		logStream << "VALUE::New Min delay = " << minDelay << "\n";
	}
	logStream << "MIND:TIME="<<timeStamp<<",MIND="<<minDelay<<endl;
	udpStateInfo.minDelay = minDelay;
}
