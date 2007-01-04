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
	unsigned long long tmpMaxDelay = maxDelay;

        if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to MaxDelaySensor::localAck was less than the "
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

	oneWayDelay = ( oneWayDelay )*1500 / (Len + 1);

        // Set this as the new maximum one way delay.
        if(oneWayDelay > tmpMaxDelay)
        {
                eventFlag = true;
                tmpMaxDelay = oneWayDelay;
        }

        // Send an event message to the monitor to change the value of maximum one way delay.
	maxDelay = tmpMaxDelay - udpStateInfo.minDelay;
        if(udpStateInfo.packetLoss != 0 || eventFlag == true)
        {
		// Report the maximum delay
		cout << "New Max Delay = " << maxDelay << "\n";
        }

	udpStateInfo.maxDelay = maxDelay;
}
