/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpThroughputSensor.h"

using namespace std;

UdpThroughputSensor::UdpThroughputSensor(UdpPacketSensor const * udpPacketSensorVal)
	: lastAckTime(0),
	throughputKbps(0.0),
	packetHistory(udpPacketSensorVal)
{
	// Do nothing.

}

UdpThroughputSensor::~UdpThroughputSensor()
{

}

void UdpThroughputSensor::localSend(PacketInfo *packet)
{
	sendValid = true;
	ackValid = false;
}

void UdpThroughputSensor::localAck(PacketInfo *packet)
{
	// This is a re-ordered ACK - don't do anything
	// with it - just return.
	sendValid = false;

	if( packetHistory->isAckValid() != true )
	{
		ackValid = false;
		return;
	}
	else
		ackValid = true;

	// Find out how many redundant ACKs this packet is carrying - 0 to 121.
	unsigned char numRedunAcksChar = 0;
	memcpy(&numRedunAcksChar, &packet->payload[0], global::UCHAR_SIZE);
	int numRedunAcks = static_cast<int>(numRedunAcksChar);

	int numThroughputAcks = 1;
	double avgThroughput = 0;

	// This is the timestamp at the receiver, when the original packet was received.
	unsigned long long currentAckTimeStamp = *(unsigned long long *)(packet->payload + 1 + 2*global::USHORT_INT_SIZE ); 

	// This is the first ACK we have seen, store its receiver timestamp
	// and return, we cannot calculate throughput from just one ACK - at least 2.
	if(lastAckTime == 0)
	{
		lastAckTime = currentAckTimeStamp;
		return;
	}

	unsigned short int seqNum = *(unsigned int *)(packet->payload + 1);
	unsigned short int echoedPacketSize = *(unsigned short int *)(packet->payload + 1 + global::USHORT_INT_SIZE);

	unsigned long long ackTimeDiff = currentAckTimeStamp - lastAckTime;
	unsigned long long timeDiff = 0;
	vector<UdpPacketInfo > ackedPackets = packetHistory->getAckedPackets();
	vector<UdpPacketInfo>::iterator vecIterator;

	// Average the throughput over all the packets being acknowledged.
	if(numRedunAcks > 0)
	{
		int i;
		unsigned short int redunSeqNum;
		unsigned short int redunPacketSize;

		for(i = 0;i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned short int *)(packet->payload + 1 + global::udpMinAckPacketSize + i*global::udpRedunAckSize);
			redunPacketSize = *(unsigned short int *)(packet->payload + 1 + global::udpMinAckPacketSize + i*global::udpRedunAckSize + global::USHORT_INT_SIZE);

			// Find if this redundant ACK is useful - or it was acked before.
			vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), redunSeqNum));

			if(vecIterator != ackedPackets.end())
			{

				timeDiff = *(unsigned long long *)(packet->payload + 1 + global::udpMinAckPacketSize + i*global::udpRedunAckSize + global::udpSeqNumSize);

				// Avoid dividing by zero.
				if(ackTimeDiff - timeDiff == 0)
					continue;

				// Calculate throughput for the packet being acked by
				// the redundant ACK.
				numThroughputAcks++;

				// We lost the record of the size of this packet due to libpcap
				// loss, use the length echoed back in the ACK.
				if((*vecIterator).isFake == true)
					avgThroughput += 8000000.0*( static_cast<double> ( redunPacketSize ))  / ( static_cast<double>(ackTimeDiff - timeDiff)*1024.0 );
				else
					avgThroughput += 8000000.0*( static_cast<double> ( (*vecIterator).packetSize ))  / ( static_cast<double>(ackTimeDiff - timeDiff)*1024.0 );

					ackTimeDiff = timeDiff;
			}

		}
	}

	// Calculate the throughput for the current packet being ACKed.
	vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Avoid dividing by zero.
	if(ackTimeDiff != 0)
	{
		// We lost the record of the size of this packet due to libpcap
		// loss, use the length echoed back in the ACK.
		if(packetHistory->isAckFake() == true)
			avgThroughput += 8000000.0*( static_cast<double> (echoedPacketSize ))  / ( static_cast<double>(ackTimeDiff)*1024.0 );
		else
			avgThroughput += 8000000.0*( static_cast<double> ((*vecIterator).packetSize ))  / ( static_cast<double>(ackTimeDiff)*1024.0 );
	}

	throughputKbps = avgThroughput / (static_cast<double> (numThroughputAcks) );

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();
	// Send a message to the monitor with the new bandwidth.
	if(packetHistory->getPacketLoss() == 0)
	{
		// Send this available bandwidth as a tentative value.
		// To be used for dummynet events only if it is greater
		// than the last seen value.
		logWrite(SENSOR, "VALUE::Tentative bandwidth for seqNum = %d , value = %f, acktimeDiff = %llu,packet size = %u",seqNum, throughputKbps, ackTimeDiff, (*vecIterator).packetSize);

		//CHANGE:
		logWrite(SENSOR, "TPUT:TIME=%llu,TENTATIVE=%f",timeStamp,throughputKbps);
		//logWrite(SENSOR, "LOSS:TIME=%llu,LOSS=0", timeStamp);
	}
	else
	{
		// Send this as the authoritative available bandwidth value.
		logWrite(SENSOR,"VALUE::Authoritative bandwidth for seqNum = %d , value = %f ,ackTimeDiff = %llu ",seqNum,throughputKbps,ackTimeDiff);
		logWrite(SENSOR, "TPUT:TIME=%llu,AUTHORITATIVE=%f",timeStamp,throughputKbps);
		//logWrite(SENSOR,"LOSS:TIME=%llu,LOSS=%d",timeStamp,packetHistory->getPacketLoss());
	}

	// Save the receiver timestamp of this ACK packet, so that we can
	// use for calculating throughput for the next ACK packet.
	lastAckTime = currentAckTimeStamp;
}
