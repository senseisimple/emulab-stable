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
#include <sys/select.h> 
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

#define REMOTE_SERVER_PORT 1500
#define MAX_MSG 1524
#define SNAPLEN 96

#include "UdpThroughputSensor.h"
#include "UdpMinDelaySensor.h"
#include "UdpMaxDelaySensor.h"
#include "UdpPacketSensor.h"
#include "UdpState.h"

pcap_t *pcapDescriptor = NULL;
UdpThroughputSensor *throughputSensor;
UdpMaxDelaySensor *maxDelaySensor;
UdpMinDelaySensor *minDelaySensor;
UdpPacketSensor *packetSensor;

struct UdpState globalUdpState;

unsigned long long getTimeMilli()
{
        struct timeval tp;
        gettimeofday(&tp, NULL);

        unsigned long long tmpSecVal = tp.tv_sec;
        unsigned long long tmpUsecVal = tp.tv_usec;

        return (tmpSecVal*1000*1000 + tmpUsecVal);
}

void handleUDPMsg(struct sockaddr_in *clientAddr, char *udpMessage, int messageLen, struct timeval *timeStampVal)
{
	/*
	printf("Destination IP address = %s\n", inet_ntoa(ipPacket->ip_dst));
	printf("Source port = %d\n", ntohs(udpHdr->source));
	printf("Dest port = %d\n\n", ntohs(udpHdr->dest));
	*/

	//printf("Data being received = %c, %u, %lld, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1), *(unsigned long long*)(dataPtr + 5), udpLen);

	int numHistoryAcks = (messageLen - 13)/8;

        unsigned char packetType = udpMessage[0];
        unsigned long long milliSec = 0;
        int ackLength = 0;
	unsigned long long timeStamp = 0;
	unsigned long long tmpSecVal = timeStampVal->tv_sec;
	unsigned long long tmpUsecVal = timeStampVal->tv_usec;

	timeStamp =  (tmpSecVal*1000*1000 + tmpUsecVal);

        if(packetType == '0')// This is a udp data packet arriving here. Send an
            // application level acknowledgement packet for it.

            // TODO:The packet can also be leaving from this host - our libpcap filter
           //  ignores those for now - as such, nothing needs to be done for such packets.
        {
		/*
		if(strcmp( inet_ntoa(ipPacket->ip_dst),"10.1.3.2" ) != 0 )
		{
		    packetSensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen - 8, timeStamp);
		    cout <<"Sending packets\n\n\n";
		}
		*/
	}
	else if(packetType == '1')
	{
		// We received an ACK, pass it on to the sensors.
		// TODO: Ignore the ACKs being sent out from this host.

		// TODO: For now, we are just passing the packet data part.
		// For this to work correctly when integrated with magent,
		// we also need to pass the local port, remote port and
		// remote IP address, so that the connection can be looked up.

		//Pass the received packet to udp sensors:
		packetSensor->capturePacket(udpMessage, messageLen, timeStamp);
		minDelaySensor->capturePacket(udpMessage, messageLen, timeStamp);
		maxDelaySensor->capturePacket(udpMessage, messageLen,timeStamp);
		throughputSensor->capturePacket(udpMessage, messageLen, timeStamp);
	}
	else
	{
		printf("ERROR: Unknown UDP packet received from remote agent\n");
		return;
	}
}


void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
	/*
	printf("Destination IP address = %s\n", inet_ntoa(ipPacket->ip_dst));
	printf("Source port = %d\n", ntohs(udpHdr->source));
	printf("Dest port = %d\n\n", ntohs(udpHdr->dest));
	*/

	u_char *dataPtr = udpPacketStart + 8;
	unsigned short udpLen = ntohs(udpHdr->len);

	//printf("Data being received = %c, %u, %lld, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1), *(unsigned long long*)(dataPtr + 5), udpLen);

	int numHistoryAcks = (udpLen - 13 - 8)/8;

        unsigned char packetType = *(unsigned char *)(dataPtr);
        unsigned long long milliSec = 0;
        int ackLength = 0;
	unsigned long long timeStamp = 0;
	unsigned long long tmpSecVal = pcap_info->ts.tv_sec;
	unsigned long long tmpUsecVal = pcap_info->ts.tv_usec;

	timeStamp =  (tmpSecVal*1000*1000 + tmpUsecVal);

        if(packetType == '0')// This is a udp data packet arriving here. Send an
            // application level acknowledgement packet for it.

            // TODO:The packet can also be leaving from this host - our libpcap filter
           //  ignores those for now - as such, nothing needs to be done for such packets.
        {
		/*
		if(strcmp( inet_ntoa(ipPacket->ip_dst),"10.1.3.2" ) != 0 )
		{
		    packetSensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen - 8, timeStamp);
		}
		*/
	}
	else if(packetType == '1')
	{
		// We received an ACK, pass it on to the sensors.
		// TODO: Ignore the ACKs being sent out from this host.

		// TODO: For now, we are just passing the packet data part.
		// For this to work correctly when integrated with magent,
		// we also need to pass the local port, remote port and
		// remote IP address, so that the connection can be looked up.

		// Pass the captured packet to the udp sensors.
		packetSensor->capturePacket(reinterpret_cast<char *> (dataPtr), udpLen - 8, timeStamp);
		minDelaySensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen - 8, timeStamp);
		maxDelaySensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen - 8, timeStamp);
		throughputSensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen - 8, timeStamp);
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
	u_char *udpPacketStart = (u_char *)(pkt_data + sizeof(struct ether_header) + ipHeaderLength*4); 

	struct udphdr const *udpPacket;

	udpPacket = (struct udphdr const *)(udpPacketStart);

	bytesLeft -= ipHeaderLength*4;

	if(bytesLeft < sizeof(struct udphdr))
	{
		printf("Captured packet is too small to contain a UDP header.\n");
		return;
	}

	handleUDP(pcap_info,udpPacket,udpPacketStart, ipPacket);
}

void init_pcap(unsigned int portNumber)
{
	char interface[] = "eth1";
	struct bpf_program bpfProg;
	char errBuf[PCAP_ERRBUF_SIZE];
	char filter[128] = "";

	sprintf(filter," udp and dst 10.1.2.2 and dst port %d ", portNumber);

	// IP Address and sub net mask.
	bpf_u_int32 maskp, netp;
	struct in_addr localAddress;
       
	pcap_lookupnet(interface, &netp, &maskp, errBuf);
	pcapDescriptor = pcap_open_live(interface,SNAPLEN, 0, 0, errBuf);
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

	int sd, rc, i, n, flags, error;
	socklen_t echoLen;
	struct sockaddr_in cliAddr, remoteServAddr, echoServAddr;
	struct hostent *h;
	char msg[MAX_MSG];
	fd_set readFdSet, writeFdSet;
	int packetCount = 0;


	/* check command line args */
	if(argc<4) 
	{
		printf("usage : %s <server> <data> <packetCount>\n", argv[0]);
		exit(1);
	}

	/* get server IP address (no check if input is IP address or DNS name */
	h = gethostbyname(argv[1]);

	if(h==NULL) 
	{
		printf("%s: unknown host '%s' \n", argv[0], argv[1]);
		exit(1);
	}

	printf("%s: sending data to '%s' (IP : %s) \n", argv[0], h->h_name,
	inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));

	remoteServAddr.sin_family = h->h_addrtype;
	memcpy((char *) &remoteServAddr.sin_addr.s_addr, 
	 h->h_addr_list[0], h->h_length);
	remoteServAddr.sin_port = htons(REMOTE_SERVER_PORT);

	/* socket creation */
	sd = socket(AF_INET,SOCK_DGRAM,0);

	if(sd<0) 
	{
		printf("%s: cannot open socket \n",argv[0]);
		exit(1);
	}

	/* bind any port */
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	cliAddr.sin_port = htons(3200);

	rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));

	if(rc<0) 
	{
		printf("%s: cannot bind port\n", argv[0]);
		exit(1);
	}

	flags = 0;

	//init_pcap(htons(cliAddr.sin_port));

	std::ofstream throughputStream;

	throughputStream.open("Throughput.log", std::ios::out);

	// Initialize the sensors.
	packetSensor = new UdpPacketSensor(globalUdpState);
	throughputSensor = new UdpThroughputSensor(globalUdpState, throughputStream);
	maxDelaySensor = new UdpMaxDelaySensor(globalUdpState);
	minDelaySensor = new UdpMinDelaySensor(globalUdpState);

	char packetData[1600];

	unsigned int curSeqNum = 0;
	unsigned int packetLen = 0;
	unsigned long long sendTime;

	// Timeout for the non-blocking socket reads and writes.
	struct timeval selectTimeout;

	selectTimeout.tv_sec = 0;
	selectTimeout.tv_usec = 5;

	FD_ZERO(&readFdSet);
	FD_ZERO(&writeFdSet);

	// Change the socket descriptor to non-blocking.
	flags = fcntl(sd, F_GETFL, 0);
//	flags = flags | O_NONBLOCK;
	int readFlags = MSG_WAITALL;

	if( fcntl(sd, F_SETFL, flags) < 0)
	{
		printf("Error setting non blocking socket flags with fcntl.\n");
		exit(1);
	}

	int timeStampOption = 1;

	// Set the socket option to receive socket buffer timestamps for recvmsg calls.
	setsockopt(sd, SOL_SOCKET, SO_TIMESTAMP, (const void *)&timeStampOption, sizeof(int));


	// Define the structure to be passed to recvmsg.
	struct msghdr messageBuf;

	messageBuf.msg_iovlen = 1;
	messageBuf.msg_iov = (struct iovec*)malloc(sizeof(struct iovec));

	// This is the buffer to store the incoming message.
	messageBuf.msg_iov[0].iov_base = (void *)&msg[0];
	messageBuf.msg_iov[0].iov_len = MAX_MSG;

	messageBuf.msg_name = &echoServAddr;
	messageBuf.msg_namelen = sizeof(echoServAddr);

	// The timestamp of the socket buffer will be stored in this buffer.
	messageBuf.msg_control = (void *)malloc(128);
	messageBuf.msg_controllen = 128;

	struct cmsghdr *chdr = NULL;

	struct timeval timeStamp;


	packetCount = atoi(argv[3]);
	unsigned long long lastSendTime = 0;

	lastSendTime = getTimeMilli();

	/* send data */
	while(true) 
	{
		if(curSeqNum < packetCount)
		{
	//		FD_SET(sd, &writeFdSet);

	//		select(1024,NULL,&writeFdSet, NULL,&selectTimeout);

	//		if(FD_ISSET(sd, &writeFdSet) != 0)
			{
//				if(getTimeMilli() - lastSendTime > 12000)
				{
					curSeqNum++;
					// Indicate that this is a data UDP packet - not an ACK.
					packetData[0] = '0';

					// Put the sequence number of the packet.
					memcpy(&packetData[1],&curSeqNum, sizeof(unsigned int));

					if(strlen(argv[2]) <= 9)
						packetLen = 9;
					else
						packetLen = strlen(argv[2]);

					packetLen = 58;

					memcpy(&packetData[1 + sizeof(unsigned int)],&packetLen, sizeof(unsigned int));
					sendTime = getTimeMilli();
					lastSendTime = sendTime;

					rc = sendto(sd, packetData, packetLen, flags, 
					(struct sockaddr *) &remoteServAddr, 
					sizeof(remoteServAddr));

					if(rc < 0)
					{
					    printf("Blocked in send = %d\n",errno);
					    exit(1);
					}
					else
					{
						//usleep(300);
					//    packetSensor->capturePacket(packetData, packetLen + 8 + 20 + 14 + 4, sendTime);
					  pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);
					}
				}
			}

		}


		FD_SET(sd, &readFdSet);

		select(1024, &readFdSet, NULL, NULL,&selectTimeout);

		if(FD_ISSET(sd, &readFdSet) != 0)
		{
			n = recvfrom(sd, msg, MAX_MSG, readFlags,
			(struct sockaddr *) &echoServAddr, &echoLen);
			if(n == -1 && errno == EWOULDBLOCK)
			{
				printf("Going to block in recvmsg\n");
				exit(1);
			}
			/* receive echoed message */

			/****************
			echoLen = sizeof(echoServAddr);

			n = recvmsg(sd, &messageBuf, readFlags);

			if(n > 0)
			{

				chdr = CMSG_FIRSTHDR(&messageBuf);

				timeStamp = *(struct timeval *)CMSG_DATA(chdr);

				handleUDPMsg(&echoServAddr, msg, n + 8 + 20 + 14 + 4, &timeStamp);

				// Try to read and process any packets captured by libpcap.
			}
			*///////////
			if(n > 0)
				pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);
		}


	}
	return 0;
}
