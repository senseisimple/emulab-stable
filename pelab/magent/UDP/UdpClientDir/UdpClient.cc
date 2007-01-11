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
#define MAX_MSG 1600
#define SNAPLEN 1600

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
char localIP[16] = "";

struct UdpState globalUdpState;

unsigned long long getTimeMicro()
{
        struct timeval tp;
        gettimeofday(&tp, NULL);

        unsigned long long tmpSecVal = tp.tv_sec;
        unsigned long long tmpUsecVal = tp.tv_usec;

        return (tmpSecVal*1000*1000 + tmpUsecVal);
}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{

	// Get a pointer to the data section of the UDP packet.
	u_char *dataPtr = udpPacketStart + 8;
	unsigned short udpLen = ntohs(udpHdr->len) - 8;

	//printf("Data being received = %c, %u, %lld, %u\n", *(unsigned char *)(dataPtr), *(unsigned int *)(dataPtr + 1), *(unsigned long long*)(dataPtr + 5), udpLen);


	// Indicates whether this is a data packet or an app-level ACK.
	// This is the first byte in the data section of packets sent by the magent.
        unsigned char packetType = *(unsigned char *)(dataPtr);

	unsigned long long timeStamp = 0;
	unsigned long long tmpSecVal = pcap_info->ts.tv_sec;
	unsigned long long tmpUsecVal = pcap_info->ts.tv_usec;

	// Calculate the timestamp in microseconds when this packet
	// was captured by libpcap.
	timeStamp =  (tmpSecVal*1000*1000 + tmpUsecVal);

	// Account for the size of the IP header, 8 bytes of UDP header,
	// 14 bytes of ethernet header and 4 bytes of ethernet CRC.
	// TODO: There might also be some ethernet padding if the packet
	// size is less than the min. ethernet packet size 64 bytes.
	int overheadLen = ipPacket->ip_hl*4 + 8 + 14 + 4;

        if(packetType == '0')// This is a udp data packet being sent from here. 
		// Pass it to the packet sensor so that the send event, time
		// of sending and packet size are recorded.
        {
		if(strcmp( inet_ntoa(ipPacket->ip_dst),localIP ) != 0 )
		{
		    packetSensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen, overheadLen, timeStamp);
		}
	}
	else if(packetType == '1')
	{
		// We received an app-level UDP ACK, pass it on to the sensors.
		// Ignore the ACKs being sent out from this host. We do not
		// need to record anything about them.

		// TODO: For now, we are just passing the packet data part.
		// For this to work correctly when integrated with magent,
		// we also need to pass the local port, remote port and
		// remote IP address, so that the connection can be looked up.

		if(strcmp( inet_ntoa(ipPacket->ip_dst),localIP ) == 0 )
		{
			// Pass the captured packet to the udp sensors.
			packetSensor->capturePacket(reinterpret_cast<char *> (dataPtr), udpLen, overheadLen, timeStamp);
			minDelaySensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen, overheadLen, timeStamp);
			maxDelaySensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen, overheadLen, timeStamp);
			throughputSensor->capturePacket(reinterpret_cast<char *>(dataPtr), udpLen, overheadLen, timeStamp);
		}
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

void init_pcap(char *interface, unsigned int portNumber)
{
	struct bpf_program bpfProg;
	char errBuf[PCAP_ERRBUF_SIZE];
	char filter[32] = "";

	sprintf(filter," udp and port %d ", portNumber);
	// IP Address and sub net mask.
	bpf_u_int32 maskp, netp;
	struct in_addr localAddress;
       
	pcap_lookupnet(interface, &netp, &maskp, errBuf);
	pcapDescriptor = pcap_open_live(interface,SNAPLEN, 0, 0, errBuf);
	localAddress.s_addr = netp;

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
	if(argc < 7) 
	{
		printf("usage : %s <local-interface> <local-IP> <serverName> <packetCount> <packetSize> <SendRate-bps>\n", argv[0]);
		exit(1);
	}
	strcpy(localIP, argv[2]);

	/* get server IP address (no check if input is IP address or DNS name */
	h = gethostbyname(argv[3]);

	if(h==NULL) 
	{
		printf("%s: unknown host '%s' \n", argv[0], argv[3]);
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

	// Change the socket descriptor to non-blocking.
	flags = fcntl(sd, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	int readFlags = flags;

	// Set the socket descriptor to be non-blocking.
	if( fcntl(sd, F_SETFL, flags) < 0)
	{
		printf("Error setting non blocking socket flags with fcntl.\n");
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

	// Initialize the libpcap filter.
	init_pcap(argv[1], htons(cliAddr.sin_port));

	// Open a file and pass the handle to the throughput sensor.
	std::ofstream logStream;

	logStream.open("stats.log", std::ios::out);

	// Initialize the sensors.
	packetSensor = new UdpPacketSensor(globalUdpState);
	throughputSensor = new UdpThroughputSensor(globalUdpState, logStream);
	maxDelaySensor = new UdpMaxDelaySensor(globalUdpState, logStream);
	minDelaySensor = new UdpMinDelaySensor(globalUdpState, logStream);

	char packetData[1600];
	unsigned short int curSeqNum = 0;

	// Timeout for the non-blocking socket reads and writes.
	struct timeval selectTimeout;

	selectTimeout.tv_sec = 0;
	selectTimeout.tv_usec = 5;

	FD_ZERO(&readFdSet);
	FD_ZERO(&writeFdSet);


	// Read the command line arguments.
	// Number of packets, Size of the packets to be sent, and their sending rate.
	packetCount = atoi(argv[4]);
	unsigned long long lastSendTime = 0;
	unsigned long long curTime;
	unsigned short int packetLen = atoi(argv[5]);
	long sendRate = atoi(argv[6]);

	int overheadLen = 20 + 8 + 14 + 4;

	long long timeInterval = 800000000  / sendRate;
	timeInterval *= (packetLen + overheadLen);
	timeInterval /= 100;

	lastSendTime = getTimeMicro();
	echoLen = sizeof(echoServAddr);

	FILE *sendDevFile = fopen("SendDeviation.log", "w");

	/* send data */
	while(true) 
	{
		if(curSeqNum < packetCount)
		{
			// UDP sends do not block - we don't need to 
			// check if the socket is ready for writing.
			FD_SET(sd, &writeFdSet);

			select(sd + 1,NULL,&writeFdSet, NULL,&selectTimeout);

			if(FD_ISSET(sd, &writeFdSet) != 0)
			{

				// This is used to regulate the rate
				// at which we send data - once integrated, these
				// intervals are sent to us from the application monitor.

				// For now, take a command line argument giving the rate
				// at which UDP packets should be sent ( this rate includes
				// the overhead for UDP, IP & ethernet headers ( &ethernet checksum)
				curTime = getTimeMicro();
				if(curTime - lastSendTime > timeInterval)
				{
					logStream << "SendDeviation:TIME="<<curTime<<",Deviation="<< curTime - lastSendTime - timeInterval<<std::endl;

					curSeqNum++;
					// Indicate that this is a data UDP packet - not an ACK.
					packetData[0] = '0';

					// Put the sequence number of the packet.
					memcpy(&packetData[1],&curSeqNum, globalConsts::USHORT_INT_SIZE);

					// Copy the size of the packet.. This can be 
					// by the sensors in case they miss this packet
					// because of libpcap buffer overflow.
					// The size of the packet & its timestamp at the 
					// sender are echoed in the ACKs.

					memcpy(&packetData[1 + globalConsts::USHORT_INT_SIZE],&packetLen, globalConsts::USHORT_INT_SIZE);

					// Copy the timestamp of when this packet is being sent.

					// This timestamp will not be as accurate as 
					// the one captured by libpcap, but can be
					// used as a fallback option in case we miss
					// this packet because of a libpcap buffer overflow.
					memcpy(&packetData[1 + 2*globalConsts::USHORT_INT_SIZE], &curTime, globalConsts::ULONG_LONG_SIZE);

					lastSendTime = curTime;

					rc = sendto(sd, packetData, packetLen, flags, 
					(struct sockaddr *) &remoteServAddr, 
					sizeof(remoteServAddr));

					if(rc < 0)
					{
					    printf("WARNING:Blocked in send = %d\n",errno);
					}
					else
					{
					  pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);
					}
				}
			}

		}

		// Check whether any data is available in the receive buffer
		// for reading.
		FD_SET(sd, &readFdSet);

		select(sd + 1, &readFdSet, NULL, NULL,&selectTimeout);

		if(FD_ISSET(sd, &readFdSet) != 0)
		{
			n = recvfrom(sd, msg, MAX_MSG, 0,
			(struct sockaddr *) &echoServAddr, &echoLen);

			if(n > 0)
				pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);
		}


	}
	fclose(sendDevFile);
	return 0;
}
