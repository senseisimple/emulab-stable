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
#include <set>

#define REMOTE_SERVER_PORT 9831
//#define MAX_MSG 450000
#define MAX_MSG 1458


using namespace std;

struct tcp_block{

    unsigned long HighData;
    unsigned long HighACK;
    unsigned long HighRxt;

    long pipe;
    unsigned long recoveryPoint;

    int dup_acks;

    double congWindow;
    int ssthresh;

    double rtt_est;
    double rtt_deviation_est;
    double rto_estimate;

    bool lossRecovery;

    set< pair<unsigned long, unsigned long> > sackRanges;
    int congAvoidancePackets;
    unsigned long sackEdge;


    // Temp variables.
    unsigned long range_start, range_end;
};

pcap_t *pcapDescriptor = NULL;
struct tcp_block conn_info;
int clientSocket;
struct sockaddr_in remoteServAddr;
unsigned long long startTime;

map<unsigned long, unsigned long long> packetTimeMap;
map<unsigned long, bool> rexmitMap;
long long retransTimer;
list<unsigned long> unackedPackets;
list<unsigned long> rexmitPackets;
unsigned long reportedLost;
unsigned long highSeq;

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket);
int xmit_packet(char *message, int messageLen);
unsigned long long getTimeMilli();
static int ackCount = 0;

template <class T> class In_Range : public std::unary_function<T, bool>
{
    public:
        bool operator() (T& val)
        {
            if( (val >= conn_info.range_start) && (val <= conn_info.range_end) )
                return true;
            else
                return false;
        }
};

bool IsLost(unsigned long seq)
{
    // If this sequence number was acked by any of the SACKs, we wouldn't find
    // it in the unacked list.
    /*
    if(checkUnAcked)
    {
        if(find(unackedPackets.begin(),unackedPackets.end(),seq) == unackedPackets.end())
        {
            return false;
        }
    }
    */

    { // If the packet is still unacked, check that it is below the highest seq.
        // sacked to be considered lost.
        //if( (conn_info.sackRanges.size() > 0) && (seq < (*(conn_info.sackRanges.rbegin())).second ) )
        if(seq < conn_info.sackEdge)
            return true;
        else
            return false;
    }
}

// Returns the sequence number of the next segment to be transmitted.
unsigned long NextSeq(bool timeout)
{
    list<unsigned long>::iterator listIter = unackedPackets.begin();

    // Return the sequence number of the first unacked packet in case of a timeout.
    if(timeout)
    {
        return (*listIter);
    }

    while(listIter != unackedPackets.end())
    {
        if( IsLost(*listIter) ) 
        {
            if( (*listIter) > conn_info.HighRxt )
            {
                //if( (*listIter) < (*(conn_info.sackRanges.rbegin())).second )
                if( (*listIter) < conn_info.sackEdge )
                {
                    return (*listIter);
                }
            }
        }

        listIter++;
    }

    // else - no unacked packets were eligible for retransmission - send a new packet.
    return (conn_info.HighData+1);
}

// Estimate of the number of packets outstanding in the network.
void SetPipe()
{
    unsigned long i;

    conn_info.pipe = 0;

    //for(i = conn_info.HighACK+1; i <= conn_info.HighData; i++)

    list<unsigned long>::iterator listIter = unackedPackets.begin();

    // Check only in the unacked list.
    while(listIter != unackedPackets.end())
    {
        if( ! IsLost((*listIter)) )
            conn_info.pipe++;

        //FIXME:
        //if((*listIter) <= conn_info.HighRxt)
         //   conn_info.pipe++;

        listIter++;
    }
}


/*
bool sackCompare( pair<unsigned long, unsigned long> pair1, pair<unsigned long, unsigned long> pair2)
{
    if(pair1.second <= pair2.second)
        return true;
    else
        return false;
}

*/

struct sackCompare
{

    bool operator()( pair<unsigned long, unsigned long> pair1, pair<unsigned long, unsigned long> pair2)
    {
        if(pair1.second <= pair2.second)
            return true;
        else
            return false;
    }
};

void UpdateSACKs(int numSACKBlocks, u_char *dataPtr)
{
    // Remove SACK ranges to the left side of packets which have been cumulatively acked.
    /*
    if(conn_info.HighACK >= (*(conn_info.sackRanges.begin())).second)
    {
        set<pair<unsigned long, unsigned long> >::iterator sackIter = conn_info.sackRanges.begin();

        while(sackIter != conn_info.sackRanges.end())
        {
            if(conn_info.HighACK >= (*sackIter).second)
                conn_info.sackRanges.erase(sackIter++);
            else
                sackIter++;
        }

    }
    */

    if(numSACKBlocks > 0)
    {
       // unsigned long lastSACKBlock = *(dataPtr + ((numSACKBlocks-1)*2 + 1 )*sizeof(unsigned long)); 
        pair<unsigned long, unsigned long> tmpRange;

                //printf("Received a SACK for seq = %u\n",conn_info.HighACK+1);

        // Erase packets acked by the SACK blocks from the unacked list.
        list<unsigned long>::iterator listIter;

        for(int i = 0;i < numSACKBlocks; i++)
        {
            tmpRange.first = *( (unsigned long *)(dataPtr + i*2*sizeof(unsigned long)));
            tmpRange.second = *( (unsigned long *)(dataPtr + (i*2+1)*sizeof(unsigned long)));

            /*
            if(conn_info.sackRanges.find(tmpRange) == conn_info.sackRanges.end())
            {
                conn_info.sackRanges.insert(tmpRange);
                cout << " Inserting "<<tmpRange.first<<", "<<tmpRange.second<<", size = "<<conn_info.sackRanges.size()<<endl;
            }
            */

            conn_info.range_start = tmpRange.first;
            conn_info.range_end = tmpRange.second;
            unackedPackets.remove_if( In_Range<unsigned long>() );

            /*
            for( unsigned long j = tmpRange.first; j <= tmpRange.second; j++)
            {
        //        listIter = find(unackedPackets.begin(),unackedPackets.end(),j);

         //       if(listIter != unackedPackets.end())
          //          unackedPackets.erase(listIter);

                rexmitMap.erase(j);
                packetTimeMap.erase(j);
            }
            */

            // Remember the highest SACK number seen.
            if(tmpRange.second > conn_info.sackEdge)
                conn_info.sackEdge = tmpRange.second;
        }

        // Update the SACK ranges.
        // It is not necessary to merge them - all that we care about is their
        // right most ( most recent ) edge.
        /*
        {
            for(int i = 0;i < numSACKBlocks; i++)
            {
                tmpRange.first = *( (unsigned long *)(dataPtr + i*2*sizeof(unsigned long)));
                tmpRange.second = *( (unsigned long *)(dataPtr + (i*2+1)*sizeof(unsigned long)));

                //cout<<"Adding range " << tmpRange.first << ", "<<tmpRange.second<<endl;
                conn_info.sackRanges.insert(tmpRange);
            }
        }
        */

        //conn_info.sackRanges.sort(sackCompare);

        /*
        // Remove duplicate entries.
        list<pair<unsigned long, unsigned long> >::iterator sackIter = conn_info.sackRanges.begin();
        unsigned long range_start, range_end;
        range_start = (*sackIter).first;
        range_end = (*sackIter).second;
        sackIter++;
        while(sackIter != conn_info.sackRanges.end())
        {
            if( ( (*sackIter).first == range_start) && ((*sackIter).second == range_end) )
            {
                conn_info.sackRanges.erase(sackIter++);
            }
            else
            {
                range_start = (*sackIter).first;
                range_end = (*sackIter).second;
                sackIter++;
            }
        }
        */

        /*
        list<pair<unsigned long, unsigned long> >::iterator sackIter = conn_info.sackRanges.begin();
        printf("SACK Ranges:\n");
        while(sackIter != conn_info.sackRanges.end())
        {
            printf("%d %d : ", (*sackIter).first, (*sackIter).second);
            sackIter++;
        }
        printf("\n");
        */
    }

    /*
        sackIter = conn_info.sackRanges.begin();
        printf("SACK Ranges after ( unacked = %d,front = %u, back = %u):\n",unackedPackets.size(), unackedPackets.front(), unackedPackets.back());
        while(sackIter != conn_info.sackRanges.end())
        {
            printf("%d %d :\n", (*sackIter).first, (*sackIter).second);
            sackIter++;
        }
        printf("\n");
        */

}

// Updates all the state variables.
//void Update(unsigned long seqNum, int numSACKBlocks, u_char *dataPtr, struct pcap_pkthdr const *pcap_info)
void Update(unsigned long seqNum, int numSACKBlocks, u_char *dataPtr, unsigned long long recvTime)
{
    unsigned long numPacketsAcked = 0;

    //printf("ACK for seq # = %u\n", seqNum);
    if((seqNum-1) > conn_info.HighACK)
    {
        numPacketsAcked = seqNum - conn_info.HighACK - 1;

        // Update the RTT estimates.
        // Only include non retransmitted packets in our calculations.
        if(! rexmitMap[seqNum-1] )
        {
            // Count only the packets that have been individually acked.
            // ie. only the single packet at the left most position in the
            // window. Do not take cumulatively acked packets into account.
            if((seqNum-1) == conn_info.HighACK+1)
            {
                //unsigned long long secVal = pcap_info->ts.tv_sec;
                //unsigned long long usecVal = pcap_info->ts.tv_usec;

                //long rtt_val = (secVal*1000 + usecVal/1000 - packetTimeMap[seqNum-1]);
                long rtt_val = (recvTime - packetTimeMap[seqNum-1]);

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

        list<unsigned long>::iterator listIter;
        // Remove all cumulatively acked packets from the unacked list.
        conn_info.range_start = conn_info.HighACK;
        conn_info.range_end = seqNum - 1;

        unackedPackets.remove_if( In_Range<unsigned long>() );

        for(unsigned long i = conn_info.HighACK; i < seqNum; i++)
        {
            
        //    listIter = find(unackedPackets.begin(),unackedPackets.end(),i);

         //   if(listIter != unackedPackets.end())
          //      unackedPackets.erase(listIter);

            rexmitMap.erase(i);
            packetTimeMap.erase(i);
        }

        conn_info.HighACK = seqNum - 1;

        // If all packets upto the loss recovery point have been acked, 
        // terminate loss recovery.
        if( ( conn_info.lossRecovery ) && ( seqNum >= conn_info.recoveryPoint ) )
        {
//            printf("Exiting loss recovery at %u\n", seqNum);
            conn_info.lossRecovery = false;
            conn_info.dup_acks = 0;
        }

        // Reset the timout value - because this ACK is acknowledging new data.
        unsigned long long currTime = getTimeMilli();
        retransTimer = currTime + (int)conn_info.rto_estimate;
    }
    else
    {
        numPacketsAcked = 1;

        if(! conn_info.lossRecovery )
        {
            conn_info.dup_acks++;

            // We received three duplicate ACKS - enter the loss recovery stage.
            if(conn_info.dup_acks == 3)
            {
                //printf("Entering loss recovery for seq = %u\n", seqNum);
                conn_info.lossRecovery = true;

                // Set the recovery point - multiple losses upto this point
                // will NOT result in multiple cong. window reductions.
                conn_info.recoveryPoint = conn_info.HighData;

                // Decrease the congestion window and slow start threshold.
                conn_info.ssthresh = unackedPackets.size() / 2;
                conn_info.congWindow = unackedPackets.size() / 2;
                if(conn_info.ssthresh < 2)
                {
                    conn_info.ssthresh = 2;
                    conn_info.congWindow = 2;
                }

                //printf("Setting ssthresh = %d\n", conn_info.ssthresh);
            }
        }
    }

    unsigned long long measure1, measure2;
                 //       measure1 = getTimeMilli();
    UpdateSACKs(numSACKBlocks, dataPtr);
    /*
                        measure2 = getTimeMilli();
        if(measure2 - measure1 > 2)
            cout << " Spent " << measure2 - measure1 << " ms in select\n";
            */

    SetPipe();

    // Congestion avoidance.
    if(conn_info.congWindow >= conn_info.ssthresh)
    {
        conn_info.congAvoidancePackets += numPacketsAcked;

        if(conn_info.congAvoidancePackets >= conn_info.congWindow)
        {
            conn_info.congWindow += 1.0;
            conn_info.congAvoidancePackets = 0;

            //FIXME:
            //if(conn_info.congWindow > 170)
             //   conn_info.congWindow = 170;
        }
    }
    else // Slow Start
    {
//        printf("For seq # = %u,packets = %d, window before = %f ", seqNum,numPacketsAcked, conn_info.congWindow);
        conn_info.congWindow += numPacketsAcked;
            //FIXME:
         //   if(conn_info.congWindow > 170)
          //      conn_info.congWindow = 170;
  //      printf(", window after = %f\n", conn_info.congWindow);
    }
//    cout << "CongWin: " << conn_info.congWindow<< " "<< conn_info.ssthresh << " , outstanding=" <<conn_info.pipe << ", last Range = "<<conn_info.sackRanges.back().second<<endl;
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

void init_pcap( char *ipAddress, bool WriteFlag)
{
    char interface[] = "eth4";
    struct bpf_program bpfProg;
    char errBuf[PCAP_ERRBUF_SIZE];
    char filter[128] = " udp ";

    // IP Address and sub net mask.
    bpf_u_int32 maskp, netp;
    struct in_addr localAddress;

    pcapDescriptor = pcap_open_live(interface, 120, 0, 0, errBuf);

    if(pcapDescriptor == NULL)
    {
        printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
        exit(1);
    }

    pcap_lookupnet(interface, &netp, &maskp, errBuf);
    localAddress.s_addr = netp;
    //printf("IP addr = %s\n", ipAddress);

    sprintf(filter," udp and ( (dst host %s and src port 9831 )) ", ipAddress);

    pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp);
    pcap_setfilter(pcapDescriptor, &bpfProg);
    pcap_setnonblock(pcapDescriptor, 1, errBuf);

}

void handleUDP(struct pcap_pkthdr const *pcap_info, struct udphdr const *udpHdr, u_char *const udpPacketStart, struct ip const *ipPacket)
{
    u_char *dataPtr = udpPacketStart + 8;

    char packetType = *(char *)(dataPtr);

    if(packetType == '0')
    {
        /*
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 1));
        unsigned long long currTime = getTimeMilli();

        unsigned long long secVal = pcap_info->ts.tv_sec;
        unsigned long long usecVal = pcap_info->ts.tv_usec;

        packetTimeMap[seqNum] = (unsigned long long)(secVal*1000 + usecVal/1000);

        // Set the retransmission timer.
        retransTimer = currTime + (unsigned long long)conn_info.rto_estimate;

        //cout<<"Sending seqNum "<<seqNum<<endl;

        unackedPackets.push_back(seqNum);
        */

    }
    else if(packetType == '1')
    {
        // We received an ACK.
        int sackBlocks = (int)(*(char *)(dataPtr + 1));
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 2));
        reportedLost = ( *(unsigned long *)(dataPtr + 2 + sizeof(unsigned long)));
        highSeq = ( *(unsigned long *)(dataPtr + 2 + 2*sizeof(unsigned long)));

        int sackStart = 2 + 3*sizeof(unsigned long);

        unsigned long long recvTime = pcap_info->ts.tv_sec*1000 + pcap_info->ts.tv_usec/1000;
        //cout << "Received an ACK for "<<seqNum<<endl;
        Update(seqNum, sackBlocks, (dataPtr + sackStart), recvTime);
    //    Update(seqNum, sackBlocks, (dataPtr + sackStart), pcap_info);

    }
    else
    {
        printf("ERROR: Unknown UDP packet received from remote agent\n");
        return;
    }
}

void handlePacket(u_char *const dataPtr, unsigned long long recvTime, bool newPacketTx)
{
    char packetType = *(char *)(dataPtr);

    if(packetType == '0')
    {
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 1));
       // unsigned long long currTime = getTimeMilli();

     //   unsigned long long secVal = pcap_info->ts.tv_sec;
      //  unsigned long long usecVal = pcap_info->ts.tv_usec;

       // packetTimeMap[seqNum] = (unsigned long long)(secVal*1000 + usecVal/1000);

        // Set the retransmission timer.
        //retransTimer = currTime + (unsigned long long)conn_info.rto_estimate;

      //  cout<<"Sending seqNum "<<seqNum<<endl;

        if(newPacketTx)
            unackedPackets.push_back(seqNum);

    }
    else if(packetType == '1')
    {
        // We received an ACK.
        ackCount++;
        int sackBlocks = (int)(*(char *)(dataPtr + 1));
        unsigned long seqNum = ( *(unsigned long *)(dataPtr + 2));
        reportedLost = ( *(unsigned long *)(dataPtr + 2 + sizeof(unsigned long)));
        highSeq = ( *(unsigned long *)(dataPtr + 2 + 2*sizeof(unsigned long)));

        /*
        if(sackBlocks == 0)
            cout << "Received an ACK for "<<seqNum<<endl;
        else
            cout << "HigSeq = "<<highSeq<<endl;
            */
        highSeq = ( *(unsigned long *)(dataPtr + 2 + 2*sizeof(unsigned long)));

        int sackStart = 2 + 3*sizeof(unsigned long);

        Update(seqNum, sackBlocks, (dataPtr + sackStart), recvTime);


    }
    else
    {
        printf("ERROR: Unknown UDP packet received from remote agent\n");
        return;
    }
}
int xmit_packet(u_char *message, int messageLen)
{

    int flags = 0;

    return sendto(clientSocket, message, messageLen, flags,
            (struct sockaddr *) &remoteServAddr,
            sizeof(remoteServAddr));
}

int main(int argc, char **argv)
{
    int rc, i, n, flags = 0;
    socklen_t echoLen = 0;
    struct sockaddr_in servAddr, localHostAddr;
    struct hostent *host1, *localhostEnt;
    u_char msg[2000];

    string localHostName = argv[1];
    string hostName = argv[2];
    int portNum = atoi(argv[3]);
    unsigned long long connDuration = atoi(argv[4]); 

    localhostEnt = gethostbyname(argv[1]);
    memcpy((char *) &localHostAddr.sin_addr.s_addr,
            localhostEnt->h_addr_list[0], localhostEnt->h_length);

    /*
    init_pcap(inet_ntoa(localHostAddr.sin_addr), true);
    int pcapfd = pcap_get_selectable_fd(pcapDescriptor);
    */


    /*
    init_pcap(inet_ntoa(localHostAddr.sin_addr), false);
    int pcapfd_read = pcap_get_selectable_fd(pcapDescriptor_Read);
    int pcapfd_write = pcap_get_selectable_fd(pcapDescriptor_Write);
    */

    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

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
    u_char messageString[1458];
    fd_set socketReadSet, socketWriteSet;

    // Initialize the congestion window and slow start threshold.
    conn_info.congWindow = 2.0;
    conn_info.ssthresh = 85;
    conn_info.HighACK = 0;
    conn_info.HighData = 0;
    conn_info.pipe = 0;
    conn_info.dup_acks = 0;

    // Initialize the timeout value to be one second.
    conn_info.rto_estimate = 3000;
    conn_info.rtt_est = 3000;
    conn_info.rtt_deviation_est = 0;
    retransTimer = 0;
    conn_info.sackEdge = 0;

    connDuration *= 1000;
    rexmitMap.clear();
    packetTimeMap.clear();

    // Used for passing along the timeout information to the retransmission part of the code.
    bool timeoutFlag = false;

    reportedLost = 0;
    highSeq = 0;

    /*
    struct pcap_stat pcapStatObj;
    {
        int bufferSize = 0;
        socklen_t buflen = sizeof(bufferSize);
        int retVal = getsockopt( pcapfd_read, SOL_SOCKET, SO_RCVBUF, 
                &bufferSize, &buflen);
        printf("Retval = %d, Recv buffer size = %d bytes\n", retVal, bufferSize);

    }
    */

    struct timeval timeoutStruct, writeTimeout;

    timeoutStruct.tv_sec = 0;
    timeoutStruct.tv_usec = 20000;

    writeTimeout.tv_sec = 0;
    writeTimeout.tv_usec = 2000;

    unsigned long long measure1, measure2;
    unsigned long long recvTime = getTimeMilli();

    while ( ( ( lastSentTime - startTime) < connDuration) && (( recvTime - startTime) < connDuration))
    {
        FD_ZERO(&socketReadSet);
        FD_SET(clientSocket,&socketReadSet);
        //FD_SET(pcapfd,&socketReadSet);

        //select(clientSocket+pcapfd_read+pcapfd_write+1,&socketReadSet,&socketWriteSet,0,&timeoutStruct);
        //select(clientSocket+1,&socketReadSet,&socketWriteSet,0,&timeoutStruct);
        select(clientSocket+1,&socketReadSet,NULL,0,&timeoutStruct);

        //select(pcapfd+clientSocket+1,&socketReadSet,&socketWriteSet,0,&timeoutStruct);

        /*
        if (FD_ISSET(pcapfd,&socketReadSet) )
        {
          //  while(pcap_dispatch(pcapDescriptor, 100, pcapCallback, NULL) != 0);
            pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL);
        }
        */
        /*
        if (FD_ISSET(pcapfd_write,&socketReadSet) )
        {
            //while(pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL) != 0);
            pcap_dispatch(pcapDescriptor_Write, 10000, pcapCallback, NULL);
        }
        */

        if (FD_ISSET(clientSocket,&socketReadSet) )
        {
            recvTime = getTimeMilli();
            //cout << recvTime - startTime << " " << conn_info.congWindow<< " "<< conn_info.ssthresh << " , outstanding=" <<conn_info.pipe <<", reportedLost = "<<reportedLost<<endl;
            //memset(msg, 0x0, 100);
            int retval = 1;
            flags = 0;

       //     cout<<"Into recv at "<<recvTime - startTime<<endl;
            //while( (retval = recvfrom(clientSocket, (void *)msg, MAX_MSG, flags,(struct sockaddr *) &servAddr, &echoLen) ) > 0)
         //   do
            {
                retval = recvfrom(clientSocket, (void *)msg, 75, flags,(struct sockaddr *) &servAddr, &echoLen) ;
                if(retval > 0)
                {
                    /*
                    if(retval > 120)
                    {
                        printf("ERROR, packet size = %d\n",retval);
                        exit(1);
                    }
                    */
                    handlePacket(msg, recvTime,false);
                    //memset(msg, 0x0, 100);
                }
                /*
                else
                {
                 //   printf("Retval = %d\n",retval);
                  //  printf("ERRNO = %d\n", errno);
                }
                //pcap_dispatch(pcapDescriptor, 300, pcapCallback, NULL);
                */
            }
        //    while(retval > 0);

        //    recvTime = getTimeMilli();
         //   cout<<"Out of recv at "<<recvTime - startTime<<endl;

        }

        if( (conn_info.congWindow - conn_info.pipe >= 1.0) || timeoutFlag ) 
        {
            FD_ZERO(&socketWriteSet);
            FD_SET(clientSocket,&socketWriteSet);

            select(clientSocket+1,NULL,&socketWriteSet,0,&writeTimeout);

            if (FD_ISSET(clientSocket,&socketWriteSet) != 0)
            {
                unsigned long long currTime;
                unsigned long nextSeq;

                currTime = getTimeMilli();

                // Send a packet as long as the oustanding data is less than cong. window
                while( (conn_info.congWindow - conn_info.pipe >= 1.0) || timeoutFlag ) 
                {
                    nextSeq = NextSeq(timeoutFlag);


                    memset(messageString, 0x0, 100);
                    // Data packet.
                    messageString[0] = '0';

                    // Copy the sequence number into the packet data.
                    memcpy(&messageString[1], &nextSeq, sizeof(unsigned long));

                    rc = xmit_packet(messageString, 1458);

                    if(rc <= 0)
                        break;
                    else
                    {
                        //Record a timestamp for the packet. This is less accurate than
                        // the libpcap timestamp, but serves as a backup if we fail to
                        // catch this packet on the way out due to libpcap buffer overflow.

                        // In the most common case, this is going to be revised when the
                        // packet is recorded by the libpcap filter function.
                        packetTimeMap[conn_info.HighData] = currTime;

                        retransTimer = currTime + (long long)conn_info.rto_estimate;

                        // Reduce the congestion window to '1' on a timeout.
                        if(timeoutFlag)
                        {
                            conn_info.ssthresh = unackedPackets.size()/2;
                            if(conn_info.ssthresh < 2)
                                conn_info.ssthresh = 2;

                            conn_info.congWindow = 1;

                            timeoutFlag = false;
                            //printf("Timed out\n");

                            conn_info.rto_estimate *= 2;

                            if(conn_info.rto_estimate > 60)
                                conn_info.rto_estimate = 60;
                        }

                        //cout << "Transmitting # "<<conn_info.HighData<<", Time = "<<currTime - startTime<<endl;
                        // This is a new packet, increment the sequence number.
                        if(nextSeq > conn_info.HighData)
                        {
                            conn_info.HighData = nextSeq;
                            conn_info.pipe++;
                            handlePacket(messageString, currTime,true);
                        }
                        else if( (nextSeq > conn_info.HighRxt) && conn_info.lossRecovery )
                        {
                            conn_info.HighRxt = nextSeq;

                            // Mark this as a retransmitted packet - doesn't count
                            // towards RTT/RTO calculation.
                            rexmitMap[nextSeq] = true;
                            handlePacket(messageString, currTime,false);
                        }

                        // Recalculate the number of outstanding packets.
                        //    SetPipe();
                    }
                }

                //////////////////////////
                //pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL);



                //cout << "Last sent at time = "<<lastSentTime - startTime<<endl;
            }
        }

        lastSentTime = getTimeMilli();
        // Check the retransmission timer.
        if(retransTimer < lastSentTime || retransTimer < recvTime)
        {
            timeoutFlag = true;

            // Start retransmitting from the first unacked packet.
            conn_info.HighRxt = conn_info.HighACK;
        }

        /*
        if( (lastSentTime - lastPrintTime) >= 100)
        {
            cout << lastSentTime - startTime << " " << conn_info.congWindow<< " "<< conn_info.ssthresh << " , outstanding=" <<conn_info.pipe <<", reportedLost = "<<reportedLost<<endl;
            //pcap_stats(pcapDescriptor_Read, &pcapStatObj);
            //if(pcapStatObj.ps_drop > 0)
            //   printf("pcap: Packets received %d, dropped %d\n", pcapStatObj.ps_recv, pcapStatObj.ps_drop);
            lastPrintTime = lastSentTime;
        }
        */
        /*
        if( (recvTime - lastPrintTime) >= 100)
        {
//            cout << recvTime - startTime << " " << conn_info.congWindow<< " "<< conn_info.ssthresh << " , outstanding=" <<conn_info.pipe <<", reportedLost = "<<reportedLost<<endl;
            //pcap_stats(pcapDescriptor_Read, &pcapStatObj);
            //if(pcapStatObj.ps_drop > 0)
            //   printf("pcap: Packets received %d, dropped %d\n", pcapStatObj.ps_recv, pcapStatObj.ps_drop);
            lastPrintTime = recvTime;
        }
        */

    }

    close(clientSocket);

    //double tput = (double)conn_info.HighACK/(double)connDuration;
    //double tput = (double)(conn_info.HighACK - reportedLost)/(double)connDuration;
    double tput = (double)(conn_info.HighData - unackedPackets.size())/(double)connDuration;
    tput *= (1500*8);

    //printf("Packets sent = %u(last Packet = %u, highSeq = %u), time = %lld seconds, throughput = %f Kbits/sec\n",conn_info.HighACK,conn_info.HighData,highSeq,connDuration/1000, tput );
    printf("%d Kbits/sec\n", (long)tput );

    //printf("Ack count = %d\n", ackCount);
}

