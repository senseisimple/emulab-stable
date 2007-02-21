/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef _UDP_SERVER_H_PELAB
#define _UDP_SERVER_H_PELAB

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pcap.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>

const int ackQueueSize = 125;
const int minNoOfAcks = 3;

struct udpAck{
        unsigned short int seqNo;
        unsigned short int packetSize;
        unsigned long long senderTimestamp;
        unsigned long long ackTime;
};

struct ClientAddress
{
        unsigned int ipAddress;
        unsigned short portNumber;

        bool operator==(ClientAddress & secondAddress)
        {                 return ( ipAddress = secondAddress.ipAddress) && (portNumber == secondAddress.portNumber);
        }

};

// Information about a particular client, last seen sequence number
// last seen ACKs etc.
struct ClientInfo{

	ClientInfo()
		:queueStartPtr(-1),
		queueEndPtr(-1),
		curSeqNum(0),
		packetLoss(0)
	{

	}

	int queueStartPtr;
	int queueEndPtr;
	struct udpAck ackQueue[ackQueueSize];
	unsigned int curSeqNum;
	int packetLoss;
	unsigned long long clientEpoch;
};

struct CompareAddresses
{
         bool operator()(const ClientAddress & firstAddress, const ClientAddress & secondAddress)
        {
                if(firstAddress.ipAddress != secondAddress.ipAddress)
                        return(firstAddress.ipAddress < secondAddress.ipAddress);
                else if (firstAddress.portNumber != secondAddress.portNumber)
                        return(firstAddress.portNumber < secondAddress.portNumber);
                else
                        return false;

        }

};

namespace globalConsts {

        const short int USHORT_INT_SIZE = sizeof(unsigned short int);
        const short int ULONG_LONG_SIZE = sizeof(unsigned long long);
        const short int UCHAR_SIZE = sizeof(unsigned char);
}

// Function definitions.
void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);

void handleUDP_Version_1(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);

void handleUDP_Version_2(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);



#endif
