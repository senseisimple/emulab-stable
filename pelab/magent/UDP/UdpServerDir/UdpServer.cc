/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "UdpServer.h"

#define MAX_MSG 1524
#define SNAPLEN 100

// Libpcap file descriptor.
pcap_t *pcapDescriptor = NULL;

// Buffer to create the application ACKs.
char appAck[1600];

int sd, flags;
socklen_t cliLen;
struct sockaddr_in cliAddr;

// Information about received packets ( packet size & receive time) is
// written into this file.
std::ofstream outFile;

// Port number to bind to - should be passed in from the command line.
int localServerPort;

// Hold the mapping from a client connection to the data about the connection,
// current sequence number, redundant ACKs, and packet loss of the connection.
std::map<struct ClientAddress, ClientInfo, CompareAddresses> ClientMap;

bool useMinAcksFlag = true;
struct pcap_stat pcapStats;
long currentPcapLoss = 0;

// Convert the argument to microseconds.
unsigned long long getPcapTimeMicro(const struct timeval *tp)
{
	unsigned long long tmpSecVal = tp->tv_sec;
	unsigned long long tmpUsecVal = tp->tv_usec;

	return (tmpSecVal*1000*1000 + tmpUsecVal);
}

// Handle a packet captured by libpcap.
void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
	// Get a pointer to the start of the data portion of the packet.
        u_char *dataPtr = udpPacketStart + 8;
	unsigned char requiredVersion = *(unsigned char *)(dataPtr);

	switch(static_cast<int>(requiredVersion))
	{
		case 1:
			handleUDP_Version_1(pcap_info, udpHdr, udpPacketStart+1, ipPacket);

			break;

		case 2:
			handleUDP_Version_2(pcap_info, udpHdr, udpPacketStart+1, ipPacket);
			break;

		default:
			fprintf(stderr, "ERROR: Received UDP packet has unknown format\n");
			break;

	}

}

// Version-1 sends ACKs to packets received from the standalone UdpClient code.
void handleUDP_Version_1(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
	// Look the up the connection to this client.
	ClientAddress clientAddr;
	clientAddr.portNumber = ntohs(udpHdr->source);
	clientAddr.ipAddress = ntohl(ipPacket->ip_src.s_addr);

	std::map<struct ClientAddress, ClientInfo, CompareAddresses>::iterator clientIter;

	clientIter = ClientMap.find(clientAddr);

	// Add a new client.
	if(clientIter == ClientMap.end())
	{
		ClientInfo clientInfo;

		
		ClientMap[clientAddr] = clientInfo;
		clientIter = ClientMap.find(clientAddr);
	}

	// Get a pointer to the start of the data portion of the packet.
        u_char *dataPtr = udpPacketStart + 8;

	// Calculate the size of the data portion of this UDP packet.
	// Subtract eight bytes to account for the UDP header.
        unsigned short udpLen = ntohs(udpHdr->len) - 8;

	unsigned short int sourcePort = ntohs(udpHdr->source);

	unsigned long long milliSec = 0;

	// This is a udp data packet arriving here. Send an
	    // application level acknowledgement packet for it.

		unsigned short int packetSeqNum = *(unsigned short int *)(dataPtr) ;

		// Calculate the header overhead, IP, 8 bytes for UDP, 14 + 4 for ethernet
		int overhead = ipPacket->ip_hl*4 + 8 + 14 + 4;
		unsigned short int recvPacketLen = *(unsigned short int *)(dataPtr + globalConsts::USHORT_INT_SIZE) + overhead;

		unsigned long long senderTimestamp = *(unsigned long long *)(dataPtr + 2*globalConsts::USHORT_INT_SIZE );

		// This holds the length of ACK packet we are going to send out.
		int ackLength = 0;

	    // If this sequence number is greater than the last sequence number
	    // we saw, then send an acknowledgement for it. Otherwise, ignore the
	    // packet - it arrived out of order.

	    // TODO:Take wrap around into account.
	    {
		    if(packetSeqNum > (clientIter->second.curSeqNum + 1))
		    {
		//	    std::cout<<"Packet being ACKed = "<<packetSeqNum<<std::endl;
			    clientIter->second.packetLoss += (packetSeqNum - clientIter->second.curSeqNum - 1);
		//	    std::cout<<"Forward packet loss = "<<packetLoss<<std::endl;
		//	    for(int k = 1;k < packetSeqNum - curSeqNum; k++)
		//		    std::cout<<"Lost packet seqNum = "<<curSeqNum + k<<"\n"<<std::endl;

		//	    std::cout<<std::endl;
		    }

		// Print the sequence number being ACKed.
		memcpy(&appAck[1], &packetSeqNum, globalConsts::USHORT_INT_SIZE);

		// Print the size of the received packet.
		memcpy(&appAck[1 + globalConsts::USHORT_INT_SIZE], &recvPacketLen, globalConsts::USHORT_INT_SIZE);

		// Get the timestamp of when this packet was received by libpcap.
		milliSec = getPcapTimeMicro(&pcap_info->ts);
		memcpy(&appAck[1 + 2*globalConsts::USHORT_INT_SIZE], &milliSec, globalConsts::ULONG_LONG_SIZE);

		// Echo the sender timestamp received in the original packet.
		memcpy(&appAck[1 + 2*globalConsts::USHORT_INT_SIZE + globalConsts::ULONG_LONG_SIZE], &senderTimestamp, globalConsts::ULONG_LONG_SIZE);

		outFile << "TIME="<<milliSec<<",SIZE="<<udpLen + overhead<<std::endl;

		// Include the sequence numbers, and ACK times of at least the last
		// seen three packets - more than 3 if the packet size allows.

		// This condition holds only for the first ACK - means that
		// there weren't any packets ACKed before this.
		int numAcks;
		unsigned long long timeDiff = 0;

		// Size of each redundant ACK - 2 bytes for sequence number + 2 bytes
	       // for the packet size +	8 bytes for receiver timestamp 
		int ackSize = 2*globalConsts::USHORT_INT_SIZE +  globalConsts::ULONG_LONG_SIZE;

		if(clientIter->second.queueStartPtr == -1 && clientIter->second.queueEndPtr == -1)
		{
		    // Do nothing.
		    numAcks = 0;
		}
		else if(clientIter->second.queueStartPtr <= clientIter->second.queueEndPtr)
			numAcks = clientIter->second.queueEndPtr - clientIter->second.queueStartPtr + 1;
		else
			numAcks = ackQueueSize;

		int minimumAcks = minNoOfAcks; // Minimum is currently 3.

		// This is the packet size that is needed to hold the minimum
		// number of redundant ACKs + sequence number being ACKed + other data.
		ackLength = minimumAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 1;

		// Check to see if the original packet size can accommodate more
		// than 3 redundant ACKs.
		int spaceLeft = udpLen - ackLength;

		// Accommodate as many redundant ACKs as possible.
		if(spaceLeft > ackSize)
			minimumAcks = minimumAcks + spaceLeft / ackSize;

		// We have more ACKs in store than we can send,
		// send only so many ACKs that will fit into the
		// packet being sent.
		if(numAcks > minimumAcks)
			numAcks = minimumAcks;

		// Print in the first byte of the ACK how many redundant
		// ACKs it is carrying.( minimum 3, maximum 121 )
		memcpy(&appAck[0], &numAcks, globalConsts::UCHAR_SIZE);

		// Calculate the size of the data portion to accommodate the
		// ACK + redundant ACKs.
		// Note: Some bytes ( < 12 ) might not be counted in this number
		// because our redundant ACK size is 12. These extra bytes
		// will be accounted for by the step below which makes the
		// packet being sent the same size as the received packet.
		ackLength = numAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 1;
		// The ACK packet must be the same size as the original UDP
		// packet that was received - this is needed so that the 
		// one way delay can be calculated as RTT/2.
		if( (udpLen) > ackLength)
			ackLength = udpLen;

		// This indicates where the redundant ACKs start in the packet.
		int redunAckStart = 1 + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE;

		// Copy the redundant ACKs.
		for(int i = 0;i < numAcks; i++)
		{
			// Copy the seq. number this redun ACK is acking.
		    memcpy(&appAck[redunAckStart + i*ackSize], &clientIter->second.ackQueue[(clientIter->second.queueStartPtr + i)%ackQueueSize].seqNo, globalConsts::USHORT_INT_SIZE);

		    // Copy the size of the packet being acked.
		    memcpy(&appAck[redunAckStart + i*ackSize + globalConsts::USHORT_INT_SIZE], &clientIter->second.ackQueue[(clientIter->second.queueStartPtr + i)%ackQueueSize].packetSize, globalConsts::USHORT_INT_SIZE);

		    timeDiff = milliSec - clientIter->second.ackQueue[(clientIter->second.queueStartPtr + i)%ackQueueSize].ackTime;

		    // Copy the time diffrence between when this packet was received
		    // and when the latest packet being ACKed was received here.
		    memcpy(&appAck[redunAckStart + i*ackSize + 2*globalConsts::USHORT_INT_SIZE], &timeDiff, globalConsts::ULONG_LONG_SIZE);
		}

		// Always maintain the sequence numbers and ack send times
		// of the last ackQueueSize(121) ACK packets.
		clientIter->second.queueEndPtr = (clientIter->second.queueEndPtr + 1)%ackQueueSize;
		if(clientIter->second.queueStartPtr != -1)
		{
			if(clientIter->second.queueStartPtr == clientIter->second.queueEndPtr)
				clientIter->second.queueStartPtr = (clientIter->second.queueStartPtr + 1)%ackQueueSize;
		}
		else
		    clientIter->second.queueStartPtr = 0;

		clientIter->second.ackQueue[clientIter->second.queueEndPtr].seqNo = packetSeqNum;
		clientIter->second.ackQueue[clientIter->second.queueEndPtr].ackTime = milliSec;
		clientIter->second.ackQueue[clientIter->second.queueEndPtr].packetSize = recvPacketLen;
		clientIter->second.ackQueue[clientIter->second.queueEndPtr].senderTimestamp = senderTimestamp;

		// Store the last sequence number seen, so that we can discard
		// UDP packets which have been re-ordered.
		clientIter->second.curSeqNum = packetSeqNum;

		cliAddr.sin_family = AF_INET;
		cliAddr.sin_addr = ipPacket->ip_src;
		cliAddr.sin_port = htons(sourcePort);
		// Send the ACK off to the host we received the data packet from.
		sendto(sd,appAck,ackLength,flags,(struct sockaddr *)&cliAddr,cliLen), errno;
	    }
	 //   else // This packet might be a re-ordered packet.
	    {

	    }
}

// Version-2 replies with ACKs to packets received from magent code.
void handleUDP_Version_2(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
	// Look the up the connection to this client.
	ClientAddress clientAddr;
	clientAddr.portNumber = ntohs(udpHdr->source);
	clientAddr.ipAddress = ntohl(ipPacket->ip_src.s_addr);

	std::map<struct ClientAddress, ClientInfo, CompareAddresses>::iterator clientIter;

	clientIter = ClientMap.find(clientAddr);

	// Add a new client.
	if(clientIter == ClientMap.end())
	{
		ClientInfo clientInfo;

		clientInfo.clientEpoch = getPcapTimeMicro(&pcap_info->ts);
		ClientMap[clientAddr] = clientInfo;
		clientIter = ClientMap.find(clientAddr);
	}

	pcap_stats(pcapDescriptor, &pcapStats);

	if(pcapStats.ps_drop > currentPcapLoss)
	{
		 currentPcapLoss = pcapStats.ps_drop;
		 printf("STAT::Number of packets lost in libpcap = %d\n",currentPcapLoss);
	}

        // Get a pointer to the start of the data portion of the packet.
        u_char *dataPtr = udpPacketStart + 8;

        // Calculate the size of the data portion of this UDP packet.
        // Subtract eight bytes to account for the UDP header.
        unsigned short udpLen = ntohs(udpHdr->len) - 8;

        unsigned short int sourcePort = ntohs(udpHdr->source);

        unsigned long long packetLibpcapTimestamp = 0;

        // Check whether the packet is the minimum size packet to hold
        // a short sequence number, a short packet size & a long long timestamp.
        if(udpLen < (2*globalConsts::USHORT_INT_SIZE + globalConsts::ULONG_LONG_SIZE))
        {
                fprintf(stderr,"ERROR: Invalid/truncated Udp packet received.\n");
                return;
        }

        //This is a udp data packet arriving here. Send an
            // application level acknowledgement packet for it.
        unsigned short int packetSeqNum = ntohs(*(unsigned short int *)(dataPtr)) ;

        // Calculate the header overhead, IP, 8 bytes for UDP, 14 + 4 for ethernet
        int overhead = ipPacket->ip_hl*4 + 8 + 14 + 4;
        unsigned short int recvPacketLen = *(unsigned short int *)(dataPtr + globalConsts::USHORT_INT_SIZE) + overhead;

        unsigned long long senderTimestamp = *(unsigned long long *)(dataPtr + 2*globalConsts::USHORT_INT_SIZE );

        // This holds the length of ACK packet we are going to send out.
        int ackLength = 0;

        // If this sequence number is greater than the last sequence number
        // we saw, then send an acknowledgement for it. Otherwise, ignore the
        // packet - it arrived out of order.

                packetLibpcapTimestamp = getPcapTimeMicro(&pcap_info->ts);
//	printf("%d - %s:%d Received seqNum=%u,size=%u\n", (packetLibpcapTimestamp - clientIter->second.clientEpoch)/1000000, inet_ntoa(ipPacket->ip_src),sourcePort , packetSeqNum,recvPacketLen);
//	printf("Received seqNum=%u,size=%u\n", packetSeqNum,recvPacketLen);
//	printf("Packet loss = %d\n", clientIter->second.packetLoss);
	if(packetSeqNum == clientIter->second.curSeqNum)
	{
		printf("This=%d is a duplicate packet\n\n",packetSeqNum);
		return;
	}
	// Ignore loopback packets.
	if(ipPacket->ip_src.s_addr == ipPacket->ip_dst.s_addr)
		return;

        // TODO:Take wrap around into account.
        {
                if(packetSeqNum > (clientIter->second.curSeqNum + 1))
                {
                        clientIter->second.packetLoss += (packetSeqNum - clientIter->second.curSeqNum - 1);
                }
               // Print the sequence number being ACKed - in the second byte.
                memcpy(&appAck[1], &packetSeqNum, globalConsts::USHORT_INT_SIZE);

                // Print the size of the received packet.
                memcpy(&appAck[1 + globalConsts::USHORT_INT_SIZE], &recvPacketLen, globalConsts::USHORT_INT_SIZE);

                // Get the timestamp of when this packet was received by libpcap.
                packetLibpcapTimestamp = getPcapTimeMicro(&pcap_info->ts);
                memcpy(&appAck[1 + 2*globalConsts::USHORT_INT_SIZE], &packetLibpcapTimestamp, globalConsts::ULONG_LONG_SIZE);

                // Echo the sender timestamp received in the original packet.
                memcpy(&appAck[1 + 2*globalConsts::USHORT_INT_SIZE + globalConsts::ULONG_LONG_SIZE], &senderTimestamp, globalConsts::ULONG_LONG_SIZE);

                outFile<<"TIME="<<packetLibpcapTimestamp<<",SIZE="<<udpLen + overhead<<std::endl;

                // Include the sequence numbers, and ACK times of at least the last
                // seen three packets - more than 3 if the packet size allows.

                // This condition holds only for the first ACK - means that
                // there weren't any packets ACKed before this.
                int numAcks;
                unsigned long long timeDiff = 0;

                // Size of each redundant ACK - 2 bytes for sequence number + 2 bytes
                // for the packet size +        8 bytes for receiver timestamp
                int ackSize = 2*globalConsts::USHORT_INT_SIZE +  globalConsts::ULONG_LONG_SIZE;

                if(clientIter->second.queueStartPtr == -1 && clientIter->second.queueEndPtr == -1)
                {
                        // Do nothing.
                        numAcks = 0;
                }
                else if(clientIter->second.queueStartPtr <=clientIter->second. queueEndPtr)
                        numAcks = clientIter->second.queueEndPtr - clientIter->second.queueStartPtr + 1;
                else
                        numAcks = ackQueueSize;

                int minimumAcks = minNoOfAcks; // Minimum is currently 3.

                // This is the packet size that is needed to hold the minimum
                // number of redundant ACKs + sequence number being ACKed + other data.
                ackLength = minimumAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 2;

                // Check to see if the original packet size can accommodate more
                // than 3 redundant ACKs.
                int spaceLeft = udpLen - ackLength;

		// Use only 3 ACKs if specified at the command line.
		if(useMinAcksFlag == true)
			spaceLeft = 0;

                // Accommodate as many redundant ACKs as possible.
                if(spaceLeft > ackSize)
                        minimumAcks = minimumAcks + spaceLeft / ackSize;
                // We have more ACKs in store than we can send,
                // send only so many ACKs that will fit into the
                // packet being sent.
                if(numAcks > minimumAcks)
                        numAcks = minimumAcks;

                // Print in the first byte of the ACK how many redundant
                // ACKs it is carrying.( minimum 3, maximum 121 )
                memcpy(&appAck[0], &numAcks, globalConsts::UCHAR_SIZE);

                // Calculate the size of the data portion to accommodate the
                // ACK + redundant ACKs.
                // Note: Some bytes ( < 12 ) might not be counted in this number
                // because our redundant ACK size is 12. These extra bytes
                // will be accounted for by the step below which makes the
                // packet being sent the same size as the received packet.
                ackLength = numAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 2;

                // The ACK packet must be the same size as the original UDP
                // packet that was received - this is needed so that the
                // one way delay can be calculated as RTT/2.
		if(useMinAcksFlag == false)
		{
			if( (udpLen) > ackLength)
				ackLength = udpLen;
		}

                // This indicates where the redundant ACKs start in the packet.
                int redunAckStart = 1 + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE;
		int index = (clientIter->second.queueEndPtr + 1 - numAcks + ackQueueSize)%ackQueueSize;
	//	printf("#Redundant Acks=%d\n", numAcks);
               // Copy the redundant ACKs.
                for(int i = 0;i < numAcks; i++)
                {
                        // Copy the seq. number this redun ACK is acking.
                        memcpy(&appAck[redunAckStart + i*ackSize], &clientIter->second.ackQueue[index].seqNo, globalConsts::USHORT_INT_SIZE);

                        // Copy the size of the packet being acked.
                        memcpy(&appAck[redunAckStart + i*ackSize + globalConsts::USHORT_INT_SIZE], &clientIter->second.ackQueue[index].packetSize, globalConsts::USHORT_INT_SIZE);

                        timeDiff = packetLibpcapTimestamp - clientIter->second.ackQueue[index].ackTime;

                        // Copy the time diffrence between when this packet was received
                        // and when the latest packet being ACKed was received here.
                        memcpy(&appAck[redunAckStart + i*ackSize + 2*globalConsts::USHORT_INT_SIZE], &timeDiff, globalConsts::ULONG_LONG_SIZE);
	//		printf("Redun Ack=%d,TimeDiff=%llu,byte=%d\n",clientIter->second.ackQueue[index].seqNo, timeDiff, redunAckStart + i*ackSize);

			index = (index + 1)%ackQueueSize;
                }
		printf("\n");

                // Always maintain the sequence numbers and ack send times
                // of the last ackQueueSize(120) ACK packets.
                clientIter->second.queueEndPtr = (clientIter->second.queueEndPtr + 1)%ackQueueSize;
                if(clientIter->second.queueStartPtr != -1)
                {
                        if(clientIter->second.queueStartPtr == clientIter->second.queueEndPtr)
                               clientIter->second.queueStartPtr = (clientIter->second.queueStartPtr + 1)%ackQueueSize;
                }
                else
                        clientIter->second.queueStartPtr = 0;

                clientIter->second.ackQueue[clientIter->second.queueEndPtr].seqNo = packetSeqNum;
                clientIter->second.ackQueue[clientIter->second.queueEndPtr].ackTime = packetLibpcapTimestamp;
                clientIter->second.ackQueue[clientIter->second.queueEndPtr].packetSize = recvPacketLen;
                clientIter->second.ackQueue[clientIter->second.queueEndPtr].senderTimestamp = senderTimestamp;

                // Store the last sequence number seen, so that we can discard
                // UDP packets which have been re-ordered.
                clientIter->second.curSeqNum = packetSeqNum;

                cliAddr.sin_family = AF_INET;
                cliAddr.sin_addr = ipPacket->ip_src;
                cliAddr.sin_port = htons(sourcePort);
                // Send the ACK off to the host we received the data packet from.
                sendto(sd,appAck,ackLength,flags,(struct sockaddr *)&cliAddr,cliLen), errno;
        }
}

int getLinkLayer(struct pcap_pkthdr const *pcap_info, const u_char *pkt_data)
{
	unsigned int caplen = pcap_info->caplen;

	if (caplen < sizeof(struct ether_header))
	{
		fprintf(stderr,"ERROR: A captured packet was too short to contain "
		       "an ethernet header\n");
		return -1;
	}
	else
	{
		struct ether_header * etherPacket = (struct ether_header *) pkt_data;
		return ntohs(etherPacket->ether_type);
	}
}

void pcapCallback(u_char *user, const struct pcap_pkthdr *pcap_info, const u_char *pkt_data)
{
        int packetType = getLinkLayer(pcap_info, pkt_data);

        if(packetType != ETHERTYPE_IP)
        {
                fprintf(stderr,"ERROR: Unknown link layer type: %d\n", packetType);
                return;
        }

        struct ip const *ipPacket;
        size_t bytesLeft = pcap_info->caplen - sizeof(struct ether_header);


        if(bytesLeft < sizeof(struct ip))
        {
                fprintf(stderr,"ERROR: Captured packet was too short to contain an IP header.\n");
                return;
        }

        ipPacket = (struct ip const *)(pkt_data + sizeof(struct ether_header));
        int ipHeaderLength = ipPacket->ip_hl;
        int ipVersion = ipPacket->ip_v;


        if(ipVersion != 4)
        {
                fprintf(stderr,"ERROR: Captured IP packet is not IPV4.\n");
                return;
        }

        if(ipHeaderLength < 5)
        {
                fprintf(stderr,"ERROR: Captured IP packet has header less than the minimum 20 bytes.\n");
                return;
        }

        if(ipPacket->ip_p != IPPROTO_UDP)
        {
                fprintf(stderr,"ERROR: Captured packet is not a UDP packet.\n");
                return;
        }

        // Ignore the IP options for now - but count their length.
        /////////////////////////////////////////////////////////
        u_char *const udpPacketStart = (u_char *const)(pkt_data + sizeof(struct ether_header) + ipHeaderLength*4);

        struct udphdr const *udpPacket;

        udpPacket = (struct udphdr const *)udpPacketStart;

        bytesLeft -= ipHeaderLength*4;

        if(bytesLeft < sizeof(struct udphdr))
        {
                printf("ERROR: Captured packet is too small to contain a UDP header.\n");
                return;
        }

        handleUDP(pcap_info,udpPacket,udpPacketStart, ipPacket);
}


void init_pcap(char *interface)
{
        struct bpf_program bpfProg;
        char errBuf[PCAP_ERRBUF_SIZE];
        char filter[32] = "";

        // IP Address and sub net mask.
        bpf_u_int32 maskp, netp;
	struct in_addr tempAddr;

        pcap_lookupnet(interface, &netp, &maskp, errBuf);

	tempAddr.s_addr = netp;
	printf("Server address = %s\n", inet_ntoa(tempAddr));
        sprintf(filter," udp and dst port %d and dst host %s", localServerPort, inet_ntoa(tempAddr));
        pcapDescriptor = pcap_open_live(interface, SNAPLEN, 0, 0, errBuf);

        if(pcapDescriptor == NULL)
        {
                fprintf(stderr,"ERROR: Error opening device %s with libpcap = %s\n", interface, errBuf);
                exit(1);
        }

        pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
        pcap_setfilter(pcapDescriptor, &bpfProg);

}


int main(int argc, char *argv[]) 
{
  
	char msg[MAX_MSG];
	struct sockaddr_in servAddr;

	if(argc < 3)
	{
		fprintf(stderr,"ERROR: Usage: ./UdpServer <interface_name> <port-num> -maxAcks\n");
		exit(1);
	}
	localServerPort = atoi(argv[2]);

	// Fit as many redundant ACKs as possible into the ACK packets.
	if(argc >= 4)
		if(strcmp(argv[3], "-maxAcks"))
			useMinAcksFlag = false;

	/* socket creation */
	sd=socket(AF_INET, SOCK_DGRAM, 0);

	if(sd<0) 
	{
		fprintf(stderr,"ERROR: Cannot open datagram socket \n");
		exit(1);
	}

	/* bind local server port */
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(localServerPort);
	int retVal = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));

	if(retVal < 0) 
	{
		fprintf(stderr,"ERROR: Cannot bind to port number %d \n",localServerPort);
		exit(1);
	}

	init_pcap(argv[1]);

	flags = fcntl(sd, F_GETFL, 0);

	cliLen = sizeof(cliAddr);

	outFile.open("ServerThroughput.log", std::ios::out);

	/* server infinite loop */
	while(true) 
		pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);


	return 0;

}

