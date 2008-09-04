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
#include <string>
#include <vector>
#include <map>
#include <list>
#include <fcntl.h>


#define LOCAL_SERVER_PORT 9831
#define MAX_MSG 150000

pcap_t *pcapDescriptor = NULL;

char appAck[120];
unsigned long long milliSec = 0;

int sd, rc, n, flags;
socklen_t cliLen;
struct sockaddr_in cliAddr, servAddr, destAddr;

using namespace std;

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);

struct tcp_info {
    vector<unsigned long > recvdPackets;

    unsigned long lastInOrderPacket;
    list< pair<unsigned long, unsigned long> > sackRanges;

    unsigned long highSeq;
    unsigned long missingPacketTotal;
};

map<string , struct tcp_info*> connectionMap;

bool sackCompare( pair<unsigned long, unsigned long> pair1, pair<unsigned long, unsigned long> pair2)
{
    if(pair1.first <= pair2.first && pair1.second <= pair2.first)
        return true;
    else
        return false;
}

unsigned long long getTimeMilli()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);

    long long tmpSecVal = tp.tv_sec;
    long long tmpUsecVal = tp.tv_usec;

    return (tmpSecVal*1000 + tmpUsecVal/1000);
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


void init_pcap( char *ipAddress)
{
    char interface[] = "eth2";
    struct bpf_program bpfProg;
    char errBuf[PCAP_ERRBUF_SIZE];
    char filter[128] = " udp ";

    // IP Address and sub net mask.
    bpf_u_int32 maskp, netp;
    struct in_addr localAddress;

    pcap_lookupnet(interface, &netp, &maskp, errBuf);
    //pcapDescriptor = pcap_open_live(interface, BUFSIZ, 0, 0, errBuf);
    pcapDescriptor = pcap_open_live(interface, 64, 0, 0, errBuf);
    localAddress.s_addr = netp;
    printf("IP addr = %s\n", ipAddress);
    sprintf(filter," udp and dst host %s and dst port 9831 ", ipAddress);

    if(pcapDescriptor == NULL)
    {
        printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
        exit(1);
    }

    pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
    pcap_setfilter(pcapDescriptor, &bpfProg);
    pcap_setnonblock(pcapDescriptor, 1, errBuf);
}

void UpdateSACKs(unsigned long seqNum, bool consecutiveSeq , struct tcp_info *connInfo)
{
    // Update the SACK ranges.
    pair<unsigned long, unsigned long> tmpRange;
    tmpRange.first = seqNum;
    tmpRange.second = seqNum;

    if(!consecutiveSeq) // We have a confirmed hole in the received packets.
    {
        if(connInfo->sackRanges.size() == 0)
            connInfo->sackRanges.push_back(tmpRange);
        else
        {
            // Find the SACK-range that this packet fits into/extends, merging
            // two ranges if necessary.
            bool extendsBlock = false;

            // If this packet is after the right end of our last SACK block,
            // then it is indicating new packet loss.
            if(seqNum > connInfo->sackRanges.back().second)
                connInfo->missingPacketTotal += (seqNum - connInfo->sackRanges.back().second - 1);
            else
                connInfo->missingPacketTotal -= 1;
            

            list<pair<unsigned long, unsigned long> >::iterator sackIter = connInfo->sackRanges.begin();
            while(sackIter != connInfo->sackRanges.end())
            {
                // This packet is the next seqnum after the right most edge
                // in this sack block.
                if(seqNum == (*sackIter).second + 1)
                {
                    (*sackIter).second++;
                    extendsBlock = true;
                    break;
                }

                // This packet is the seqnum right before the left most edge
                // in this sack block.
                if(seqNum == (*sackIter).first - 1)
                {
                    (*sackIter).first--;
                    extendsBlock = true;
                    break;
                }

                sackIter++;
            }

            if(!extendsBlock) // Create a new SACK block.
            {

                connInfo->sackRanges.push_back(tmpRange);
                connInfo->sackRanges.sort(sackCompare);
            }
            else if(connInfo->sackRanges.size() > 1)// Merge sack blocks if possible.
            {
                list<pair<unsigned long, unsigned long> >::iterator sackIter = connInfo->sackRanges.begin();
                list<pair<unsigned long, unsigned long> >::iterator advanceIter = connInfo->sackRanges.begin();

                advanceIter = ++sackIter;
                sackIter = connInfo->sackRanges.begin();

                while(advanceIter != connInfo->sackRanges.end())
                {
                    if((*sackIter).second == ((*advanceIter).first - 1))
                    {
                        (*advanceIter).first = (*sackIter).first;
                        connInfo->sackRanges.erase(sackIter++);
                    }
                    else
                        sackIter++;

                    advanceIter++;
                }
            }
        }
    }
    else // We received a packet that is advancing the left edge of the window.
    {
        // If we don't have any outstanding SACK blocks, do nothing.
        if(connInfo->sackRanges.size() != 0)
        {
            if(connInfo->sackRanges.size() == 1)
            {
                if(connInfo->sackRanges.front().second == connInfo->lastInOrderPacket)
                {
                    // Delete this lone SACK block, all the packets have been ACKed.
                    connInfo->sackRanges.clear();

                    connInfo->missingPacketTotal = 0;
                }
            }
            else
            {
                list<pair<unsigned long, unsigned long> >::iterator sackIter = connInfo->sackRanges.begin();
                // Remove the first SACK block if possible.
                {
                    if(connInfo->lastInOrderPacket == connInfo->sackRanges.front().second)
                    {
                        // Delete this SACK block because all packets before its left edge
                        // have been now ACKed.
                        connInfo->missingPacketTotal -= 1;
                        connInfo->sackRanges.erase(sackIter);
                    }
                }
            }
        }
    }
}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{

    u_char *dataPtr = udpPacketStart + 8;
    unsigned char packetType = *(unsigned char *)(dataPtr);

    if(packetType == '0')// This is a udp data packet arriving here. Send an
        // application level acknowledgement packet for it.
    {
        unsigned long seqNum = *(unsigned long *)(dataPtr + 1);
        unsigned long ackedSeq = 0;
        struct tcp_info *connInfo;
        string src_ip_port;

        src_ip_port = inet_ntoa(ipPacket->ip_src) ; 
        src_ip_port += ":";
        src_ip_port += ntohs(udpHdr->source);

//        cout << "Received packet #"<<seqNum<<endl;

        if(connectionMap.find(src_ip_port) != connectionMap.end()) // Established connection.
        {
            // Look up the connection info.
            connInfo = connectionMap[src_ip_port];

            if(seqNum == connInfo->lastInOrderPacket+1) // Received the next packet in window
                // and ACK as many packets as possible.
            {

                if(connInfo->recvdPackets.size() != 0) // There are future seq. numbered packets
                    // in the buffer.
                {
                    unsigned long curSeq = seqNum;

                    // Remove all contiguous packets from the buffer.
                    while(connInfo->recvdPackets.front() == curSeq+1)
                    {
                        curSeq++;
                        pop_heap(connInfo->recvdPackets.begin(), connInfo->recvdPackets.end(), greater<int>());
                        connInfo->recvdPackets.pop_back();
                    }

                    connInfo->lastInOrderPacket = curSeq;
                }
                else // This packet advances the right edge of the window by one.
                    connInfo->lastInOrderPacket++;

                ackedSeq = connInfo->lastInOrderPacket+1;
                UpdateSACKs(seqNum, true, connInfo);
            }
            else if(seqNum <= connInfo->lastInOrderPacket) // Redundant packet.
                ackedSeq = connInfo->lastInOrderPacket+1;
            else
            {
                vector<unsigned long >::iterator seqNumIter = connInfo->recvdPackets.end();

                seqNumIter = find(connInfo->recvdPackets.begin(), connInfo->recvdPackets.end(), seqNum);

                if(seqNumIter == connInfo->recvdPackets.end()) // We haven't seen this packet
                    // before, put it in the received packet list.
                {
                    connInfo->recvdPackets.push_back(seqNum);
                    push_heap(connInfo->recvdPackets.begin(), connInfo->recvdPackets.end(), greater<int>());
                    ackedSeq = connInfo->lastInOrderPacket+1;
                    //printf("Initializing SACK blocks for packet %d, ack = %d\n", seqNum,ackedSeq);
                    UpdateSACKs(seqNum, false, connInfo);
                }
                else // Redundant packet, doesn't advance the window.
                    ackedSeq = connInfo->lastInOrderPacket+1;
            }

        }
        else // New connection
        {
            connInfo = new struct tcp_info;

            connInfo->recvdPackets.resize(0);
            connInfo->lastInOrderPacket = 0;
            connInfo->sackRanges.resize(0);
            connectionMap[src_ip_port] = connInfo;

            connInfo->highSeq = seqNum;
            connInfo->missingPacketTotal = 0;

            if(seqNum != connInfo->lastInOrderPacket+1) // This isn't the first packet.
            {
                connInfo->recvdPackets.push_back(seqNum);
                push_heap(connInfo->recvdPackets.begin(), connInfo->recvdPackets.end(), greater<int>());
                connInfo->lastInOrderPacket = 0;
                ackedSeq = connInfo->lastInOrderPacket + 1;

                UpdateSACKs(seqNum, false, connInfo);

            }
            else // Received the first packet.
            {
                connInfo->lastInOrderPacket = 1;
                ackedSeq = connInfo->lastInOrderPacket + 1;

            }
        }

        // Keep track of the highest sequence number received from the sender.
        if(seqNum > connInfo->highSeq)
            connInfo->highSeq = seqNum;

        // Send an ACK for the expected sequence number.
        appAck[0] = '1';
        char numSackRanges = (char)0;
        int packetSize = 2 + 3*sizeof(unsigned long);

        // Limit the maximum number of sack blocks to 5.
        if(connInfo->sackRanges.size() != 0)
        {
            if(connInfo->sackRanges.size() >= 5)
                numSackRanges = (char)5;
            else
                numSackRanges = (char)connInfo->sackRanges.size();
        }
        memcpy(&appAck[1], &numSackRanges, sizeof(char));
        memcpy(&appAck[2], &ackedSeq, sizeof(unsigned long));
        //cout<<"Sending ACK for seqNum = " << ackedSeq<<endl;

        // Send the total of missing packets.
        memcpy(&appAck[2 + sizeof(unsigned long)], &connInfo->missingPacketTotal, sizeof(unsigned long ));

        // Also the highest sequence number seen.
        memcpy(&appAck[2 + 2*sizeof(unsigned long)], &connInfo->highSeq, sizeof(unsigned long ));

        if((int)numSackRanges != 0)
        {
            printf("Creating a SACK for packet = %d ^ ", ackedSeq);
            packetSize += (2*(int)numSackRanges*sizeof(unsigned long));

            unsigned long rangeBegin, rangeEnd;

            list<pair<unsigned long, unsigned long> >::iterator sackIter = connInfo->sackRanges.begin();
            int i = 0;

            // Send the 'n' most recent SACK blocks.
            if(connInfo->sackRanges.size() != numSackRanges)
            {
                int index = connInfo->sackRanges.size() - numSackRanges;

                while(index != 0)
                {
                    sackIter++;
                    index--;
                }
            }


            while(i < (int)numSackRanges && (sackIter != connInfo->sackRanges.end()))
            {
                rangeBegin = (*sackIter).first;
                rangeEnd = (*sackIter).second;

                printf("%d %d: ", rangeBegin, rangeEnd);

                memcpy(&appAck[2 + 3*sizeof(unsigned long) + 2*i*sizeof(unsigned long)], &rangeBegin, sizeof(unsigned long ));
                memcpy(&appAck[2 + 3*sizeof(unsigned long) + (2*i+1)*sizeof(unsigned long)], &rangeEnd, sizeof(unsigned long ));

                i++;
                sackIter++;
            }
            printf("\n");

            printf("HighSeq = %u, lostNum = %u\n", connInfo->highSeq, connInfo->missingPacketTotal);
        }
        else
        {
            printf("Sending ACK for packet = %u, received = %u\n", ackedSeq,seqNum);

        }

        // Fill in the IP address & port number.
        memcpy((char *) &destAddr.sin_addr.s_addr,
                (char *) &ipPacket->ip_src.s_addr, sizeof(ipPacket->ip_src.s_addr));
        destAddr.sin_port = udpHdr->source;

        int retval = sendto(sd,appAck,packetSize,flags,(struct sockaddr *)&destAddr,sizeof(destAddr));
        if(retval < 0)
        {
            cout<<"Send failed for ACK "<<ackedSeq<<endl;
            exit(1);

        }
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

    // Prepare the structure to be used for ACKs.
    destAddr.sin_family = AF_INET;
    destAddr.sin_family = localHost->h_addrtype;


    printf("%s: waiting for data on port UDP %u\n", 
            argv[0],LOCAL_SERVER_PORT);

    flags = 0;


    init_pcap(inet_ntoa(servAddr.sin_addr));
    int pcapfd = pcap_get_selectable_fd(pcapDescriptor);
    struct pcap_stat pcapStatObj;
    fd_set socketReadSet;

    struct timeval timeoutStruct;

    timeoutStruct.tv_sec = 0;
    timeoutStruct.tv_usec = 50000;

    memset(msg,0x0,MAX_MSG);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    /* server infinite loop */
    while(1) {
        FD_ZERO(&socketReadSet);
        FD_SET(sd,&socketReadSet);
        FD_SET(pcapfd,&socketReadSet);

        select(sd+pcapfd+1,&socketReadSet,0,0,&timeoutStruct);


        if (FD_ISSET(sd,&socketReadSet) )
        {   
            /* receive message */
            cliLen = sizeof(cliAddr);

            while(true)
            {
                if(recvfrom(sd, msg, MAX_MSG, flags,(struct sockaddr *) &cliAddr, &cliLen))
                {
                    pcap_dispatch(pcapDescriptor, 300, pcapCallback, NULL);
                }
                else
                    break;
            }
        }

        if (FD_ISSET(pcapfd,&socketReadSet) )
        {           
            while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);
        }


        if(n<0) {
            printf("%s: cannot receive data \n",argv[0]);
            continue;
        }
    }
    pcap_stats(pcapDescriptor, &pcapStatObj);
    if(pcapStatObj.ps_drop > 0)
        printf("pcap: Packets received %d, dropped %d\n", pcapStatObj.ps_recv, pcapStatObj.ps_drop);

    return 0;

}

