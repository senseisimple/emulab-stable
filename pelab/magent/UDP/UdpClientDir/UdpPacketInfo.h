#ifndef UDP_PACKET_INFO_PELAB_H
#define UDP_PACKET_INFO_PELAB_H

class UdpPacketInfo{
        public:
	UdpPacketInfo::UdpPacketInfo();
	UdpPacketInfo::UdpPacketInfo(unsigned short int, unsigned short int, unsigned long long);
        unsigned short int seqNum;
        unsigned short int packetSize;
        unsigned long long timeStamp;

	bool isFake;
};

#endif
