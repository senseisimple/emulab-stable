#include "UdpMaxDelaySensor.h"

UdpMaxDelaySensor::UdpMaxDelaySensor(UdpState &udpStateVal)
	: maxDelay(0),
	udpStateInfo(udpStateVal)
{

}


void UdpMaxDelaySensor::localSend(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{

}

void UdpMaxDelaySensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);

        if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to MaxDelaySensor::localAck was less than the "
                        " required minimum "<< minSize << " bytes\n";
		udpStateInfo.ackError = true;
                return;
        }

	// Something went wrong with this packet - either the packet was not
	// the minimum size or it was a re-ordered ACK - don't do anything
	// with it - just return.
	if( udpStateInfo.ackError == true )
		return;

        int numRedunAcks = static_cast<int>(packetData[0]);
        vector<UdpPacketInfo * >::iterator vecIterator;
        unsigned int seqNum = *(unsigned int *)(packetData + 1);
        unsigned long long oneWayQueueDelay;
        bool eventFlag = false;

        vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Find the one way RTT for this packet.
        oneWayQueueDelay = (timeStamp - (*vecIterator)->timeStamp)/2;

	// Scale the value of one way RTT, so that it is correct for a transmission
	// size of 1500 bytes.
	oneWayQueueDelay = ( oneWayQueueDelay )*1500 / (overheadLen + Len + 1);

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
		cout << "New Max Delay = " << maxDelay << "\n";
        }

	udpStateInfo.maxDelay = maxDelay;
}
