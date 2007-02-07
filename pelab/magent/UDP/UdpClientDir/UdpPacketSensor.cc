/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpPacketSensor.h"

UdpPacketSensor::UdpPacketSensor(UdpState &udpStateVal, ofstream &logStreamVal)
	:udpStateInfo(udpStateVal),
	lastSeenSeqNum(-1),
	logStream(logStreamVal)
{
	statReorderedPackets = 0;

}

UdpPacketSensor::~UdpPacketSensor()
{
	// Empty the list used to store the packets.
	sentPacketList.clear();

}

void UdpPacketSensor::localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	int minSize = 2*globalConsts::USHORT_INT_SIZE;

	if(Len < minSize)
	{
		logStream << "ERROR::UDP packet data sent to PacketSensor::localSend was less than the "
			" required minimum "<< minSize << " bytes\n";
		return;
	}

	unsigned short int seqNum = *(unsigned short int *)(packetData);
	unsigned short int packetSize = *(unsigned short int *)(packetData + globalConsts::USHORT_INT_SIZE) + overheadLen;
	UdpPacketInfo tmpPacketInfo;

	if(lastSeenSeqNum != -1)
	{
		// We missed some packets because of loss in libpcap buffer.
		// Add fake packets to the sent list, their sizes and time stamps
		// are unknown - but they can be gathered from the ACK packets.
		if(seqNum > (lastSeenSeqNum + 1))
		{
			for(int i = 1;i < seqNum - lastSeenSeqNum ; i++)
			{
				tmpPacketInfo.seqNum = lastSeenSeqNum + i;
				tmpPacketInfo.isFake = true;

				sentPacketList.push_back(tmpPacketInfo);
			}
			udpStateInfo.libpcapSendLoss += (seqNum - lastSeenSeqNum - 1);
		}
	}

	lastSeenSeqNum = seqNum;

	tmpPacketInfo.seqNum = seqNum;
	tmpPacketInfo.packetSize = packetSize;
	tmpPacketInfo.timeStamp = timeStamp;
	tmpPacketInfo.isFake = false;

	sentPacketList.push_back(tmpPacketInfo);
}

void UdpPacketSensor::localAck(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	if(Len < globalConsts::minAckPacketSize)
	{
		logStream << "ERROR::UDP packet data sent to PacketSensor::localAck was less than the "
			" minimum "<< globalConsts::minAckPacketSize << " bytes\n";
		return;
	}

	// Remove the old state information.
	udpStateInfo.currentAckedPackets.clear();
	udpStateInfo.packetLoss = 0;

	unsigned short int seqNum = *(unsigned short int *)(packetData + 1);

	// Find the entry for the packet this ACK is acknowledging, and
	// remove it from the sent(&unacked) packet list.
	list<UdpPacketInfo >::iterator listIterator;

	listIterator = find_if(sentPacketList.begin(), sentPacketList.end(), bind2nd(equalSeqNum(), seqNum)); 

	bool isReordered = handleReorderedAck(packetData, Len, overheadLen, timeStamp);

	if( ( listIterator == sentPacketList.end() ) && ( isReordered == false ) )
	{
		logStream << "WARNING::Unknown seq number "<<seqNum<<" is being ACKed. "
			"We might have received "
			" a reordered ACK, which has already been ACKed using redundant ACKs .\n";
		udpStateInfo.ackError = true;
	}
	else
		udpStateInfo.ackError = false;

	// We received an ACK correctly(without reordering), but we dont have any record of ever
	// sending the original packet(Actually, we have a fake packet inserted into our send list)
       //	-- this indicates libpcap loss.
	if( (*listIterator).isFake == true)
		udpStateInfo.isAckFake = true;
	else
		udpStateInfo.isAckFake = false;



	int i;
	unsigned char numRedunAcksChar = 0; 
	int numRedunAcks = 0; 

	// Read how many redundant ACKs are being sent in this packet.
	memcpy(&numRedunAcksChar, &packetData[0], globalConsts::UCHAR_SIZE);

	numRedunAcks = static_cast<int>(numRedunAcksChar);

	// Store an iterator to the current seqNum being acknowledge, and delete it at the end.
	list<UdpPacketInfo >::iterator curPacketIterator = listIterator;

	// Look at the redundant ACKs first.
	UdpPacketInfo tmpPacketInfo;

	if(numRedunAcks > 0)
	{
		unsigned short int redunSeqNum;

		for(i = 0; i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned short int *)(packetData + globalConsts::minAckPacketSize + i*globalConsts::redunAckSize);
			listIterator = sentPacketList.end();

			// Check whether the packet that this redundant ACK refers to exists
			// in our list of sent ( but unacked ) packets. It might not exist
			// if its ACK was not lost earlier.
			listIterator = find_if(sentPacketList.begin(), curPacketIterator, bind2nd(equalSeqNum(), redunSeqNum)); 

			// An unacked packet exists with this sequence number, delete it 
			// from the list and consider it acked.
			if(listIterator != curPacketIterator && listIterator != sentPacketList.end())
			{
				tmpPacketInfo.seqNum = (*listIterator).seqNum;
				tmpPacketInfo.packetSize = (*listIterator).packetSize;
				tmpPacketInfo.timeStamp = (*listIterator).timeStamp;
				tmpPacketInfo.isFake = (*listIterator).isFake;

				udpStateInfo.currentAckedPackets.push_back(tmpPacketInfo);

				sentPacketList.erase(listIterator);
			}
			else
			{
				// Check whether this ACK corresponds to a packet reordered
				// on the forward path.
				list<UdpPacketInfo >::iterator reOrderedPacketIterator ;
				reOrderedPacketIterator = find_if(unAckedPacketList.begin(), unAckedPacketList.end(), bind2nd(equalSeqNum(), redunSeqNum)); 

				// An unacked packet exists with this sequence number, delete it 
				// from the list and consider it acked.
				if(reOrderedPacketIterator != unAckedPacketList.end())
				{
					tmpPacketInfo.seqNum = (*reOrderedPacketIterator).seqNum;
					tmpPacketInfo.packetSize = (*reOrderedPacketIterator).packetSize;
					tmpPacketInfo.timeStamp = (*reOrderedPacketIterator).timeStamp;
					tmpPacketInfo.isFake = (*reOrderedPacketIterator).isFake;

					udpStateInfo.currentAckedPackets.push_back(tmpPacketInfo);

					unAckedPacketList.erase(reOrderedPacketIterator);
					statReorderedPackets++;
					logStream << "STAT:: Number of reordered packets = "<<statReorderedPackets<<endl;
				}

			}
		}
	}

	tmpPacketInfo.seqNum = (*curPacketIterator).seqNum;
	tmpPacketInfo.packetSize = (*curPacketIterator).packetSize;
	tmpPacketInfo.timeStamp = (*curPacketIterator).timeStamp;
	tmpPacketInfo.isFake = (*curPacketIterator).isFake;

	udpStateInfo.currentAckedPackets.push_back(tmpPacketInfo);

	// Check for packet loss - if we have any unacked packets with sequence
	// numbers less than the received ACK seq number, then the packets/or their ACKS
	// were lost - treat this as congestion on the forward path.

	// Find out how many packets were lost.

	listIterator = find_if(sentPacketList.begin(), curPacketIterator, bind2nd(lessSeqNum(), seqNum)); 

	if( (listIterator != sentPacketList.end()) && (listIterator != curPacketIterator ))
	{
		logStream<<"STAT::Packet being ACKed = "<<seqNum<<endl;

		do{
			logStream<<"STAT::Lost packet seqnum = "<<(*listIterator).seqNum<<endl;

			// This packet might have been lost - but store it as UnAcked
			// to account for reordering on the forward path.
			tmpPacketInfo.seqNum = (*listIterator).seqNum;
			tmpPacketInfo.packetSize = (*listIterator).packetSize;
			tmpPacketInfo.timeStamp = (*listIterator).timeStamp;
			tmpPacketInfo.isFake = (*listIterator).isFake;

			unAckedPacketList.push_back(tmpPacketInfo);
			sentPacketList.erase(listIterator);
			listIterator = sentPacketList.end();

			udpStateInfo.packetLoss++;
			udpStateInfo.totalPacketLoss++;

			listIterator = find_if(sentPacketList.begin(), curPacketIterator, bind2nd(lessSeqNum(), seqNum )); 

		}
		while( (listIterator != sentPacketList.end()) && (listIterator != curPacketIterator) );
		logStream<<"STAT::Total packet loss = "<<udpStateInfo.totalPacketLoss<<"\n"<<endl;
	}

	sentPacketList.erase(curPacketIterator);
}

list<UdpPacketInfo>& UdpPacketSensor::getUnAckedPacketList()
{
	return unAckedPacketList;
}

bool UdpPacketSensor::handleReorderedAck(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	list<UdpPacketInfo >::iterator listIterator;
	unsigned short int seqNum = *(unsigned short int *)(packetData + 1);
	bool retVal = false;

	listIterator = find_if(unAckedPacketList.begin(), unAckedPacketList.end(), bind2nd(equalSeqNum(), seqNum)); 

	if(listIterator == unAckedPacketList.end())
		retVal = false;
	else
	{
		retVal = true;
		statReorderedPackets++;
		logStream << "STAT:: Number of reordered packets = "<<statReorderedPackets<<endl;
	}

	return retVal;
}
