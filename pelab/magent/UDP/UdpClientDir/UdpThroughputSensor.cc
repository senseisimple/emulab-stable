#include "UdpThroughputSensor.h"

UdpThroughputSensor::UdpThroughputSensor(UdpState &udpStateVal, std::ofstream &outStreamVal)
	: lastAckTime(0),
	throughputKbps(0.0),
	udpStateInfo(udpStateVal),
	outStream(outStreamVal)
{

}

void UdpThroughputSensor::localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpThroughputSensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);

	if(Len < minSize)
        {
                cout << "Error: UDP packet data sent to ThroughputSensor::localAck was less than the "
                        " required minimum "<< minSize << " bytes\n";
		udpStateInfo.ackError = true;
                return;
        }

	// Something went wrong with this packet - either the packet was not
	// the minimum size or it was a re-ordered ACK - don't do anything
	// with it - just return.
	if( udpStateInfo.ackError == true )
		return;


	int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);
	int seqNumSize = sizeof(unsigned int);

	// Find out how many redundant ACKs this packet is carrying - 0 to 3.
	int numRedunAcks = static_cast<int>(packetData[0]);
	packetData++;

	int numThroughputAcks = 1;
	double avgThroughput = 0;

	// This is the timestamp at the receiver, when the original packet was received.
	unsigned long long currentAckTimeStamp = *(unsigned long long *)(packetData + sizeof(unsigned int)); 

	// This is the first ACK we have seen, store its receiver timestamp
	// and return, we cannot calculate throughput from just one ACK - at least 2.
	if(lastAckTime == 0)
	{
		lastAckTime = currentAckTimeStamp;
		return;
	}

	unsigned int seqNum = *(unsigned int *)(packetData);

	unsigned long ackTimeDiff = currentAckTimeStamp - lastAckTime;
	unsigned long timeDiff = 0;
	vector<UdpPacketInfo * >::iterator vecIterator;

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
				numThroughputAcks++;

				timeDiff = *(unsigned long *)(packetData + minSize + i*redunAckSize + seqNumSize);

				avgThroughput += 8000000.0*( static_cast<double> ( (*vecIterator)->packetSize + overheadLen ))  / ( static_cast<double>(ackTimeDiff - timeDiff)*1024.0 );

					ackTimeDiff = timeDiff;
			}

		}
	}

	// Calculate the throughput for the current packet being ACKed.
	vecIterator = find_if(udpStateInfo.recentSentPackets.begin(), udpStateInfo.recentSentPackets.end(), bind2nd(equalSeqNum(), seqNum));

	avgThroughput += 8000000.0*( static_cast<double> ((*vecIterator)->packetSize + overheadLen ))  / ( static_cast<double>(ackTimeDiff)*1024.0 );

	throughputKbps = avgThroughput / (static_cast<double> (numThroughputAcks) );

	// Send a message to the monitor with the new bandwidth.
	if(udpStateInfo.packetLoss == 0)
	{
		// Send this available bandwidth as a tentative value.
		// To be used for dummynet events only if it is greater
		// than the last seen value.
		cout << "Tentative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<"acktimeDiff = "<<ackTimeDiff<<"\n";

		outStream << "TIME="<<timeStamp<<",TENTATIVE="<<throughputKbps<<endl;
	}
	else
	{
		// Send this as the authoritative available bandwidth value.
		cout << "Authoritative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<"ackTimeDiff = "<<ackTimeDiff<<"\n";
		outStream << "TIME="<<timeStamp<<",AUTHORITATIVE="<<throughputKbps<<endl;
	}

	// Save the receiver timestamp of this ACK packet, so that we can
	// use for calculating throughput for the next ACK packet.
	lastAckTime = currentAckTimeStamp;
}
