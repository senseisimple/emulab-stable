#include "UdpThroughputSensor.h"

UdpThroughputSensor::UdpThroughputSensor(UdpState &udpStateVal, ofstream &outStreamVal)
	: lastAckTime(-1),
	throughputKbps(0.0),
	udpStateInfo(udpStateVal),
	outStream(outStreamVal)
{
	udpStateInfo.minAckTimeDiff = 999999999;

}

void UdpThroughputSensor::localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpThroughputSensor::localAck(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);

	if(Len < minSize )
        {
                cout << "Error: UDP packet data sent to ThroughputSensor::localAck was less than the "
                        " required minimum "<< minSize  << " bytes\n";
                return;
        }

	int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);
	int seqNumSize = sizeof(unsigned int);
	int numRedunAcks = static_cast<int>(packetData[0]);

	unsigned long long currentAckTimeStamp = *(unsigned long long *)(packetData + 1 + sizeof(unsigned int)); 

	if(lastAckTime == -1)
	{
		lastAckTime = currentAckTimeStamp;
		return;
	}

	unsigned int seqNum;
	seqNum = *(unsigned int *)(packetData + 1);

	unsigned long ackTimeDiff = currentAckTimeStamp - lastAckTime;
	unsigned long timeDiff = 0;
	vector<UdpPacketInfo * >::iterator vecIterator;
	long dataSize = 0;
	unsigned long long oneWayDelay = 0;
	unsigned long long totalDelay = 0;

	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	//oneWayDelay = (timeStamp - (*vecIterator)->timeStamp)/2;
	oneWayDelay = (timeStamp - udpStateInfo.lastSentTime)/2;
	totalDelay = oneWayDelay;

	// Average the throughput over all the packets being acknowledged.
	if(numRedunAcks > 0)
	{
		int i;
		unsigned int redunSeqNum;

		for(i = 0;i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned int *)(packetData + minSize + i*redunAckSize);

			// Find if this redundant ACK is useful - or it was acked before.
			vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), redunSeqNum));

			if(vecIterator != udpStateInfo.recentSentPackets.end())
			{
				// Calculate throughput for the packet being acked by
				// the redundant ACK.
				dataSize += (*vecIterator)->packetSize;

			}

		}
	}

	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));
	dataSize += ( (*vecIterator)->packetSize + overheadLen );

	throughputKbps = 8000000.0*( static_cast<double> (dataSize ))  / ( static_cast<double>(totalDelay)*1024.0 );


	if(udpStateInfo.packetLoss == 0)
	{
		// Send this available bandwidth as a tentative value.
		// To be used for dummynet events only if it is greater
		// than the last seen value.
		cout << "Tentative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<", ackTimeDiff = "<<ackTimeDiff<<"\n";
		outStream << "TIME="<<timeStamp<<",TENTATIVE="<<throughputKbps<<endl;

	}
	else
	{
		// Send this as the authoritative available bandwidth value.
		cout << "Authoritative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<", ackTimeDiff ="<<ackTimeDiff<<"\n";
		outStream << "TIME="<<timeStamp<<",AUTHORITATIVE="<<throughputKbps<<endl;
	}

	lastAckTime = currentAckTimeStamp;
}
