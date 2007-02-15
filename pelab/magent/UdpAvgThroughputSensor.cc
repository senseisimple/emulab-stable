/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpAvgThroughputSensor.h"
#include "CommandOutput.h"

using namespace std;

UdpAvgThroughputSensor::UdpAvgThroughputSensor(UdpPacketSensor const *packetHistoryVal, UdpLossSensor const *lossSensorVal)
	: packetHistory(packetHistoryVal),
	lossSensor(lossSensorVal),
	lastAckTime(0),
	throughputKbps(0.0),
	lastSeenThroughput(0),
	numSamples(0),
	queuePtr(0)
{
}

UdpAvgThroughputSensor::~UdpAvgThroughputSensor()
{

}

void UdpAvgThroughputSensor::localSend(PacketInfo *packet)
{
	// Do nothing.

}

void UdpAvgThroughputSensor::localAck(PacketInfo *packet)
{

	// This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
	{
		ackValid = false;
		return;
	}

	// Find out how many redundant ACKs this packet is carrying - 0 to 121.
	unsigned char numRedunAcksChar = 0;
	memcpy(&numRedunAcksChar, &packet->payload[0], global::UCHAR_SIZE);
	int numRedunAcks = static_cast<int>(numRedunAcksChar);

	double avgThroughput = 0;

	// This is the timestamp at the receiver, when the original packet was received.
	unsigned long long currentAckTimeStamp = *(unsigned long long *)(packet->payload + 1 + 2*global::USHORT_INT_SIZE ); 
	unsigned long long timeStamp = packet->packetTime.toMicroseconds();

	// This is the first ACK we have seen, store its receiver timestamp
	// and return, we cannot calculate throughput from just one ACK - at least 2.
	if(lastAckTime == 0)
	{
		lastAckTime = currentAckTimeStamp;
		return;
	}

	unsigned short int seqNum = *(unsigned int *)(packet->payload + 1);
	unsigned short int echoedPacketSize = *(unsigned short int *)(packet->payload + 1 + global::USHORT_INT_SIZE);



	unsigned long long ackTimeDiff = (currentAckTimeStamp - lastAckTime);
	vector<UdpPacketInfo>::iterator vecIterator;
	UdpAck tmpUdpAck;
	vector<UdpPacketInfo > ackedPackets = packetHistory->getAckedPackets();

	unsigned long long timeDiff = 0;
	// Average the throughput over all the packets being acknowledged.
	if(numRedunAcks > 0)
	{
		int i;
		unsigned short int redunSeqNum;
		unsigned short int redunPacketSize;

		for(i = 0;i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned short int *)(packet->payload + global::udpMinAckPacketSize + i*global::udpRedunAckSize);
			redunPacketSize = *(unsigned short int *)(packet->payload + global::udpMinAckPacketSize + i*global::udpRedunAckSize + global::USHORT_INT_SIZE);

			// Find if this redundant ACK is useful - or it was acked before.
			vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), redunSeqNum));

			if(vecIterator != ackedPackets.end())
			{
				// Calculate throughput for the packet being acked by
				// the redundant ACK.

				timeDiff = *(unsigned long long *)(packet->payload + global::udpMinAckPacketSize + i*global::udpRedunAckSize + 2*global::USHORT_INT_SIZE);

				if((timeDiff > ackTimeDiff) || (ackTimeDiff - timeDiff == 0))
				{
					logWrite(EXCEPTION, "Error using UDP redun Seqnum = %d, for seqNum = %d, time taken = %llu,ackTimeDiff = %llu, timeDiff = %llu, i = %d, numAcks = %d",redunSeqNum,seqNum, ackTimeDiff - timeDiff,ackTimeDiff,timeDiff, i, numRedunAcks);
					continue;
				}

				tmpUdpAck.timeTaken = ackTimeDiff - timeDiff;
				tmpUdpAck.isRedun = true;
				tmpUdpAck.seqNum = redunSeqNum;


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
				queuePtr = (queuePtr + 1)%MAX_SAMPLES;
				if(numSamples < MAX_SAMPLES)
					numSamples++;

				ackTimeDiff = timeDiff;
			}

		}
	}

	if(ackTimeDiff == 0)
	{
		calculateTput(timeStamp, packet);
		lastAckTime = currentAckTimeStamp;
		return;
	}

	// Calculate the throughput for the current packet being ACKed.
	vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	if(vecIterator == ackedPackets.end())
	{
		logWrite(ERROR,"Error - Incorrect ack packets state passed to UdpAvgThroughputSensor");
		return;
	}

	// We lost the record of the size of this packet due to libpcap
	// loss, use the length echoed back in the ACK.
	if(packetHistory->isAckFake() == true)
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
	tmpUdpAck.isRedun = false;
	tmpUdpAck.seqNum = seqNum;

	ackList[queuePtr] = tmpUdpAck;
	queuePtr = (queuePtr + 1)%MAX_SAMPLES;

	if(numSamples < MAX_SAMPLES)
		numSamples++;

	calculateTput(timeStamp, packet);

	// Save the receiver timestamp of this ACK packet, so that we can
	// use for calculating throughput for the next ACK packet.
	lastAckTime = currentAckTimeStamp;
}

void UdpAvgThroughputSensor::calculateTput(unsigned long long timeStamp, PacketInfo *packet)
{
	// We don't have enough samples to calculate the average throughput.
	if(numSamples < MIN_SAMPLES)
		return;

	int sampleCount = ( numSamples < MAX_SAMPLES )?numSamples:MAX_SAMPLES;
	int i, index;
	unsigned long long timePeriod = 0;
	long packetSizeSum = 0;

	for(i = 0;(i < sampleCount && timePeriod < MAX_TIME_PERIOD); i++)
	{
		index = (queuePtr -1 - i + MAX_SAMPLES)%MAX_SAMPLES;

		timePeriod += ackList[index].timeTaken;
		packetSizeSum += ackList[index].packetSize;
	}

	// Avoid dividing by zero.
	if(timePeriod == 0)
	{
		logWrite(ERROR, "Timeperiod is zero in UdpAvgThroughput calculation");
		return;
	}

	if(timePeriod > 5000000)
	{
		logWrite(ERROR, " Incorrect UdpAvgThroughput timePeriod = %llu, bytes = %d, i = %d, numSamples = %d, sampleCount = %d queuePtr = %d", timePeriod, packetSizeSum, i,numSamples,sampleCount, queuePtr);
		int k;
		for(k = 0; k < i; k++)
		{
			index = (queuePtr -1 - k + MAX_SAMPLES)%MAX_SAMPLES;
			logWrite(ERROR, "Wrong UDP seqnum = %d, index = %d, bytes = %d, timePeriod = %llu, isRedun = %d", ackList[index].seqNum,index, ackList[index].packetSize, ackList[index].timeTaken, ackList[index].isRedun);
		}

		return;
	}

	// Calculate the average throughput.
	throughputKbps = 8000000.0*( static_cast<double> (packetSizeSum ))  / ( static_cast<double>(timePeriod)*1024.0 );


	int tputValue = static_cast<int>(throughputKbps);
	//Avoid sending a zero throughput value to the monitor.
	if(tputValue == 0)
		tputValue = 1;
	// Send a message to the monitor with the new bandwidth.
	if(lossSensor->getPacketLoss() == 0 )
	{
		if(tputValue > lastSeenThroughput )
		{
			// Send this available bandwidth as a tentative value.
			// To be used for dummynet events only if it is greater
			// than the last seen value.

			ostringstream messageBuffer;
			messageBuffer << tputValue;
			global::output->genericMessage(TENTATIVE_THROUGHPUT, messageBuffer.str(), packet->elab);
		}
		logWrite(SENSOR, "AVGTPUT:TIME=%llu,TENTATIVE=%f",timeStamp,throughputKbps);
		logWrite(SENSOR, "LOSS:TIME=%llu,LOSS=0",timeStamp);
	}
	else
	{
		ostringstream messageBuffer;
		messageBuffer << tputValue;
		global::output->genericMessage(AUTHORITATIVE_BANDWIDTH, messageBuffer.str(), packet->elab);
		// Send this as the authoritative available bandwidth value.
		logWrite(SENSOR, "AVGTPUT:TIME=%llu,AUTHORITATIVE=%f",timeStamp,throughputKbps);
		logWrite(SENSOR, "LOSS:TIME=%llu,LOSS=%d",timeStamp,lossSensor->getPacketLoss());

		(const_cast<UdpLossSensor *>(lossSensor))->resetLoss();
	}
	lastSeenThroughput = tputValue;
	logWrite(SENSOR, "STAT:TOTAL_LOSS = %d",lossSensor->getTotalPacketLoss());
}
