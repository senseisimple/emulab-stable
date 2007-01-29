#include "UdpMaxDelaySensor.h"

UdpMaxDelaySensor::UdpMaxDelaySensor(UdpState &udpStateVal, ofstream &logStreamVal)
	: maxDelay(0),
	udpStateInfo(udpStateVal),
	logStream(logStreamVal)
{

}

UdpMaxDelaySensor::~UdpMaxDelaySensor()
{

}


void UdpMaxDelaySensor::localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{

}

void UdpMaxDelaySensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
        if(Len < globalConsts::minAckPacketSize)
        {
                logStream << "ERROR::UDP packet data sent to MaxDelaySensor::localAck was less than the "
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
        unsigned long long oneWayQueueDelay;
        bool eventFlag = false;

        vector<UdpPacketInfo>::iterator vecIterator;
        vecIterator = find_if(udpStateInfo.currentAckedPackets.begin(), udpStateInfo.currentAckedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Find the one way RTT for this packet.

	// We lost this packet send time due to loss in libpcap, use the
	// time echoed in the ACK packet.
	if(udpStateInfo.isAckFake == true)
		oneWayQueueDelay = (timeStamp - echoedTimestamp)/2;
	else
		oneWayQueueDelay = (timeStamp - (*vecIterator).timeStamp)/2;

	// Scale the value of one way RTT, so that it is correct for a transmission
	// size of 1518 bytes.

	// We lost this packet size details due to loss in libpcap, use the
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	if(udpStateInfo.isAckFake == true)
		oneWayQueueDelay = ( oneWayQueueDelay )*1518 / (echoedPacketSize);
	else
		oneWayQueueDelay = ( oneWayQueueDelay )*1518 / ((*vecIterator).packetSize);

	// Find the queuing delay for this packet, by subtracting the
	// one way minimum delay from the above value.
	oneWayQueueDelay = oneWayQueueDelay - udpStateInfo.minDelay;

        // Set this as the new maximum one way queuing delay.
        if(oneWayQueueDelay > maxDelay)
        {
                eventFlag = true;
                maxDelay = oneWayQueueDelay;
        }

        // Send an event message to the monitor to change the value of maximum one way delay.
        if(eventFlag == true)
        {
		// Report the maximum delay
		logStream << "VALUE::New Max Delay = " << maxDelay << "\n";
        }
	logStream << "MAXD:TIME="<<timeStamp<<",MAXD="<<maxDelay<<endl;
	logStream << "ACTUAL_MAXD:TIME="<<timeStamp<<",ACTUAL_MAXD="<<oneWayQueueDelay<<endl;

	udpStateInfo.maxDelay = maxDelay;
}
