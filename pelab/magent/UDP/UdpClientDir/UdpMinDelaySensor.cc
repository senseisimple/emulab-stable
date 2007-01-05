#include "UdpMinDelaySensor.h"

UdpMinDelaySensor::UdpMinDelaySensor(UdpState &udpStateVal)
	: minDelay(ULONG_LONG_MAX),
	udpStateInfo(udpStateVal)
{

}


void UdpMinDelaySensor::localSend(char *packetData, int Len,int overheadLen,unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpMinDelaySensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);

        if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to MinDelaySensor::localAck was less than the "
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
	unsigned long long oneWayDelay;
	bool eventFlag = false;

	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Calculate the one way delay as half of RTT.
	oneWayDelay = (timeStamp - (*vecIterator)->timeStamp)/2;

	// Calculate the delay for the maximum sized packet.
	oneWayDelay = ( oneWayDelay ) * 1500 / (overheadLen + Len + 1);

	// Set this as the new minimum one way delay.
	if(oneWayDelay < minDelay)
	{
		eventFlag = true;
		minDelay = oneWayDelay;
	}

	int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);
	int seqNumSize = sizeof(unsigned int);

	// We should not be calculating the minimum delay based on the
	// redundant ACKs - because we cannot exactly calculate their
	// RTT values, from just the receiver timestamps.
	/*
	if(numRedunAcks > 0)
	{
		int i;
		unsigned int redunSeqNum;
		unsigned long timeDiff;

		for(i = 0;i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned int *)(packetData + minSize + i*redunAckSize);
			timeDiff = *(unsigned long *)(packetData + minSize + i*redunAckSize + seqNumSize);

			vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), redunSeqNum));

			if(vecIterator != udpStateInfo.recentSentPackets.end())
			{
				oneWayDelay = (timeStamp - timeDiff - (*vecIterator)->timeStamp ) /2;
				oneWayDelay = ( oneWayDelay ) * 1500 / ( (*vecIterator)->packetSize + overheadLen );

				if(oneWayDelay > 0 && oneWayDelay < minDelay)
				{
					eventFlag = true;
					minDelay = oneWayDelay;
					minDelayBytes = (*vecIterator)->packetSize + overheadLen;
				}
			}
		}

	}
	*/

	// Send an event message to the monitor to change the value of minimum one way delay.
	if(eventFlag == true)
	{
		cout << "New Min delay = " << minDelay << "\n";
	}
	udpStateInfo.minDelay = minDelay;
}
