/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpMaxDelaySensor.h"
#include "CommandOutput.h"

using namespace std;

UdpMaxDelaySensor::UdpMaxDelaySensor(UdpPacketSensor const *udpPacketSensorVal, UdpMinDelaySensor const *minDelaySensorVal, UdpLossSensor const *lossSensorVal)
: maxDelay(0),sentDelay(0),
    packetHistory(udpPacketSensorVal),
    minDelaySensor(minDelaySensorVal),
    lossSensor(lossSensorVal)
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
    // If this packet is ACKing a packet that we lost due to libpcap send loss,
    // dont use this packet timestamp for RTT calculations.
    if( packetHistory->isAckValid() == false || packetHistory->isAckFake() == true)
        return;

    unsigned short int seqNum = *(unsigned short int *)(packet->payload + 1);
    unsigned long long oneWayQueueDelay;

    vector<UdpPacketInfo>::iterator vecIterator;
    vector<UdpPacketInfo> ackedPackets = packetHistory->getAckedPackets();
    vecIterator = find_if(ackedPackets.begin(), ackedPackets.end(), bind2nd(equalSeqNum(), seqNum));

    if(vecIterator == ackedPackets.end())
    {
        logWrite(ERROR,"UdpMaxDelay: Cannot find seqNum=%d in AckedPacketList", seqNum);
        return;
    }


    unsigned long long timeStamp = packet->packetTime.toMicroseconds();

    // Find the one way RTT for this packet.
    oneWayQueueDelay = (timeStamp - (*vecIterator).timeStamp)/2;

    // Scale the value of one way RTT, so that it is correct for a transmission
    // size of 1518 bytes.

    // We lost this packet size details due to loss in libpcap, use the
    // size echoed in the ACK packet - this does not included the header
    // overhead for the packet - we assume that the packet on the reverse path
    // has the same overhead length as the original packet.
    //oneWayQueueDelay = ( oneWayQueueDelay )*1518 / ((*vecIterator).packetSize);
    // NOTE: Do not scale the queuing delay - the results are bad for small
    // packet transmissions.

    //if(oneWayQueueDelay < minDelaySensor->getMinDelay())
    //{
    //	logWrite(ERROR,"Incorrect oneWayQueueDelay value = %llu, minDelay = %llu, seqNum=%d, SENDTS=%llu,RECVTS=%llu", oneWayQueueDelay, minDelaySensor->getMinDelay(),seqNum, (*vecIterator).timeStamp, timeStamp);
    //	return;

    //}
    // Find the queuing delay for this packet, by subtracting the
    // one way minimum delay from the above value.
    if(oneWayQueueDelay > minDelaySensor->getMinDelay())
        oneWayQueueDelay = oneWayQueueDelay - minDelaySensor->getMinDelay();
    else
        oneWayQueueDelay = 0;


    if(oneWayQueueDelay > 90000000)
    {
        logWrite(ERROR,"Incorrect oneWayQueueDelay value = %llu, minDelay = %llu, seqNum=%d, SENDTS=%llu,RECVTS=%llu", oneWayQueueDelay, minDelaySensor->getMinDelay(),seqNum, (*vecIterator).timeStamp, timeStamp);
        return;
    }

    // Set this as the new maximum one way queuing delay.
    if(oneWayQueueDelay > maxDelay)
        maxDelay = oneWayQueueDelay;

    // Send an event message to the monitor to change the value of maximum one way delay.
    if(lossSensor->getPacketLoss() > 0)
    {
        // Report the maximum delay
        ostringstream messageBuffer;
        if(maxDelay > sentDelay)
        {
            messageBuffer<<"MAXINQ="<<(maxDelay+minDelaySensor->getMinDelay())/1000;
            global::output->eventMessage(messageBuffer.str(), packet->elab);
            sentDelay = maxDelay;
        }

        logWrite(SENSOR,"VALUE::New Max Delay = %llu",maxDelay);
    }
    logWrite(SENSOR,"MAXD:TIME=%llu,MAXD=%llu",timeStamp,maxDelay);
    logWrite(SENSOR,"ACTUAL_MAXD:TIME=%llu,ACTUAL_MAXD=%llu",timeStamp,oneWayQueueDelay);

}
