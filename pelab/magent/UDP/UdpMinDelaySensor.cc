#include "UdpMinDelaySensor.h"

UdpMinDelaySensor::UdpMinDelaySensor(UdpState &udpStateVal)
	: minDelay(ULONG_LONG_MAX),
	udpStateInfo(udpStateVal)
{

}


void UdpMinDelaySensor::localSend(char *packetData, int Len,unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpMinDelaySensor::localAck(char *packetData, int Len, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);

        if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to MinDelaySensor::localAck was less than the "
                        " required minimum "<< minSize << " bytes\n";
                return;
        }

	int numRedunAcks = static_cast<int>(packetData[0]);
	vector<UdpPacketInfo * >::iterator vecIterator;
	unsigned int seqNum = *(unsigned int *)(packetData + 1);
	unsigned long long oneWayDelay;
	bool eventFlag = false;

	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	oneWayDelay = (timeStamp - (*vecIterator)->timeStamp)/2;
	long minDelayBytes = 0;

	// Calculate the delay for the maximum possibly sized packet.
	oneWayDelay = ( oneWayDelay ) * 1500 / (Len + 1);

	// Set this as the new minimum one way delay.
	if(oneWayDelay < minDelay)
	{
		eventFlag = true;
		minDelay = oneWayDelay;

		// One byte was chopped off in the base sensor class.
		// Add that to the packet length.
		minDelayBytes = Len + 1;
	}

	int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);
	int seqNumSize = sizeof(unsigned int);

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
				oneWayDelay = ( oneWayDelay ) * 1500 / ( (*vecIterator)->packetSize );

				if(oneWayDelay > 0 && oneWayDelay < minDelay)
				{
					eventFlag = true;
					minDelay = oneWayDelay;
					minDelayBytes = (*vecIterator)->packetSize;
				}
			}
		}

	}

	// Send an event message to the monitor to change the value of minimum one way delay.
	if(eventFlag == true)
	{
		cout << "New Min delay = " << minDelay << "\n";
	}
	udpStateInfo.minDelay = minDelay;
	udpStateInfo.minDelayBytes = minDelayBytes;
}
