#include "UdpPacketSensor.h"

UdpPacketSensor::UdpPacketSensor(UdpState &udpStateVal)
	:udpStateInfo(udpStateVal)
{
	lastPacketTime = 0;


}

void UdpPacketSensor::localSend(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	int minSize = 2*sizeof(unsigned int);
	if(Len < minSize)
	{
		cout << "Error: UDP packet data sent to PacketSensor::localSend was less than the "
			" required minimum "<< minSize << " bytes\n";
		return;
	}
	unsigned int seqNum = *(unsigned int *)(packetData);
	unsigned int packetSize = *(unsigned int *)(packetData + sizeof(unsigned int));
	bool isFastPacket = false;
	unsigned long long sendTimeDelta = 0;

	if(lastPacketTime == 0)
		lastPacketTime = timeStamp;
	else
	{

		sendTimeDelta = timeStamp - lastPacketTime;
		lastPacketTime = timeStamp;

		if(sendTimeDelta < udpStateInfo.minDelay)
			isFastPacket = 1;

		// We did not receive an ACK back, so we dont know the minimum
		// one way delay. Treat all packets as fast packets until we 
		// find out the min. delay - this lowers the throughput a bit
		// in case of packet loss if the packets were not being sent
		// faster than min delay.
		if(udpStateInfo.minDelay == 0)
			isFastPacket = 1;
	}
	if(! sentPacketList.empty())
		sentPacketList.back()->lastTimeDiff = sendTimeDelta;

	sentPacketList.push_back(new UdpPacketInfo(seqNum, packetSize, timeStamp, isFastPacket));
}

void UdpPacketSensor::localAck(char *packetData, int Len, int overheadLen, unsigned long long timeStamp)
{
	int minSize = 1 + sizeof(unsigned int) + sizeof(unsigned long long);
	int i;

	// Remove the old state information.
	for(i = 0;i < udpStateInfo.recentSentPackets.size(); i++)
		delete udpStateInfo.recentSentPackets[i];
	udpStateInfo.recentSentPackets.clear();
	udpStateInfo.packetLoss = 0;
	udpStateInfo.fastPacketLoss = 0;
	udpStateInfo.lostPacketDelay = 0;

	udpStateInfo.lastSentTime = (ULONG_LONG_MAX);

	if(Len < minSize)
	{
		cout << "Error: UDP packet data sent to PacketSensor::localAck was less than the "
			" minimum "<< minSize << " bytes\n";
		return;
	}

	int redunAckSize = sizeof(unsigned int) + sizeof(unsigned long);

	int numRedunAcks = static_cast<int>(packetData[0]);

	unsigned int seqNum = *(unsigned int *)(packetData + 1);

	// Find the entry for the packet this ACK is acknowledging, and
	// remove it from the sent(&unacked) packet list.
	list<UdpPacketInfo * >::iterator listIterator;

	listIterator = find_if(sentPacketList.begin(), sentPacketList.end(), bind2nd(equalSeqNum(), seqNum)); 

	if(listIterator == sentPacketList.end())
	{
		cout << "ERROR: Unacked packet list is incorrect Or incorrect"
			"acknowledgement received for seqNum = "<<seqNum<<" in PacketSensor::localAck\n";
		return;
	}


	// Store an iterator to the current seqNum being acknowledge, and delete it at the end.
	list<UdpPacketInfo * >::iterator curPacketIterator = listIterator;

	// Look at the redundant ACKs first.

	if(numRedunAcks > 0)
	{
		unsigned int redunSeqNum;

		for(i = 0; i < numRedunAcks; i++)
		{
			redunSeqNum = *(unsigned int *)(packetData + minSize + i*redunAckSize);
			listIterator = sentPacketList.end();

			// Check whether the packet that this redundant ACK refers to exists
			// in our list of sent ( but unacked ) packets. It might not exist
			// if its ACK was not lost earlier.
			//listIterator = find_if(sentPacketList.begin(), sentPacketList.end(), bind2nd(equalSeqNum(), redunSeqNum)); 
			listIterator = find_if(sentPacketList.begin(), curPacketIterator, bind2nd(equalSeqNum(), redunSeqNum)); 

			// An unacked packet exists with this sequence number, delete it 
			// from the list and consider it acked.
			if(listIterator != curPacketIterator && listIterator != sentPacketList.end())
			{
				udpStateInfo.recentSentPackets.push_back(new UdpPacketInfo((*listIterator)->seqNum, (*listIterator)->packetSize, (*listIterator)->timeStamp, (*listIterator)->isFastPacket) );

				if( (*listIterator)->timeStamp < udpStateInfo.lastSentTime)
					udpStateInfo.lastSentTime = (*listIterator)->timeStamp;

				delete (*listIterator);
				sentPacketList.erase(listIterator);
			}
		}
	}

	udpStateInfo.recentSentPackets.push_back(new UdpPacketInfo((*curPacketIterator)->seqNum, (*curPacketIterator)->packetSize, (*curPacketIterator)->timeStamp, (*curPacketIterator)->isFastPacket));

	// Check for packet loss - if we have any unacked packets with sequence
	// numbers less than the received ACK seq number, then the packets/or their ACKS
	// were lost - treat this as congestion on the forward path.

	// Find out how many packets were lost.

	listIterator = find_if(sentPacketList.begin(), sentPacketList.end(), bind2nd(lessSeqNum(), seqNum  )); 

	if(listIterator != sentPacketList.end())
	{

		do{
			if( (*listIterator)->timeStamp < udpStateInfo.lastSentTime)
				udpStateInfo.lastSentTime = (*listIterator)->timeStamp;

			if( (*listIterator)->isFastPacket)
				udpStateInfo.fastPacketLoss++;

			udpStateInfo.lostPacketDelay += (*listIterator)->lastTimeDiff;

			delete (*listIterator);
			sentPacketList.erase(listIterator);
			listIterator = sentPacketList.end();

			udpStateInfo.packetLoss++;

			listIterator = find_if(sentPacketList.begin(), curPacketIterator, bind2nd(lessSeqNum(), seqNum )); 

		}
		while( (listIterator != sentPacketList.end()) && (listIterator != curPacketIterator) );

	}

	if( (*curPacketIterator)->timeStamp < udpStateInfo.lastSentTime)
		udpStateInfo.lastSentTime = (*curPacketIterator)->timeStamp;
	delete (*curPacketIterator);
	sentPacketList.erase(curPacketIterator);
}
