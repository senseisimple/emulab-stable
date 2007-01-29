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
	vector< UdpPacketInfo > currentAckedPackets;

	// Indicates the number of packets lost -
	// updated whenever an ACK is received.
	int packetLoss;

	// This is the total number of packets lost till now for the connection.
	int totalPacketLoss;

	// Did we drop any packets in libpcap ?
	// This number only indicates the number of sent packets that
	// were dropped in pcap buffer - based on the differences between
	// the sequence numbers seen.
	int libpcapSendLoss;

	unsigned long long minDelay;
	unsigned long long maxDelay;

	bool ackError, isAckFake;

	UdpState()
		:packetLoss(0),
		totalPacketLoss(0),
		libpcapSendLoss(0)
	{

	}

	void reset()
	{
		packetLoss = 0;
		totalPacketLoss = 0;
		libpcapSendLoss = 0;

		ackError = false;
		isAckFake = false;

	}

	~UdpState()
	{
		// Remove any packets stored in the vector.
		currentAckedPackets.clear();
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
