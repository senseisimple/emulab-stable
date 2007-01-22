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
#include "UdpSensorList.h"

pcap_t *pcapDescriptor = NULL;
UdpThroughputSensor *throughputSensor;
UdpMaxDelaySensor *maxDelaySensor;
UdpMinDelaySensor *minDelaySensor;
UdpPacketSensor *packetSensor;
char localIP[16] = "";

struct UdpState globalUdpState;

struct pcap_stat pcapStats;
int currentPcapLoss = 0;

// Use this file handle to log messages - statistics and any warnings/errors.
std::ofstream logStream;
UdpSensorList *sensorList;

/* Grab the TSC register. */
inline volatile unsigned long long RDTSC() {
   register unsigned long long TSC asm("eax");
   asm volatile (".byte 0xf, 0x31" : : : "eax", "edx");
   return TSC;
}

unsigned long long getTimeMicro()
{
        struct timeval tp;
        gettimeofday(&tp, NULL);

        unsigned long long tmpSecVal = tp.tv_sec;
        unsigned long long tmpUsecVal = tp.tv_usec;

        return (tmpSecVal*1000*1000 + tmpUsecVal);
}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, unsigned char *udpPacketStart, struct ip const *ipPacket)
{
	// Report any packets dropped in libpcap.
	pcap_stats(pcapDescriptor, &pcapStats);

	if(pcapStats.ps_drop > currentPcapLoss)
	{
		currentPcapLoss += pcapStats.ps_drop;
		logStream <<"STAT::Number of packets lost in libpcap = "<<currentPcapLoss<<endl;
	}

	// Get a pointer to the data section of the UDP packet.
	unsigned char *dataPtr = udpPacketStart + 8;
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

        if(ntohs(udpHdr->dest) == (REMOTE_SERVER_PORT))// This is a udp data packet being sent from here. 
        {
		// Pass it to the packet sensor so that the send event, time
		// of sending and packet size are recorded.
		sensorList->capturePacket(reinterpret_cast<char *> (dataPtr), udpLen, overheadLen, timeStamp, 0);
	}
	else 
	{
		// We received an app-level UDP ACK, pass it on to the sensors.
		// Ignore the ACKs being sent out from this host. We do not
		// need to record anything about them.

		// Pass the captured packet to the udp sensors.
		sensorList->capturePacket(reinterpret_cast<char *> (dataPtr), udpLen, overheadLen, timeStamp, 1);
	}
}

int getLinkLayer(struct pcap_pkthdr const *pcap_info, unsigned char const *pkt_data)
{
	unsigned int caplen = pcap_info->caplen;

	if (caplen < sizeof(struct ether_header))
	{
		logStream<<"ERROR::A captured packet was too short to contain "
		"an ethernet header"<<endl;
		return -1;
	}
	else
	{
		struct ether_header * etherPacket = (struct ether_header *) pkt_data;
		return ntohs(etherPacket->ether_type);
	}
}

void pcapCallback(unsigned char *user, struct pcap_pkthdr const *pcap_info, unsigned char const *pkt_data)
{
	int packetType = getLinkLayer(pcap_info, pkt_data);

	if(packetType != ETHERTYPE_IP)
	{
		logStream<<"ERROR::Unknown link layer type: "<<packetType<<endl;
		return;
	}

	struct ip const *ipPacket;
	size_t bytesLeft = pcap_info->caplen - sizeof(struct ether_header);

	if(bytesLeft < sizeof(struct ip))
	{
		logStream<<"ERROR::Captured packet was too short to contain an IP header"<<endl;
		return;
	}

	ipPacket = (struct ip const *)(pkt_data + sizeof(struct ether_header));
	int ipHeaderLength = ipPacket->ip_hl;
	int ipVersion = ipPacket->ip_v;


	if(ipVersion != 4)
	{
		logStream<<"ERROR::Captured IP packet is not IPV4."<<endl;
		return;
	}

	if(ipHeaderLength < 5)
	{
		logStream<<"ERROR::Captured IP packet has header less than the minimum 20 bytes.\n"<<endl;
		return;
	}

	if(ipPacket->ip_p != IPPROTO_UDP)
	{
		logStream<<"ERROR::Captured packet is not a UDP packet.\n"<<endl;
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
		logStream<<"ERROR::Captured packet is too small to contain a UDP header."<<endl;
		return;
	}

	handleUDP(pcap_info,udpPacket,udpPacketStart, ipPacket);
}

int init_pcap(char *interface)
{
	struct bpf_program bpfProg;
	char errBuf[PCAP_ERRBUF_SIZE];
	char filter[32] = "";

	sprintf(filter," udp ");
	// IP Address and sub net mask.
	bpf_u_int32 maskp, netp;
	struct in_addr localAddress;
       
	pcap_lookupnet(interface, &netp, &maskp, errBuf);
	pcapDescriptor = pcap_open_live(interface,SNAPLEN, 0, 0, errBuf);
	localAddress.s_addr = netp;

	if(pcapDescriptor == NULL)
	{
		logStream<<"ERROR::Error opening device "<<interface<<"  with libpcap = "<< errBuf<<endl;
		exit(1);
	}

	pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp); 
	pcap_setfilter(pcapDescriptor, &bpfProg);

	pcap_setnonblock(pcapDescriptor, 1, errBuf);

	return pcap_get_selectable_fd(pcapDescriptor);

}

int main(int argc, char *argv[]) 
{

	int sd, rc = 0, i, n, flags, error;
	socklen_t echoLen, cliLen;
	struct sockaddr_in cliAddr, remoteServAddr, echoServAddr;
	struct hostent *h;
	char msg[MAX_MSG];
	fd_set readFdSet, writeFdSet;
	int packetCount = 0;


	/* check command line args */
	if(argc < 7) 
	{
		printf("usage : %s <local-interface> <serverName> <packetCount> <packetSize> <SendRate-bps> <CPU-MHZ Integer>\n", argv[0]);
		exit(1);
	}
	logStream.open("stats.log", std::ios::out);

	/* get server IP address (no check if input is IP address or DNS name */
	h = gethostbyname(argv[2]);

	if(h==NULL) 
	{
		logStream<<"ERROR:: "<<argv[0]<<": unknown host"<< argv[2]<<endl;
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
		logStream<<"ERROR:: "<<argv[0]<<": cannot open socket."<<endl;
		exit(1);
	}

	// Change the socket descriptor to non-blocking.
	flags = fcntl(sd, F_GETFL, 0);
	flags = flags | O_NONBLOCK;
	int readFlags = flags;

	// Set the socket descriptor to be non-blocking.
	if( fcntl(sd, F_SETFL, flags) < 0)
	{
		logStream<<"ERROR::Error setting non blocking socket flags with fcntl."<<endl;
		exit(1);
	}

	flags = 0;

	// Initialize the sensors.
	sensorList = new UdpSensorList(logStream);
	sensorList->addSensor(UDP_PACKET_SENSOR);
	sensorList->addSensor(UDP_THROUGHPUT_SENSOR);
	sensorList->addSensor(UDP_MINDELAY_SENSOR);
	sensorList->addSensor(UDP_MAXDELAY_SENSOR);

	// Initialize the libpcap filter.
	int pcapFD = init_pcap(argv[1]);

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
	packetCount = atoi(argv[3]);
	unsigned long long lastSendTime = 0;
	unsigned long long curTime;
	unsigned short int packetLen = atoi(argv[4]);
	long sendRate = atoi(argv[5]);

	int overheadLen = 20 + 8 + 14 + 4;

	// Fill in the MHz of the CPU - should be read from /proc/cpuinfo
	long freqDivisor = atoi(argv[6]);
	//long freqDivisor = 601;

	long long timeInterval = 800000000  / sendRate;
	timeInterval *= (packetLen + overheadLen);
	timeInterval /= 100;

	//lastSendTime = getTimeMicro();
	lastSendTime = RDTSC();
	echoLen = sizeof(echoServAddr);

	/* send data */
	while(true) 
	{
		if(curSeqNum < packetCount)
		{
			// UDP sendTo blocks in Linux - we need to 
			// check if the socket is ready for writing.
			FD_SET(sd, &writeFdSet);

			// Check whether any data is available in the pcap buffer
			// for reading.
			FD_SET(pcapFD, &readFdSet);

			select(sd + pcapFD + 1,&readFdSet,&writeFdSet, NULL,&selectTimeout);

			if(FD_ISSET(pcapFD, &readFdSet) != 0)
			{
				  pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL);
			}

			if(FD_ISSET(sd, &writeFdSet) != 0)
			{

				// This is used to regulate the rate
				// at which we send data - once integrated, these
				// intervals are sent to us from the application monitor.

				// For now, take a command line argument giving the rate
				// at which UDP packets should be sent ( this rate includes
				// the overhead for UDP, IP & ethernet headers ( &ethernet checksum)

			//	curTime = getTimeMicro();
			//	if( (curTime - lastSendTime) > timeInterval)

				curTime = RDTSC();
				if( (curTime - lastSendTime) > timeInterval*freqDivisor)
				{
					logStream << "SendDeviation:TIME="<<curTime<<",Deviation="<< (curTime - lastSendTime)/freqDivisor - timeInterval<<std::endl;

//					logStream << "SendDeviation:TIME="<<curTime<<",Deviation="<< (curTime - lastSendTime) - timeInterval<<std::endl;

					curSeqNum++;
					// We want the server to be Version - 1.
					packetData[0] = 1;

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

					// Get the port number which the socket was
					// assigned, after the first the sendTo call.
					if(curSeqNum == 1)
						getsockname(sd, (sockaddr *)&cliAddr, &cliLen);

					if(rc < 0)
					{
					    logStream<<"WARNING::Blocked in send, errno = "<<errno<<endl;
					}
				}
				else
				{
					usleep(timeInterval - ( curTime - lastSendTime)/freqDivisor );
		//			usleep(timeInterval - ( curTime - lastSendTime));
				}
			}

		}

	}
	return 0;
}
