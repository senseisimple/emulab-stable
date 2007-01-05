#ifndef UDP_STATE_PELAB_H
#define UDP_STATE_PELAB_H

#include "UdpLibs.h"
#include "UdpPacketInfo.h"

using namespace std;

class UdpPacketInfo;

struct UdpState{
	// vector of info about packets sent from this host,
        // sequence number, timestamp & size of the packet.
	vector< UdpPacketInfo * > recentSentPackets;

	// Indicates the number of packets lost ( in a batch of 4 sent packets ) 
	// updated whenever an ACK is received.
	int packetLoss, fastPacketLoss;
	unsigned long long lastSentTime;
	unsigned long long minDelay;
	unsigned long long maxDelay;
	unsigned long long minAckTimeDiff;
	unsigned long long queuingDelay;
	unsigned long long lostPacketDelay;

	long minDelayBytes;
	bool sendError, ackError;
};

class equalSeqNum:public binary_function<const UdpPacketInfo * , unsigned int, bool> {
        public:
        bool operator()(const UdpPacketInfo *packet, unsigned int seqNum) const
        {
                return (packet->seqNum == seqNum);
        }
};

class lessSeqNum:public binary_function<const UdpPacketInfo *, int, bool> {
        public:
        bool operator()(const UdpPacketInfo *packet,unsigned int seqNum) const
        {
                return (packet->seqNum < seqNum);
        }
};


#endif
