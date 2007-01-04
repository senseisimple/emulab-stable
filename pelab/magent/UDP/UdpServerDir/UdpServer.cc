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

pcap_t *pcapDescriptor = NULL;

struct udpAck{
	int seqNo;
	unsigned long long ackTime;
};


char appAck[2000];
unsigned int curSeqNum = 0;
unsigned long long milliSec = 0;

int queueStartPtr = -1;
int queueEndPtr = -1;
const int ackQueueSize = 3;
struct udpAck ackQueue[ackQueueSize];

int sd, rc, n, flags;
socklen_t cliLen;
struct sockaddr_in cliAddr, servAddr;
unsigned long long lastAckTime = 0;

unsigned long long getPcapTimeMicro(const struct timeval *tp)
{
	unsigned long long tmpSecVal = tp->tv_sec;
	unsigned long long tmpUsecVal = tp->tv_usec;

	return (tmpSecVal*1000*1000 + tmpUsecVal);
}

// This is not used right now - only relevant if we are using SO_TIMESTAMP option instead
// of libpcap.
void handleUDPMsg(struct sockaddr_in *clientAddr, char *udpMessage, int messageLen, struct timeval *timeStamp, std::ofstream &fileHandle)
{
	/*
        printf("Destination IP address = %s\n", inet_ntoa(ipPacket->ip_dst));
        printf("Source port = %d\n", ntohs(udpHdr->source));
        printf("Dest port = %d\n\n", ntohs(udpHdr->dest));
	*/
	struct timeval timeVal;
//	gettimeofday(&timeVal, NULL);
	//std::cout << "Time before = "<<getPcapTimeMilli(&timeVal)<<std::endl;

	unsigned char packetType = udpMessage[0];
	unsigned long long milliSec = 0;
	int ackLength = 0;

	if(packetType == '0')
	{
		// This is a udp data packet arriving here. Send an
		// application level acknowledgement packet for it.

		// TODO:The packet can also be leaving from this host - our libpcap filter
		//  ignores those for now - as such, nothing needs to be done for such packets.

		unsigned int packetSeqNum = *(unsigned int *)(udpMessage + 1) ;
		//printf("Data being received = %c, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1));

		// If this sequence number is greater than the last sequence number
		// we saw then send an acknowledgement for it. Otherwise, ignore the
		// packet.

	    // TODO:Take wrap around into account.
		if(packetSeqNum > curSeqNum)
		{
			appAck[0] = '1';
			memcpy(&appAck[2], &packetSeqNum, sizeof(unsigned int));

			milliSec = getPcapTimeMicro(timeStamp);
			fileHandle << "Time="<<milliSec<<",Size="<<messageLen+20+8+14+4<<std::endl;
			if(lastAckTime == 0)
				lastAckTime = milliSec;
			else
			{
			//	std::cout << "Time of receive = "<<milliSec<<std::endl;
				lastAckTime = milliSec;
			}

			memcpy(&appAck[2 + sizeof(unsigned int)], &milliSec, sizeof(unsigned long long));

			// Include the sequence numbers, and ACK times of the last
			// seen three packets.

			// This condition holds only for the first ACK - means that
			// there weren't any packets ACKed before this.
			int numAcks;
			unsigned long timeDiff = 0;
			int ackSize = sizeof(unsigned int) + sizeof(unsigned long);

			if(queueStartPtr == -1 && queueEndPtr == -1)
			{
			    // Do nothing.
			    numAcks = 0;
			}
			else if(queueStartPtr <= queueEndPtr)
				numAcks = queueEndPtr - queueStartPtr + 1;
			else
				numAcks = 3;

			// Print a single digit number (0,1,2 or 3), indicating the
			// number of redundant ACKs being sent.
			appAck[1] = static_cast<char>(numAcks);
			ackLength = numAcks*ackSize + sizeof(unsigned int) + sizeof(unsigned long long) + 2;

			int redunAckStart = 2 + sizeof(unsigned int) + sizeof(unsigned long long);

			// The ACK packet must be the same size as the original UDP
			// packet that was received - this is needed so that the 
			// one way delay can be calculated as RTT/2.
			if( messageLen > ackLength)
				ackLength = messageLen;

			for(int i = 0;i < numAcks; i++)
			{
			    memcpy(&appAck[redunAckStart + i*ackSize], &ackQueue[(queueStartPtr + i)%ackQueueSize].seqNo, sizeof(unsigned int));

			    
			    timeDiff = milliSec - ackQueue[(queueStartPtr + i)%ackQueueSize].ackTime;
		//	    std::cout << "Timediff between redun = "<< ackQueue[ (queueStartPtr + i)%ackQueueSize].seqNo <<", and this = "<<timeDiff<<std::endl;
			    memcpy(&appAck[redunAckStart + i*ackSize + sizeof(unsigned int)], &timeDiff, sizeof(unsigned long));
			}

			// Always maintain the sequence numbers and ack send times
			// of the last three ACK packets.
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

			curSeqNum = packetSeqNum;

			sendto(sd,appAck,ackLength,flags,(struct sockaddr *)clientAddr,cliLen);
		}

	}
	else if(packetType == '1') 
	{
		// TODO:This is an udp ACK packet. If it is being sent
		// out from this host, do nothing. ( right now, this case does not
		// occur, because our libpcap filter is only accepting incoming packets).

		// If we are receiving an ACK, pass it on to the sensors.
	}
	else
	{
		printf("ERROR: Unknown UDP packet received from remote agent\n");
		return;
	}

//	gettimeofday(&timeVal, NULL);
//	std::cout << "Time After = "<<getPcapTimeMicro(&timeVal)<<std::endl;
}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{

        u_char *dataPtr = udpPacketStart + 8;
        unsigned short udpLen = ntohs(udpHdr->len);

	unsigned char packetType = *(unsigned char *)(dataPtr);
	unsigned long long milliSec = 0;
	int ackLength = 0;

	if(packetType == '0')// This is a udp data packet arriving here. Send an
	    // application level acknowledgement packet for it.

	    // TODO:The packet can also be leaving from this host - the code for such packets 
		// is in the UdpClient application ( UdpClient.cc ). Once these changes
		// are integrated into magent, then the functionality of this procedure
		// will be a combination of code below and that in UdpClient.
	{
	    unsigned int packetSeqNum = *(unsigned int *)(dataPtr + 1) ;
        //printf("Data being received = %c, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1));

	    // If this sequence number is greater than the last sequence number
	    // we saw then send an acknowledgement for it. Otherwise, ignore the
	    // packet.

	    // TODO:Take wrap around into account.
	    if(packetSeqNum > curSeqNum)
	    {
		appAck[0] = '1';
		memcpy(&appAck[2], &packetSeqNum, sizeof(unsigned int));

		milliSec = getPcapTimeMicro(&pcap_info->ts);
		if(lastAckTime == 0)
			lastAckTime = milliSec;
		else
		{
			lastAckTime = milliSec;
		}
		memcpy(&appAck[2 + sizeof(unsigned int)], &milliSec, sizeof(unsigned long long));

		// Include the sequence numbers, and ACK times of the last
		// seen three packets.

		// This condition holds only for the first ACK - means that
		// there weren't any packets ACKed before this.
		int numAcks;
		unsigned long timeDiff = 0;
		int ackSize = sizeof(unsigned int) + sizeof(unsigned long);
		if(queueStartPtr == -1 && queueEndPtr == -1)
		{
		    // Do nothing.
		    numAcks = 0;
		}
		else if(queueStartPtr <= queueEndPtr)
			numAcks = queueEndPtr - queueStartPtr + 1;
		else
			numAcks = 3;

		// Print a single digit number (0,1,2 or 3), indicating the
		// number of redundant ACKs being sent.
		appAck[1] = static_cast<char>(numAcks);
		ackLength = numAcks*ackSize + sizeof(unsigned int) + sizeof(unsigned long long) + 2;

		int redunAckStart = 2 + sizeof(unsigned int) + sizeof(unsigned long long);

		// The ACK packet must be the same size as the original UDP
		// packet that was received - this is needed so that the 
		// one way delay can be calculated as RTT/2.
		if( (udpLen - 8) > ackLength)
			ackLength = udpLen - 8;

		for(int i = 0;i < numAcks; i++)
		{
		    memcpy(&appAck[redunAckStart + i*ackSize], &ackQueue[(queueStartPtr + i)%ackQueueSize].seqNo, sizeof(unsigned int));

		    timeDiff = milliSec - ackQueue[(queueStartPtr + i)%ackQueueSize].ackTime;
		    memcpy(&appAck[redunAckStart + i*ackSize + sizeof(unsigned int)], &timeDiff, sizeof(unsigned long));
		}

		// Always maintain the sequence numbers and ack send times
		// of the last three ACK packets.
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

		curSeqNum = packetSeqNum;

		sendto(sd,appAck,ackLength,flags,(struct sockaddr *)&cliAddr,cliLen);
	    }

	}
	else if(packetType == '1') // TODO:This is an udp ACK packet. If it is being sent
	    // out from this host, do nothing. ( right now, this case does not
	    // occur, because our libpcap filter is only accepting incoming packets).

	    // If we are receiving an ACK, pass it on to the sensors.
	{

	}
      else
        {
                printf("ERROR: Unknown UDP packet received from remote agent\n");
                return;
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
        pcapDescriptor = pcap_open_live(interface, BUFSIZ, 0, 0, errBuf);
        localAddress.s_addr = netp;
        printf("IP addr = %s\n", inet_ntoa(localAddress));

        if(pcapDescriptor == NULL)
        {
                printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
                exit(1);
        }

        pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
        pcap_setfilter(pcapDescriptor, &bpfProg);

	pcap_setnonblock(pcapDescriptor, 1, errBuf);
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

	std::ofstream outFile;

	outFile.open("Throughput.log", std::ios::out);

	/* server infinite loop */
	while(true) 
	{

		/* receive message */
		n = recvfrom(sd, msg, MAX_MSG, flags,
		 (struct sockaddr *) &cliAddr, &cliLen);

		pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);

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

