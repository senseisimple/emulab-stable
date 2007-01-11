#ifndef UDP_STATE_PELAB_H
#define UDP_STATE_PELAB_H

#include "UdpLibs.h"
#include "UdpPacketInfo.h"

using namespace std;

class UdpPacketInfo;

class UdpState{
	public:
	// vector of info about packets sent from this host,
        // sequence number, timestamp & size of the packet.
	vector< UdpPacketInfo > recentSentPackets;

	// Indicates the number of packets lost ( in a batch of 4 sent packets ) 
	// updated whenever an ACK is received.
	int packetLoss;
	unsigned long long minDelay;
	unsigned long long maxDelay;

	bool ackError, isAckFake;

	~UdpState()
	{
		// Remove any packets stored in the vector.
		recentSentPackets.clear();
	}
};

class equalSeqNum:public binary_function<UdpPacketInfo , unsigned short int, bool> {
        public:
        bool operator()(const UdpPacketInfo& packet, unsigned short int seqNum) const
        {
                return (packet.seqNum == seqNum);
        }
};

class lessSeqNum:public binary_function<UdpPacketInfo , unsigned short int, bool> {
        public:
        bool operator()(const UdpPacketInfo& packet,unsigned short int seqNum) const
        {
                return (packet.seqNum < seqNum);
        }
};


#endif
