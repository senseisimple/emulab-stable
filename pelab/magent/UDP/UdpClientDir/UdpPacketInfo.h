#ifndef UDP_PACKET_INFO_PELAB_H
#define UDP_PACKET_INFO_PELAB_H

class UdpPacketInfo{
        public:
	explicit UdpPacketInfo::UdpPacketInfo(unsigned int, unsigned int, unsigned long long, bool);
        unsigned int seqNum;
        unsigned int packetSize;
        unsigned long long timeStamp;
	bool isFastPacket;
	unsigned long long lastTimeDiff;
};

#endif
