#include "UdpAvgThroughputSensor.h"

UdpAvgThroughputSensor::UdpAvgThroughputSensor(UdpLossSensor *lossSensorVal,UdpState &udpStateVal, std::ofstream &logStreamVal)
	: lastAckTime(0),
	throughputKbps(0.0),
	udpStateInfo(udpStateVal),
	logStream(logStreamVal)
{
	lossSensor = lossSensorVal;
	minSamples = 0;
	queuePtr = 0;
}

UdpAvgThroughputSensor::~UdpAvgThroughputSensor()
{

}

void UdpAvgThroughputSensor::localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	// Do nothing.

}

void UdpAvgThroughputSensor::localAck(char *packetData, int Len,int overheadLen, unsigned long long timeStamp)
{
	if(Len < globalConsts::minAckPacketSize )
        {
                logStream << "ERROR::UDP packet data sent to ThroughputSensor::localAck was less than the "
                        " required minimum "<< globalConsts::minAckPacketSize<< " bytes\n";
                return;
        }

	// This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( udpStateInfo.ackError == true )
		return;

	// Find out how many redundant ACKs this packet is carrying - 0 to 121.
	unsigned char numRedunAcksChar = 0;
	memcpy(&numRedunAcksChar, &packetData[0], globalConsts::UCHAR_SIZE);
	int numRedunAcks = static_cast<int>(numRedunAcksChar);

	int numThroughputAcks = 1;
	double avgThroughput = 0;

	// This is the timestamp at the receiver, when the original packet was received.
	unsigned long long currentAckTimeStamp = *(unsigned long long *)(packetData + 1 + 2*globalConsts::USHORT_INT_SIZE ); 

	// This is the first ACK we have seen, store its receiver timestamp
	// and return, we cannot calculate throughput from just one ACK - at least 2.
	if(lastAckTime == 0)
	{
		lastAckTime = currentAckTimeStamp;
		return;
	}

	unsigned short int seqNum = *(unsigned int *)(packetData + 1);
	unsigned short int echoedPacketSize = *(unsigned short int *)(packetData + 1 + globalConsts::USHORT_INT_SIZE);



	unsigned long long ackTimeDiff = (currentAckTimeStamp - lastAckTime);
	vector<UdpPacketInfo>::iterator vecIterator;
	UdpAck tmpUdpAck;

	unsigned long long timeDiff = 0;
	// Average the throughput over all the packets being acknowledged.
	if(numRedunAcks > 0)
	{
		int i;
		unsigned short int redunSeqNum;
		unsigned short int redunPacketSize;

		for(i = 0;i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned short int *)(packetData + 1 + globalConsts::minAckPacketSize + i*globalConsts::redunAckSize);
			redunPacketSize = *(unsigned short int *)(packetData + 1 + globalConsts::minAckPacketSize + i*globalConsts::redunAckSize + globalConsts::USHORT_INT_SIZE);

			// Find if this redundant ACK is useful - or it was acked before.
			vecIterator = find_if(udpStateInfo.currentAckedPackets.begin(), udpStateInfo.currentAckedPackets.end(), bind2nd(equalSeqNum(), redunSeqNum));

			if(vecIterator != udpStateInfo.currentAckedPackets.end())
			{
				// Calculate throughput for the packet being acked by
				// the redundant ACK.
				numThroughputAcks++;

				timeDiff = *(unsigned long long *)(packetData + 1 + globalConsts::minAckPacketSize + i*globalConsts::redunAckSize + globalConsts::seqNumSize);

				if(ackTimeDiff - timeDiff == 0)
					continue;

				tmpUdpAck.timeTaken = ackTimeDiff - timeDiff;


				// We lost the record of the size of this packet due to libpcap
				// loss, use the length echoed back in the ACK.
				if((*vecIterator).isFake == true)
				{
					avgThroughput += 8000000.0*( static_cast<double> ( redunPacketSize ))  / ( static_cast<double>(ackTimeDiff - timeDiff)*1024.0 );
					tmpUdpAck.packetSize = redunPacketSize;
				}
				else
				{
					avgThroughput += 8000000.0*( static_cast<double> ( (*vecIterator).packetSize ))  / ( static_cast<double>(ackTimeDiff - timeDiff)*1024.0 );
					tmpUdpAck.packetSize = (*vecIterator).packetSize;
				}

				ackList[queuePtr] = tmpUdpAck;
				queuePtr = (queuePtr + 1)%100;
				if(minSamples < 100)
					minSamples++;

				ackTimeDiff = timeDiff;
			}

		}
	}

	if(ackTimeDiff == 0)
	{
		calculateTput(timeStamp);
		lastAckTime = currentAckTimeStamp;
		return;
	}

	// Calculate the throughput for the current packet being ACKed.
	vecIterator = find_if(udpStateInfo.currentAckedPackets.begin(), udpStateInfo.currentAckedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	if(vecIterator == udpStateInfo.currentAckedPackets.end())
	{
		printf("Error - Incorrect ack packets state passed to AvgThroughputSensor \n\n\n");
		return;
	}

	// We lost the record of the size of this packet due to libpcap
	// loss, use the length echoed back in the ACK.
	if(udpStateInfo.isAckFake == true)
	{
		avgThroughput += 8000000.0*( static_cast<double> (echoedPacketSize ))  / ( static_cast<double>(ackTimeDiff)*1024.0 );
		tmpUdpAck.packetSize = echoedPacketSize;
	}
	else
	{
		avgThroughput += 8000000.0*( static_cast<double> ((*vecIterator).packetSize ))  / ( static_cast<double>(ackTimeDiff)*1024.0 );
		tmpUdpAck.packetSize = (*vecIterator).packetSize;
	}

	tmpUdpAck.timeTaken = ackTimeDiff - timeDiff;

	ackList[queuePtr] = tmpUdpAck;
	queuePtr = queuePtr + 1;
	minSamples++;

	if(minSamples < 100)
		minSamples++;

	calculateTput(timeStamp);

	// Save the receiver timestamp of this ACK packet, so that we can
	// use for calculating throughput for the next ACK packet.
	lastAckTime = currentAckTimeStamp;
}

void UdpAvgThroughputSensor::calculateTput(unsigned long long timeStamp)
{
	// We don't have enough samples to calculate the average throughput.
	if(minSamples < 5)
		return;

	int sampleCount = ( minSamples < 100 )?minSamples:100;
	int i, index;
	unsigned long long timePeriod = 0;
	long packetSizeSum = 0;

	for(i = 0;(i < sampleCount&& timePeriod < 500000); i++)
	{
		index = (queuePtr - i + 100)%100;

		timePeriod += ackList[index].timeTaken;
		packetSizeSum += ackList[index].packetSize;
	}

	throughputKbps = 8000000.0*( static_cast<double> (packetSizeSum ))  / ( static_cast<double>(timePeriod)*1024.0 );
	// Calculate the average throughput.

	// Send a message to the monitor with the new bandwidth.
	if(lossSensor->getPacketLoss() == 0)
	{
		// Send this available bandwidth as a tentative value.
		// To be used for dummynet events only if it is greater
		// than the last seen value.
		//logStream << "VALUE::Tentative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<"acktimeDiff = "<<ackTimeDiff<<"\n";

		logStream << "AVGTPUT:TIME="<<timeStamp<<",TENTATIVE="<<throughputKbps<<endl;
		logStream << "LOSS:TIME="<<timeStamp<<",LOSS=0"<<endl;
	}
	else
	{
		// Send this as the authoritative available bandwidth value.
		//logStream << "VALUE::Authoritative bandwidth for seqNum = "<<seqNum<<", value = "<< throughputKbps <<"ackTimeDiff = "<<ackTimeDiff<<"\n";
		logStream << "AVGTPUT:TIME="<<timeStamp<<",AUTHORITATIVE="<<throughputKbps<<endl;
		logStream << "LOSS:TIME="<<timeStamp<<",LOSS="<<lossSensor->getPacketLoss()<<endl;

		lossSensor->resetLoss();
	}
	logStream << "STAT:TOTAL_LOSS = "<<lossSensor->totalLoss<<endl;
}
