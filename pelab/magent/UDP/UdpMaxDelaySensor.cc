#include "UdpMaxDelaySensor.h"

UdpMaxDelaySensor::UdpMaxDelaySensor(UdpState &udpStateVal)
	: maxDelay(0),
	udpStateInfo(udpStateVal)
{

}


void UdpMaxDelaySensor::localSend(char *packetData, int Len, unsigned long long timeStamp)
{

}

void UdpMaxDelaySensor::localAck(char *packetData, int Len, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);
	unsigned long long tmpMaxDelay = maxDelay;

        if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to MaxDelaySensor::localAck was less than the "
                        " required minimum "<< minSize << " bytes\n";
                return;
        }

        //int numRedunAcks = (Len - minSize)/redunAckSize;
        int numRedunAcks = static_cast<int>(packetData[0]);
        vector<UdpPacketInfo * >::iterator vecIterator;
        unsigned int seqNum = *(unsigned int *)(packetData + 1);
        unsigned long long oneWayDelay;
        bool eventFlag = false;

        vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

        oneWayDelay = (timeStamp - (*vecIterator)->timeStamp)/2;
	//cout <<" IN maxdelay timestamp = "<<timeStamp<<", sent time = "<<(*vecIterator)->timeStamp<<endl;

	oneWayDelay = ( oneWayDelay )*1500 / (Len + 1);

        // Set this as the new maximum one way delay.
        if(oneWayDelay > tmpMaxDelay)
        {
                eventFlag = true;
                tmpMaxDelay = oneWayDelay;
        }

        int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);
        int seqNumSize = sizeof(unsigned int);

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
                                oneWayDelay = (timeStamp - timeDiff - (*vecIterator)->timeStamp )/2;
                                if(oneWayDelay > 0 && oneWayDelay > maxDelay)
                                {
                                        eventFlag = true;
                                        maxDelay = oneWayDelay;
                                }
                        }
                }

        }
	*/

        // Send an event message to the monitor to change the value of maximum one way delay.
	maxDelay = tmpMaxDelay - udpStateInfo.minDelay;
        if(udpStateInfo.packetLoss != 0 || eventFlag == true)
        {
		// Report the maximum delay
		cout << "New Max Delay = " << maxDelay << "\n";
        }

	udpStateInfo.maxDelay = maxDelay;
}
