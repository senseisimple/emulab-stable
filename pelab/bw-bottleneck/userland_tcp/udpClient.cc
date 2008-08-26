#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
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
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
#include <map>
#include <sstream>
#include <math.h>

#define REMOTE_SERVER_PORT 9831
#define MAX_MSG 450000

pcap_t *pcapDescriptor = NULL;

using namespace std;
struct tcp_block{

    unsigned long lastSentSeq;
    unsigned long lastAckedSeq;
    int dup_acks;

    double congWindow;
    int ssthresh;

    double rtt_est;
    double rtt_deviation_est;
    double rto_estimate;

    bool slowStart;

};

struct tcp_block conn_info;
int clientSocket;
struct sockaddr_in remoteServAddr;
unsigned long long startTime;

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);
int xmit_packet(char *message, int messageLen);

map<unsigned long, unsigned long long> packetTimeMap;
map<unsigned long, unsigned long long> timeoutMap;
map<unsigned long, bool> rexmitMap;
long long retransTimer;
list<unsigned long> unackedPackets;
list<unsigned long> rexmitPackets;
long num_outstanding_packets;
unsigned long seq_num_sent_before_loss;
unsigned long reportedLost;
unsigned long highSeq;

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

void init_pcap( char *ipAddress)
{
    char interface[] = "eth4";
    struct bpf_program bpfProg;
    char errBuf[PCAP_ERRBUF_SIZE];
    char filter[128] = " udp ";

    // IP Address and sub net mask.
    bpf_u_int32 maskp, netp;
    struct in_addr localAddress;

    pcap_lookupnet(interface, &netp, &maskp, errBuf);
    //pcapDescriptor = pcap_open_live(interface, BUFSIZ, 0, 0, errBuf);
    pcapDescriptor = pcap_open_live(interface, 120, 0, 0, errBuf);
    localAddress.s_addr = netp;
    //printf("IP addr = %s\n", ipAddress);
    sprintf(filter," udp and ( (src host %s and dst port 9831 ) or (dst host %s and src port 9831 )) ", ipAddress, ipAddress);

    if(pcapDescriptor == NULL)
    {
        printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
        exit(1);
    }

    pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
    pcap_setfilter(pcapDescriptor, &bpfProg);
    pcap_setnonblock(pcapDescriptor, 1, errBuf);

}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
    u_char *dataPtr = udpPacketStart + 8;

    char packetType = *(char *)(dataPtr);
    long long milliSec = 0;
    int ackLength = 0;

    if(packetType == '0')
    {
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 1));
        unsigned long long currTime = getTimeMilli();

        unsigned long long secVal = pcap_info->ts.tv_sec;
        unsigned long long usecVal = pcap_info->ts.tv_usec;

        packetTimeMap[seqNum] = (unsigned long long)(secVal*1000 + usecVal/1000);

        // Set the retransmission timer.
        retransTimer = currTime + (unsigned long long)conn_info.rto_estimate;

        //cout<<"Sending seqNum "<<seqNum<<endl;

        unackedPackets.push_back(seqNum);

    }
    else if(packetType == '1')
    {
        // We received an ACK.
        int sackBlocks = (int)(*(char *)(dataPtr + 1));
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 2));
        reportedLost = ( *(unsigned long *)(dataPtr + 2 + sizeof(unsigned long)));
        highSeq = ( *(unsigned long *)(dataPtr + 2 + 2*sizeof(unsigned long)));

        // Normal ACK without any SACK ranges.
        {
            // Change the congestion window.
            if(conn_info.slowStart)
            {
                //conn_info.congWindow += (seqNum - conn_info.lastAckedSeq);
                conn_info.congWindow += 1; 

                // If the congestion window is greater than the slow-start threshold
                // disable slow start.
                if(conn_info.congWindow >= conn_info.ssthresh)
                    conn_info.slowStart = false;
            }
            else // In congestion avoidance, increase window by one packet every RTT.
            {
                double tmp = conn_info.congWindow;
                conn_info.congWindow += (1.0/conn_info.congWindow);
            }

            // Update the RTT estimates.

            // Only include non retransmitted packets in our calculations.
            if(! rexmitMap[seqNum-1] )
            {
                // Count only the packets that have been individually acked.
                // ie. only the single packet at the left most position in the
                // window. Do not take cumulatively acked packets into account.
                if(seqNum == conn_info.lastAckedSeq+1)
                {
                    unsigned long long secVal = pcap_info->ts.tv_sec;
                    unsigned long long usecVal = pcap_info->ts.tv_usec;

                    long rtt_val = (secVal*1000 + usecVal/1000 - packetTimeMap[seqNum-1]);

                    // Use an exponential moving avg.
                    conn_info.rtt_est = conn_info.rtt_est*(1-0.125) + rtt_val*0.125;
                    conn_info.rtt_deviation_est = conn_info.rtt_deviation_est*(1-0.25) + fabs(rtt_val - conn_info.rtt_est)*0.25;

                    conn_info.rto_estimate = conn_info.rtt_est + 4.0*conn_info.rtt_deviation_est;

                    // Never allow the timeout timer to be below 1 second, to avoid
                    // spurious timeouts.(RFC 2988)
                    if(conn_info.rto_estimate < 1000)
                        conn_info.rto_estimate = 1000;
                }
            }

            if(seqNum > conn_info.lastAckedSeq)
            {

                vector<unsigned long> ackedPackets;

                for(unsigned long i = conn_info.lastAckedSeq; i < seqNum; i++)
                    ackedPackets.push_back(i);

                long tmp = num_outstanding_packets;


                if(num_outstanding_packets < 0)
                {
                    //printf("Acked = %d, lastAcked = %d, prev = %d, new = %d\n", seqNum, conn_info.lastAckedSeq, tmp,num_outstanding_packets);

                }

                conn_info.lastAckedSeq = seqNum;

                if(conn_info.dup_acks > 0)
                    conn_info.dup_acks = 0;

                // Remove the ACKed packets from all the maps.
                for(int i = 0;i < ackedPackets.size(); i++)
                {
                    packetTimeMap.erase(ackedPackets[i]);
                    rexmitMap.erase(ackedPackets[i]);
                    timeoutMap.erase(ackedPackets[i]);
                    if(find(unackedPackets.begin(),unackedPackets.end(),ackedPackets[i]) != unackedPackets.end())
                    {
                        unackedPackets.erase(find(unackedPackets.begin(),unackedPackets.end(),ackedPackets[i]));
                // Decrease the outstanding window size by the ACKed number of packets.
                        num_outstanding_packets -= 1;
                    }
                }


                list<unsigned long>::iterator rexmitIter = rexmitPackets.begin();

                while(rexmitIter != rexmitPackets.end())
                {
                    if(find(ackedPackets.begin(), ackedPackets.end(),(*rexmitIter)) != ackedPackets.end())
                    {
                        rexmitIter = rexmitPackets.erase(rexmitIter);
                    }
                    else
                        rexmitIter++;
                }

                unsigned long long currTime = getTimeMilli();
                // Reset the timout value - because this ACK is acknowledging new data.
                retransTimer = currTime + (int)conn_info.rto_estimate;
            }
            else if(seqNum == conn_info.lastAckedSeq)// Count duplicate acks.
            {
                conn_info.dup_acks += 1;
                num_outstanding_packets -= 1;

                if(conn_info.dup_acks == 3)
                { // Add the packet to the retransmit queue.

                    // Indicate that this packet needs to be retransmitted once, any
                    // further retransmissions have to be caused strictly by a timeout.

                    // Add this seqNum to the list of packets to be rexmitted - if it isn't
                    // already in that list.
                    if(find(rexmitPackets.begin(),rexmitPackets.end(),seqNum) == rexmitPackets.end())
                    {
                        rexmitPackets.push_back(seqNum);

                        //This map entry is used in two ways - a packet has been rexmitted
                        // if its entry is true in this map & it had been in the rexmitPackets
                        // vector.
                        rexmitMap[seqNum] = false;
                    }

                    if(seqNum > seq_num_sent_before_loss)
                    {
                        // Halve the slow-start threshold and congestion window size.
                        conn_info.ssthresh = (conn_info.lastSentSeq - conn_info.lastAckedSeq)/2;
                        if(conn_info.ssthresh < 2)
                            conn_info.ssthresh = 2;
                        conn_info.congWindow = conn_info.ssthresh;
                        conn_info.slowStart = false;

                        seq_num_sent_before_loss = conn_info.lastSentSeq - 1;
                    }
                    else // Ignore multiple losses in a single window.
                    {

                    }
                }
            }
        }

        // Indicates some missing packets at the receiver - add them to rexmit queue.
        if(sackBlocks != 0)
        {
            unsigned long lastAckedPacket = seqNum - 1;
            int sackStart = 2 + 3*sizeof(unsigned long);
            unsigned long long currTime = getTimeMilli();

            //printf("Received a SACK for %d with %d blocks\n", seqNum, sackBlocks);

            for(int i = 0;i < sackBlocks; i++)
            {
                unsigned long rangeStart = ( *(unsigned long *)(dataPtr + sackStart + i*2*sizeof(long)));
                unsigned long rangeEnd = ( *(unsigned long *)(dataPtr + sackStart + i*2*sizeof(long) + sizeof(long) ));

                //printf("%d %d :", rangeStart,rangeEnd);


                // This is always true, if the SACKs were correctly generated by the receiver.

                bool packetLossFlag = false;
                unsigned long lostPacketSeq = 0;

                if(rangeStart > lastAckedPacket)
                {
                    // All these packets were lost - add them to rexmit queue.
                    for(unsigned long j = lastAckedPacket + 1; j < rangeStart; j++)
                    {
                        if(find(rexmitPackets.begin(),rexmitPackets.end(),j) == rexmitPackets.end())
                        {
                            rexmitPackets.push_back(j);
                            rexmitMap[j] = false;
                            timeoutMap[j] = currTime + (long)conn_info.rto_estimate;

                            packetLossFlag = true;

                            if(j > lostPacketSeq)
                                lostPacketSeq = j;

                        }
                    }
                }
                if(packetLossFlag && (lostPacketSeq > seq_num_sent_before_loss))
                {
                    conn_info.ssthresh = unackedPackets.size()/2;
                    if(conn_info.ssthresh < 2)
                        conn_info.ssthresh = 2;
                    conn_info.congWindow = 1;

                    conn_info.slowStart = true;

                    seq_num_sent_before_loss = conn_info.lastSentSeq - 1;


                }

                lastAckedPacket = rangeEnd;

                bool newPacketsAckedFlag = false;

                // Remove acked packets by this SACK block from unacked/rexmit list.
                for(unsigned long j = rangeStart; j <= rangeEnd; j++)
                {
                    packetTimeMap.erase(j);
                    rexmitMap.erase(j);
                    timeoutMap.erase(j);
                    if(find(unackedPackets.begin(),unackedPackets.end(),j) != unackedPackets.end())
                    {
                        newPacketsAckedFlag = true;
                        unackedPackets.erase(find(unackedPackets.begin(),unackedPackets.end(),j));
                    }
                    if(find(rexmitPackets.begin(), rexmitPackets.end(),j) != rexmitPackets.end())
                    {
                        newPacketsAckedFlag = true;
                        rexmitPackets.erase(find(rexmitPackets.begin(), rexmitPackets.end(),j));
                    }
                }

                // If this SACK packet is acknowledging receipt of a new packet,
                // reset the timeout.
                if(newPacketsAckedFlag)
                    retransTimer = currTime + (int)conn_info.rto_estimate;
            }

            list<unsigned long>::iterator rexmitIter;
            if(rexmitPackets.size() > 0)
            {
            //    printf("Rexmit packets = ");
            }
            for(rexmitIter = rexmitPackets.begin();rexmitIter != rexmitPackets.end(); rexmitIter++)
            {
             //   printf("%d(%d) ", (*rexmitIter), rexmitMap[(*rexmitIter)]);
            }
            if(rexmitPackets.size() > 0)
            {
              //  printf("\n");
            }

        }
        else
        {
            //printf("Received an ACK for %d\n", seqNum);
        }

    }
    else
    {
        printf("ERROR: Unknown UDP packet received from remote agent\n");
        return;
    }
}

int xmit_packet(char *message, int messageLen)
{

    int flags = 0;

    return sendto(clientSocket, message, messageLen, flags,
            (struct sockaddr *) &remoteServAddr,
            sizeof(remoteServAddr));
}

int main(int argc, char **argv)
{
    int rc, i, n, flags = 0;
    socklen_t echoLen;
    struct sockaddr_in servAddr, localHostAddr;
    struct hostent *host1, *localhostEnt;
    char msg[MAX_MSG];

    string localHostName = argv[1];
    string hostName = argv[2];
    int portNum = atoi(argv[3]);
    unsigned long long connDuration = atoi(argv[4]); 

    localhostEnt = gethostbyname(argv[1]);
    memcpy((char *) &localHostAddr.sin_addr.s_addr,
            localhostEnt->h_addr_list[0], localhostEnt->h_length);
    init_pcap(inet_ntoa(localHostAddr.sin_addr));
    int pcapfd = pcap_get_selectable_fd(pcapDescriptor);


    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

    host1 = NULL;
    host1 = gethostbyname(hostName.c_str());
    if(host1 == NULL)
    {
        printf("ERROR: Unknown host %s\n", hostName.c_str());
        exit(1);
    }

    remoteServAddr.sin_family = host1->h_addrtype;
    memcpy((char *) &remoteServAddr.sin_addr.s_addr,
            host1->h_addr_list[0], host1->h_length);
    remoteServAddr.sin_port = htons(REMOTE_SERVER_PORT);

    startTime = getTimeMilli();
    unsigned long long lastSentTime = startTime;
    unsigned long long lastPrintTime = startTime;
    char messageString[1458];
    fd_set socketReadSet, socketWriteSet;

    // Initialize the congestion window and slow start threshold.
    conn_info.congWindow = 2.0;
    conn_info.ssthresh = 65000;
    conn_info.lastAckedSeq = 1;
    conn_info.lastSentSeq = 1;
    conn_info.slowStart = true;

    // Initialize the timeout value to be one second.
    conn_info.rto_estimate = 1000;
    conn_info.rtt_est = 1000;
    conn_info.rtt_deviation_est = 0;
    retransTimer = 0;

    connDuration *= 1000;
    rexmitMap.clear();
    packetTimeMap.clear();

    // Used for passing along the timeout information to the retransmission part of the code.
    bool timeoutFlag = false;
    bool timeoutProcessed = true; // Helps not to decrease the congestion window multiple
    // times before the last timeout was addressed.

    reportedLost = 0;
    highSeq = 0;

    seq_num_sent_before_loss = 0;
    struct pcap_stat pcapStatObj;
    {
        int bufferSize = 0;
        socklen_t buflen = sizeof(bufferSize);
        int retVal = getsockopt( pcapfd, SOL_SOCKET, SO_RCVBUF, 
                &bufferSize, &buflen);
        printf("Retval = %d, Recv buffer size = %d bytes\n", retVal, bufferSize);

    }

    // For each combination(pair), send a train of UDP packets.
    while (( lastSentTime - startTime) < connDuration) 
    {
        FD_ZERO(&socketReadSet);
        FD_SET(clientSocket,&socketReadSet);
        FD_SET(pcapfd,&socketReadSet);
        FD_ZERO(&socketWriteSet);
        FD_SET(clientSocket,&socketWriteSet);

        struct timeval timeoutStruct;

        timeoutStruct.tv_sec = 0;
        timeoutStruct.tv_usec = 10000;
////////////////////////
        while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);

        select(clientSocket+pcapfd+1,&socketReadSet,&socketWriteSet,0,&timeoutStruct);

        if (FD_ISSET(pcapfd,&socketReadSet) )
        {
                    //while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);
                    pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL);
        }

        if (FD_ISSET(clientSocket,&socketReadSet) )
        {
            while (true)
            {
                if( recvfrom(clientSocket, msg, MAX_MSG, flags,
                            (struct sockaddr *) &servAddr, &echoLen) != -1)
                {
          //          while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);
                }
                else
                    break;
            }
        }

        if (FD_ISSET(clientSocket,&socketWriteSet) != 0)
        {
            unsigned long long currTime;

            // Send packets until the congestion window is empty.
            while( (num_outstanding_packets - reportedLost) < conn_info.congWindow)
            {
                bool rexmitFlag = false;
                unsigned long rexmitSeq = 0;

                list<unsigned long>::iterator rexmitIter;

                //if(rexmitPackets.size() > 0)
                 //   printf("Rexmit packets = ");
                for(rexmitIter = rexmitPackets.begin();rexmitIter != rexmitPackets.end(); rexmitIter++)
                {
                    // Check if this packet has been queued for retransmission.
                    // If the timer has expired, retransmit the first packet in this queue
                    // even if it was sent before.
                  //  printf("%d(%d) ", (*rexmitIter), rexmitMap[(*rexmitIter)]);
                    if(rexmitMap[(*rexmitIter)] == false || timeoutFlag == true)
                    {
                        // Retransmit this packet.
                        rexmitFlag = true;
                        rexmitSeq = (*rexmitIter);
                        break;
                    }
                }
              //  if(rexmitPackets.size() > 0)
               //     printf("\n");

                // Retransmit a queued packet if possible.
                if(rexmitFlag)
                {
                    // Data packet.
                    messageString[0] = '0';

                    // Copy the sequence number into the packet data.
                    memcpy(&messageString[1], &rexmitSeq, sizeof(unsigned long));

                    rc = xmit_packet(messageString, 1458);

                    if(rc >= 0)
                    {
                        currTime = getTimeMilli();

                        rexmitMap[rexmitSeq] = true;
                        timeoutMap[rexmitSeq] = currTime + (long)conn_info.rto_estimate;

                        if(timeoutFlag == true) // Back off the timeout by a factor of 2 - capped to 32 seconds.
                        {
                            conn_info.rto_estimate = 2*conn_info.rto_estimate;
                            if(conn_info.rto_estimate > 32000)
                                conn_info.rto_estimate = 32000;

                            retransTimer = currTime + (long long)conn_info.rto_estimate;
                        }
                        else
                            retransTimer = currTime + (long long)conn_info.rto_estimate;

                        timeoutFlag = false;
                        timeoutProcessed = true;
                        num_outstanding_packets += 1;
                    }

                    continue;
                }

                // If we get here, there are no queued packets to be retransmitted,
                // send a new packet.
                {
                    // Data packet.
                    messageString[0] = '0';

                    // Copy the sequence number into the packet data.
                    memcpy(&messageString[1], &conn_info.lastSentSeq, sizeof(unsigned long));

                    rc = xmit_packet(messageString, 1458);

                    if(rc < 0)
                    {
                        break;
                    }
                    else
                    {
                        rexmitMap[conn_info.lastSentSeq] = false;
                        //Record a timestamp for the packet. This is less accurate than
                        // the libpcap timestamp, but serves as a backup if we fail to
                        // catch this packet on the way out due to libpcap buffer overflow.

                        // In the most common case, this is going to be revised when the
                        // packet is recorded by the libpcap filter function.
                        currTime = getTimeMilli();
                        packetTimeMap[conn_info.lastSentSeq] = currTime;
                        retransTimer = currTime + (long long)conn_info.rto_estimate;

                        // This is a new packet, increment the sequence number.
                        conn_info.lastSentSeq++;

                        num_outstanding_packets += 1;
                    }

                }

            }

            //////////////////////////
            while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);
        }

        // Check for timeouts and retransmit packet(s) if necessary.
        if(timeoutProcessed)
        {
            unsigned long long currTime = getTimeMilli();
            unsigned long seqNum;

            // Global timeout - this timer is reset on every packet send event & on
            // receiving acks that acknowledge new data.
            if(retransTimer < currTime)
            {
                //Retransmit the first unacked packet.
                if(unackedPackets.size() > 0)
                {
                    timeoutProcessed = false;
                    seqNum = unackedPackets.front();
                    if(find(rexmitPackets.begin(),rexmitPackets.end(),seqNum) == rexmitPackets.end())
                    {
                        rexmitPackets.push_back(seqNum);
                        rexmitMap[seqNum] = false;
                    }

                    // Halve the slow-start threshold and set congestion window size to 1.
                    // The cong. window should be halved only once per window - irrespective
                    // of the number of losses in that window.
                    if(seqNum > seq_num_sent_before_loss)
                    {
                        //conn_info.ssthresh = (conn_info.lastSentSeq - conn_info.lastAckedSeq)/2;
                        conn_info.ssthresh = unackedPackets.size()/2;
                        if(conn_info.ssthresh < 2)
                            conn_info.ssthresh = 2;
                        conn_info.congWindow = 1;

                        conn_info.slowStart = true;

                        seq_num_sent_before_loss = conn_info.lastSentSeq - 1;
                    }
                 //   else // Do not change the cong. window & ssthreshold for multiple losses
                        // in a single window.
                    {

                    }
                }
            }

            // Check the timers of retransmitted packets.
            list<unsigned long>::iterator rexmitIter;
            bool multipleTimeouts = false;

            for(rexmitIter = rexmitPackets.begin(); rexmitIter != rexmitPackets.end(); rexmitIter++)
            {
                // We rexmitted this packet already, but its timer has expired - send it again.
                if((rexmitMap[(*rexmitIter)] == true) && timeoutMap[(*rexmitIter)] < currTime)
                {
                    timeoutProcessed = false;
                    rexmitMap[(*rexmitIter)] = false;
                    multipleTimeouts = true;
                }
            }

            if(multipleTimeouts)
            {
                //conn_info.ssthresh = (conn_info.lastSentSeq - conn_info.lastAckedSeq)/2;
                conn_info.ssthresh = unackedPackets.size()/2;
                if(conn_info.ssthresh < 2)
                    conn_info.ssthresh = 2;
                conn_info.congWindow = 1;

                conn_info.slowStart = true;

                seq_num_sent_before_loss = conn_info.lastSentSeq - 1;
            }

        }
        lastSentTime = getTimeMilli();

        if( (lastSentTime - lastPrintTime) >= 2000)
        {
            cout << lastSentTime - startTime << " " << conn_info.congWindow<< " "<< conn_info.ssthresh << " , outstanding=" <<num_outstanding_packets <<", reportedLost = "<<reportedLost<<endl;
            pcap_stats(pcapDescriptor, &pcapStatObj);
            if(pcapStatObj.ps_drop > 0)
                printf("pcap: Packets received %d, dropped %d\n", pcapStatObj.ps_recv, pcapStatObj.ps_drop);
            lastPrintTime = lastSentTime;
        }
    }

    close(clientSocket);

    //double tput = (double)conn_info.lastAckedSeq/(double)connDuration;
    double tput = (double)(highSeq - reportedLost)/(double)connDuration;
    tput *= (1500*8);

    printf("Packets sent = %u(last Packet = %u, highSeq = %u), time = %lld seconds, throughput = %f Kbits/sec\n",conn_info.lastAckedSeq,conn_info.lastSentSeq,highSeq,connDuration/1000, tput );

}

