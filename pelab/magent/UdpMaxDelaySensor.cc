#include "UdpMaxDelaySensor.h"

using namespace std;

UdpMaxDelaySensor::UdpMaxDelaySensor(UdpPacketSensor const *udpPacketSensorVal, UdpMinDelaySensor const *minDelaySensorVal)
	: maxDelay(0),
	packetHistory(udpPacketSensorVal),
	minDelaySensor(minDelaySensorVal)
{

}

UdpMaxDelaySensor::~UdpMaxDelaySensor()
{

}


void UdpMaxDelaySensor::localSend(PacketInfo *packet)
{

}

void UdpMaxDelaySensor::localAck(PacketInfo *packet)
{
	 // This is a re-ordered ACK or a corrupted packet - don't do anything
	// with it - just return.
	if( packetHistory->isAckValid() == false )
		return;

	int overheadLen = 14 + 4 + 8 + packet->ip->ip_hl*4;
        unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);
        unsigned short int echoedPacketSize = *(unsigned short int *)(packet->payload + 1 + global::USHORT_INT_SIZE);
	unsigned long long echoedTimestamp = *(unsigned long long *)(packet->payload + 1 + 2*global::USHORT_INT_SIZE + global::ULONG_LONG_SIZE);
        unsigned long long oneWayQueueDelay;
        bool eventFlag = false;

	vector<UdpPacketInfo>::iterator vecIterator;
	vector<UdpPacketInfo> ackedPackets = packetHistory->getAckedPackets();
        vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

	// Find the one way RTT for this packet.

	unsigned long long timeStamp = packet->packetTime.toMicroseconds();
	// We lost this packet send time due to loss in libpcap, use the
	// time echoed in the ACK packet.
	if(packetHistory->isAckFake() == true)
		oneWayQueueDelay = (timeStamp - echoedTimestamp)/2;
	else
		oneWayQueueDelay = (timeStamp - (*vecIterator).timeStamp)/2;

	// Scale the value of one way RTT, so that it is correct for a transmission
	// size of 1518 bytes.

	// We lost this packet size details due to loss in libpcap, use the
	// size echoed in the ACK packet - this does not included the header
	// overhead for the packet - we assume that the packet on the reverse path
	// has the same overhead length as the original packet.
	if(packetHistory->isAckFake() == true)
		oneWayQueueDelay = ( oneWayQueueDelay )*1518 / (overheadLen + echoedPacketSize);
	else
		oneWayQueueDelay = ( oneWayQueueDelay )*1518 / ((*vecIterator).packetSize);

	// Find the queuing delay for this packet, by subtracting the
	// one way minimum delay from the above value.
	oneWayQueueDelay = oneWayQueueDelay - minDelaySensor->getMinDelay();

        // Set this as the new maximum one way queuing delay.
        if(oneWayQueueDelay > maxDelay)
        {
                eventFlag = true;
                maxDelay = oneWayQueueDelay;
        }

        // Send an event message to the monitor to change the value of maximum one way delay.
        if(eventFlag == true)
        {
		// Report the maximum delay
		logWrite(SENSOR,"VALUE::New Max Delay = %llu",maxDelay);
        }
	logWrite(SENSOR,"MAXD:TIME=%llu,MAXD=%llu",timeStamp,maxDelay);
	logWrite(SENSOR,"ACTUAL_MAXD:TIME=%llu,ACTUAL_MAXD=%llu",timeStamp,oneWayQueueDelay);

}
