#include <iostream>
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
#include <fcntl.h>


#define LOCAL_SERVER_PORT 19835
#define MAX_MSG 1024

pcap_t *pcapDescriptor = NULL;
long receivedPackets = 0;
long processedPackets = 0;

char appAck[60];
unsigned long long milliSec = 0;

int sd, rc, n, flags;
socklen_t cliLen;
struct sockaddr_in cliAddr, servAddr;

using namespace std;

unsigned long long getTimeMilli()
{
        struct timeval tp;
        gettimeofday(&tp, NULL);

        long long tmpSecVal = tp.tv_sec;
        long long tmpUsecVal = tp.tv_usec;

        return (tmpSecVal*1000 + tmpUsecVal/1000);
}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{

    u_char *dataPtr = udpPacketStart + 8;
    unsigned char packetType = *(unsigned char *)(dataPtr);
    struct sockaddr_in returnAddr;

    processedPackets++;
    printf("Received packets = %d, processed = %d\n", receivedPackets, processedPackets);

    if(packetType == '0')// This is a udp data packet arriving here. Send an
        // application level acknowledgement packet for it.
    {
        unsigned long long secVal = pcap_info->ts.tv_sec;
        unsigned long long usecVal = pcap_info->ts.tv_usec;
        unsigned long long recvTimestamp = secVal*1000 + usecVal/1000;
        unsigned long long sendTimestamp = *(unsigned long long *)(dataPtr + sizeof(short int) + 1);
        long long oneWayDelay = recvTimestamp - sendTimestamp;

        returnAddr.sin_family = AF_INET;
        memcpy((char *) &returnAddr.sin_addr, (char *)&(ipPacket->ip_src), sizeof(struct in_addr));
        returnAddr.sin_port = udpHdr->source;

                appAck[0] = '1';
        memcpy(&appAck[1] ,(dataPtr + 1), sizeof(short int));
                memcpy(&appAck[1 + sizeof(short int)], &sendTimestamp, sizeof(unsigned long long));
                memcpy(&appAck[1 + + sizeof(short int) + sizeof(unsigned long long)], &oneWayDelay, sizeof(long long));
        cout << "Sending app level ack to "<< inet_ntoa(returnAddr.sin_addr) << ",at " << sendTimestamp << " , recvtimestamp = "<<recvTimestamp<< ", delay = "<< oneWayDelay << endl;

                sendto(sd,appAck,1 + + sizeof(short int) + 2*sizeof(long long),flags,(struct sockaddr *)&returnAddr,cliLen);
        }
        else if(packetType == '1') // TODO:This is an udp ACK packet. If it is being sent
            // out from this host, do nothing.
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


void init_pcap( char *ipAddress, char * iface)
{
  char * interface = iface;
/*        char interface[] = "any";*/
        struct bpf_program bpfProg;
        char errBuf[PCAP_ERRBUF_SIZE];
        char filter[128] = " udp ";

        // IP Address and sub net mask.
        bpf_u_int32 maskp, netp;
        struct in_addr localAddress;

        pcap_lookupnet(interface, &netp, &maskp, errBuf);
        pcapDescriptor = pcap_open_live(interface, BUFSIZ, 0, 0, errBuf);
        localAddress.s_addr = netp;
        printf("IP addr = %s\n", ipAddress);
        sprintf(filter," udp and dst host %s and dst port 19835 ", ipAddress);

        if(pcapDescriptor == NULL)
        {
                printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
                exit(1);
        }

        pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
        pcap_setfilter(pcapDescriptor, &bpfProg);
        pcap_setnonblock(pcapDescriptor, 1, errBuf);
}


int main(int argc, char *argv[]) {

  char msg[MAX_MSG];
  struct hostent *localHost;

  /* socket creation */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }



  localHost = gethostbyname(argv[1]);
  if(localHost == NULL)
  {
      printf("ERROR: Unknown host %s\n", argv[1]);
      exit(1);
  }

  /* bind local server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_family = localHost->h_addrtype;
  memcpy((char *) &servAddr.sin_addr.s_addr,
          localHost->h_addr_list[0], localHost->h_length);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));
  if(rc<0) {
    printf("%s: cannot bind port number %d \n",
           argv[0], LOCAL_SERVER_PORT);
    exit(1);
  }

  printf("%s: waiting for data on port UDP %u\n",
          argv[0],LOCAL_SERVER_PORT);


  init_pcap(inet_ntoa(servAddr.sin_addr), argv[2]);
  int pcapfd = pcap_get_selectable_fd(pcapDescriptor);

  flags = 0;
  fcntl(sd, F_SETFL, flags | O_NONBLOCK);

  fd_set socketReadSet;
  /* server infinite loop */
  while(1) {

      FD_ZERO(&socketReadSet);
      FD_SET(sd,&socketReadSet);
      FD_SET(pcapfd,&socketReadSet);

      struct timeval timeoutStruct;

      timeoutStruct.tv_sec = 2;
      timeoutStruct.tv_usec = 0;

      select(sd+pcapfd+1,&socketReadSet,0,0,&timeoutStruct);

      if (FD_ISSET(sd,&socketReadSet) != 0)
      {
          while(true)
          {
              memset(msg,0x0,MAX_MSG);
              /* receive message */
              cliLen = sizeof(cliAddr);
              if( recvfrom(sd, msg, MAX_MSG, flags,(struct sockaddr *) &cliAddr, &cliLen) == -1)
                  break;
              else
              {
                receivedPackets++;
              }

          }
      }

      if (FD_ISSET(pcapfd,&socketReadSet) )
          pcap_dispatch(pcapDescriptor, 1000, pcapCallback, NULL);

      if(n<0) {
          printf("%s: cannot receive data \n",argv[0]);
          continue;
      }
  }

  return 0;

}

