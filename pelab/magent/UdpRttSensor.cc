#include "UdpRttSensor.h"

using namespace std;

UdpRttSensor::UdpRttSensor(UdpPacketSensor const *packetHistoryVal)
	: ewmaRtt(0),
	ewmaDevRtt(0),
	packetHistory(packetHistoryVal)
{

}

UdpRttSensor::~UdpRttSensor()
{

}


void UdpRttSensor::localSend(PacketInfo *packet)
{

}

void UdpRttSensor::localAck(PacketInfo *packet)
{
	 // This is a re-ordered ACK - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
	{
		ackValid = false;
		return;
	}

	// We lost information about this packet's send time due to loss
	// in libpcap - don't use this packet for calculating RTT.
	if(packetHistory->isAckFake() == true)
		return;

        unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);
        unsigned long long currentRtt;
	vector<UdpPacketInfo> ackedPackets = packetHistory->getAckedPackets();

        vector<UdpPacketInfo>::iterator vecIterator;
        vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Find the RTT for this packet.

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();
	currentRtt = (timeStamp - (*vecIterator).timeStamp)/2;

	// Scale the value of one way RTT, so that it is correct for a transmission
	// size of 1518 bytes.

	// We lost this packet size details due to loss in libpcap, use the
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	currentRtt = ( currentRtt )*1518 / ((*vecIterator).packetSize);

	double alpha = 0.25, beta = 0.125;
	if(ewmaRtt == 0)
	{
		ewmaRtt = currentRtt;
	}
	else
	{
		if(ewmaDevRtt == 0)
		{
			if(currentRtt > ewmaRtt)
				ewmaDevRtt = currentRtt - ewmaRtt ;
			else
				ewmaDevRtt = ewmaRtt - currentRtt ;
		}
		else
		{
			if(currentRtt > ewmaRtt)
				ewmaDevRtt = ewmaDevRtt*(1 - beta) + beta*(currentRtt - ewmaRtt);
			else
				ewmaDevRtt = ewmaDevRtt*(1 - beta) + beta*(ewmaRtt - currentRtt );
		}

		ewmaRtt = ewmaRtt*(1 - alpha) + currentRtt*alpha;
	}
}

unsigned long long UdpRttSensor::getRtt() const
{
	return ewmaRtt;
}

unsigned long long UdpRttSensor::getDevRtt() const
{
	return ewmaDevRtt;
}
