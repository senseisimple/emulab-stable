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


#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 1524
#define SNAPLEN 1600

pcap_t *pcapDescriptor = NULL;

struct udpAck{
	unsigned short int seqNo;
	unsigned short int packetSize;
	unsigned long long senderTimestamp;
	unsigned long long ackTime;
};


char appAck[1600];
unsigned int curSeqNum = 0;
unsigned long long milliSec = 0;

int queueStartPtr = -1;
int queueEndPtr = -1;
const int ackQueueSize = 125;
const int minNoOfAcks = 3;
struct udpAck ackQueue[ackQueueSize];

int sd, rc, n, flags;
socklen_t cliLen;
struct sockaddr_in cliAddr, servAddr;
std::ofstream outFile;
int packetLoss;

void handleUDP_Version_1(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);
void handleUDP_Version_2(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);

namespace globalConsts {

	const short int USHORT_INT_SIZE = sizeof(unsigned short int);
	const short int ULONG_LONG_SIZE = sizeof(unsigned long long);
	const short int UCHAR_SIZE = sizeof(unsigned char);
}

unsigned long long getPcapTimeMicro(const struct timeval *tp)
{
	unsigned long long tmpSecVal = tp->tv_sec;
	unsigned long long tmpUsecVal = tp->tv_usec;

	return (tmpSecVal*1000*1000 + tmpUsecVal);
}
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
			break;

	}

}

void handleUDP_Version_1(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{

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
        //printf("Data being received = %c, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1));

	    // If this sequence number is greater than the last sequence number
	    // we saw, then send an acknowledgement for it. Otherwise, ignore the
	    // packet - it arrived out of order.

	    // TODO:Take wrap around into account.
//	    if(packetSeqNum > curSeqNum)
	    {
		    if(packetSeqNum > (curSeqNum + 1))
		    {
		//	    std::cout<<"Packet being ACKed = "<<packetSeqNum<<std::endl;
			    packetLoss += (packetSeqNum - curSeqNum - 1);
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

		if(queueStartPtr == -1 && queueEndPtr == -1)
		{
		    // Do nothing.
		    numAcks = 0;
		}
		else if(queueStartPtr <= queueEndPtr)
			numAcks = queueEndPtr - queueStartPtr + 1;
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
		    memcpy(&appAck[redunAckStart + i*ackSize], &ackQueue[(queueStartPtr + i)%ackQueueSize].seqNo, globalConsts::USHORT_INT_SIZE);

		    // Copy the size of the packet being acked.
		    memcpy(&appAck[redunAckStart + i*ackSize + globalConsts::USHORT_INT_SIZE], &ackQueue[(queueStartPtr + i)%ackQueueSize].packetSize, globalConsts::USHORT_INT_SIZE);

		    timeDiff = milliSec - ackQueue[(queueStartPtr + i)%ackQueueSize].ackTime;

		    // Copy the time diffrence between when this packet was received
		    // and when the latest packet being ACKed was received here.
		    memcpy(&appAck[redunAckStart + i*ackSize + 2*globalConsts::USHORT_INT_SIZE], &timeDiff, globalConsts::ULONG_LONG_SIZE);
		}

		// Always maintain the sequence numbers and ack send times
		// of the last ackQueueSize(121) ACK packets.
		queueEndPtr = (queueEndPtr + 1)%ackQueueSize;
		if(queueStartPtr != -1)
		{
			if(queueStartPtr == queueEndPtr)
				queueStartPtr = (queueStartPtr + 1)%ackQueueSize;
		}
		else
		    queueStartPtr = 0;

		ackQueue[queueEndPtr].seqNo = packetSeqNum;
		ackQueue[queueEndPtr].ackTime = milliSec;
		ackQueue[queueEndPtr].packetSize = recvPacketLen;
		ackQueue[queueEndPtr].senderTimestamp = senderTimestamp;

		// Store the last sequence number seen, so that we can discard
		// UDP packets which have been re-ordered.
		curSeqNum = packetSeqNum;

		cliAddr.sin_family = AF_INET;
		cliAddr.sin_addr = ipPacket->ip_src;
		cliAddr.sin_port = htons(sourcePort);
		// Send the ACK off to the host we received the data packet from.
		sendto(sd,appAck,ackLength,flags,(struct sockaddr *)&cliAddr,cliLen), errno;
	    }
	 //   else // This packet might be a re-ordered packet.
	    {
	//	    printf("Received reordered packet = %d\n", packetSeqNum);

	    }
}

void handleUDP_Version_2(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
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
                printf("ERROR: Invalid/truncated Udp packet received.\n");
                return;
        }

        //This is a udp data packet arriving here. Send an
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
        if(packetSeqNum > curSeqNum)
        {
                if(packetSeqNum > (curSeqNum + 1))
                {
                        packetLoss += (packetSeqNum - curSeqNum - 1);

                        //printf("Packet being ACKed = %d",packetSeqNum);
                        //printf("Forward packet loss = %d",packetLoss);
                        //for(int k = 1;k < packetSeqNum - curSeqNum; k++)
                        //          printf("Lost packet seqNum = %d\n",curSeqNum + k);

                        //   printf("\n");
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

                outFile<<"TIME="<<packetLibpcapTimestamp<<",SIZE=udpLen + overhead"<<std::endl;

                // Include the sequence numbers, and ACK times of at least the last
                // seen three packets - more than 3 if the packet size allows.

                // This condition holds only for the first ACK - means that
                // there weren't any packets ACKed before this.
                int numAcks;
                unsigned long long timeDiff = 0;

                // Size of each redundant ACK - 2 bytes for sequence number + 2 bytes
                // for the packet size +        8 bytes for receiver timestamp
                int ackSize = 2*globalConsts::USHORT_INT_SIZE +  globalConsts::ULONG_LONG_SIZE;

                if(queueStartPtr == -1 && queueEndPtr == -1)
                {
                        // Do nothing.
                        numAcks = 0;
                }
                else if(queueStartPtr <= queueEndPtr)
                        numAcks = queueEndPtr - queueStartPtr + 1;
                else
                        numAcks = ackQueueSize;

                int minimumAcks = minNoOfAcks; // Minimum is currently 3.

                // This is the packet size that is needed to hold the minimum
                // number of redundant ACKs + sequence number being ACKed + other data.
                ackLength = minimumAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 2;

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
                ackLength = numAcks*ackSize + 2*globalConsts::USHORT_INT_SIZE + 2*globalConsts::ULONG_LONG_SIZE + 2;

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
                        memcpy(&appAck[redunAckStart + i*ackSize], &ackQueue[(queueStartPtr + i)%ackQueueSize].seqNo, globalConsts::USHORT_INT_SIZE);

                        // Copy the size of the packet being acked.
                        memcpy(&appAck[redunAckStart + i*ackSize + globalConsts::USHORT_INT_SIZE], &ackQueue[(queueStartPtr + i)%ackQueueSize].packetSize, globalConsts::USHORT_INT_SIZE);

                        timeDiff = packetLibpcapTimestamp - ackQueue[(queueStartPtr + i)%ackQueueSize].ackTime;

                        // Copy the time diffrence between when this packet was received
                        // and when the latest packet being ACKed was received here.
                        memcpy(&appAck[redunAckStart + i*ackSize + 2*globalConsts::USHORT_INT_SIZE], &timeDiff, globalConsts::ULONG_LONG_SIZE);
                }

                // Always maintain the sequence numbers and ack send times
                // of the last ackQueueSize(120) ACK packets.
                queueEndPtr = (queueEndPtr + 1)%ackQueueSize;
                if(queueStartPtr != -1)
                {
                        if(queueStartPtr == queueEndPtr)
                               queueStartPtr = (queueStartPtr + 1)%ackQueueSize;
                }
                else
                        queueStartPtr = 0;

                ackQueue[queueEndPtr].seqNo = packetSeqNum;
                ackQueue[queueEndPtr].ackTime = packetLibpcapTimestamp;
                ackQueue[queueEndPtr].packetSize = recvPacketLen;
                ackQueue[queueEndPtr].senderTimestamp = senderTimestamp;

                // Store the last sequence number seen, so that we can discard
                // UDP packets which have been re-ordered.
                curSeqNum = packetSeqNum;

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
		printf("A captured packet was too short to contain "
		       "an ethernet header");
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
                printf("Unknown link layer type: %d\n", packetType);
                return;
        }

        struct ip const *ipPacket;
        size_t bytesLeft = pcap_info->caplen - sizeof(struct ether_header);


        if(bytesLeft < sizeof(struct ip))
        {
                printf("Captured packet was too short to contain an IP header.\n");
                return;
        }

        ipPacket = (struct ip const *)(pkt_data + sizeof(struct ether_header));
        int ipHeaderLength = ipPacket->ip_hl;
        int ipVersion = ipPacket->ip_v;


        if(ipVersion != 4)
        {
                printf("Captured IP packet is not IPV4.\n");
                return;
        }

        if(ipHeaderLength < 5)
        {
                printf("Captured IP packet has header less than the minimum 20 bytes.\n");
                return;
        }

        if(ipPacket->ip_p != IPPROTO_UDP)
        {
                printf("Captured packet is not a UDP packet.\n");
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
                printf("Captured packet is too small to contain a UDP header.\n");
                return;
        }

        handleUDP(pcap_info,udpPacket,udpPacketStart, ipPacket);
}


void init_pcap(char *interface)
{
        struct bpf_program bpfProg;
        char errBuf[PCAP_ERRBUF_SIZE];
        char filter[32] = "";

        sprintf(filter," udp and dst port %d", ( LOCAL_SERVER_PORT ) );
        // IP Address and sub net mask.
        bpf_u_int32 maskp, netp;
        struct in_addr localAddress;


        pcap_lookupnet(interface, &netp, &maskp, errBuf);
        pcapDescriptor = pcap_open_live(interface, SNAPLEN, 0, 0, errBuf);
        localAddress.s_addr = netp;
        printf("IP addr = %s\n", inet_ntoa(localAddress));

        if(pcapDescriptor == NULL)
        {
                printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
                exit(1);
        }

        pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
        pcap_setfilter(pcapDescriptor, &bpfProg);

	//pcap_setnonblock(pcapDescriptor, 1, errBuf);
}


int main(int argc, char *argv[]) 
{
  
	char msg[MAX_MSG];

	if(argc < 2)
	{
		printf("Usage: ./UdpServer interface_name\n");
		exit(1);
	}

	/* socket creation */
	sd=socket(AF_INET, SOCK_DGRAM, 0);

	if(sd<0) 
	{
		printf("%s: cannot open socket \n",argv[0]);
		exit(1);
	}

	/* bind local server port */
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(LOCAL_SERVER_PORT);
	rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));

	if(rc<0) 
	{
		printf("%s: cannot bind port number %d \n", 
		argv[0], LOCAL_SERVER_PORT);
		exit(1);
	}

	printf("%s: waiting for data on port UDP %u\n", 
	argv[0],LOCAL_SERVER_PORT);

	init_pcap(argv[1]);

	flags = fcntl(sd, F_GETFL, 0);

	cliLen = sizeof(cliAddr);

	outFile.open("Throughput.log", std::ios::out);

	n = 0;
	/* server infinite loop */
	while(true) 
	{

		/* receive message */
		/*
		n = recvfrom(sd, msg, MAX_MSG, flags,
		 (struct sockaddr *) &cliAddr, &cliLen);
		 */

		pcap_dispatch(pcapDescriptor, -1, pcapCallback, NULL);

		if(n<0) 
		{
			printf("%s: cannot receive data \n",argv[0]);
			continue;
		}

		/* print received message */
		/*
		printf("%s: from %s:UDP%u : %s \n", 
		argv[0],inet_ntoa(cliAddr.sin_addr),
		ntohs(cliAddr.sin_port),msg);
		*/


	}

	return 0;

}

