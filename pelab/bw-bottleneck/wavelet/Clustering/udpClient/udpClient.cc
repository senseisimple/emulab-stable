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
#include <string>
#include <map>
#include <sstream>

#define REMOTE_SERVER_PORT 19835
#define MAX_MSG 100


#define SOCKET_ERROR -1
pcap_t *pcapDescriptor = NULL;

using namespace std;

vector< vector<int> > delaySequenceArray;
vector< map<unsigned long long, int> > packetTimeMaps;
vector< map<string, unsigned long long> > actualTimeMaps;

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


    char packetType = *(char *)(dataPtr);
    long long milliSec = 0;
    int ackLength = 0;

    if(packetType == '0')
    {
        int hostIndex = ( *(short int *)(dataPtr + 1));
        unsigned long long origTimestamp;
        unsigned long long secVal = pcap_info->ts.tv_sec;
        unsigned long long usecVal = pcap_info->ts.tv_usec;

        memcpy(&origTimestamp, ( unsigned long long *)(dataPtr + 1 + sizeof(short int)), sizeof(unsigned long long));
        ostringstream tmpStrStream;
        tmpStrStream << origTimestamp;
        actualTimeMaps[hostIndex][tmpStrStream.str()] = (unsigned long long)(secVal*1000 + usecVal/1000);
        printf("Recording Timestamp = %s for Host = %d, value = %llu\n", tmpStrStream.str().c_str(), hostIndex, actualTimeMaps[hostIndex][tmpStrStream.str()]);
    }
    else if(packetType == '1')
    {
        // We received an ACK, pass it on to the sensors.
        int hostIndex = ( *(short int *)(dataPtr + 1));
        unsigned long long origTimestamp;
        long long oneWayDelay;
        memcpy(&origTimestamp, ( unsigned long long *)(dataPtr + 1 + sizeof(short int)), sizeof(unsigned long long));
        memcpy(&oneWayDelay, ( long long *)(dataPtr + 1 + sizeof(short int) + sizeof(unsigned long long)), sizeof(long long));
        ostringstream tmpStrStream;
        tmpStrStream << origTimestamp;
        cout << " Onewaydelay for the ACK = " << oneWayDelay << ", host Index = "<< hostIndex << "\n";
        cout <<" Orig timestamp was "<< tmpStrStream.str() << " , actual time = "<< actualTimeMaps[hostIndex][tmpStrStream.str()]<<"\n";
        cout <<"Packet time map Index = "<< packetTimeMaps[hostIndex][origTimestamp] << ", host index = " << hostIndex << " \n";
        delaySequenceArray[hostIndex][packetTimeMaps[hostIndex][origTimestamp]] = oneWayDelay - ( actualTimeMaps[hostIndex][tmpStrStream.str()] - origTimestamp);

        if(oneWayDelay < 50000 && oneWayDelay > -50000 && (delaySequenceArray[hostIndex][packetTimeMaps[hostIndex][origTimestamp]] > 100000 || delaySequenceArray[hostIndex][packetTimeMaps[hostIndex][origTimestamp]] < -100000 ) )
        {
            cout<<"ERROR: Incorrect delay value, one way delay = "<< oneWayDelay<<", Calculated delay = "<< delaySequenceArray[hostIndex][packetTimeMaps[hostIndex][origTimestamp]]<<"\n";


        }
        if(actualTimeMaps[hostIndex].count(tmpStrStream.str()) == 0)
        {
            printf("ERROR: Original timestamp was not found in the packet Hash\n\n");
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

void init_pcap( char *ipAddress)
{
    char interface[] = "eth0";
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
    sprintf(filter," udp and ( (src host %s and dst port 19835 ) or (dst host %s and src port 19835 )) ", ipAddress, ipAddress);

    if(pcapDescriptor == NULL)
    {
        printf("Error opening device %s with libpcap = %s\n", interface, errBuf);
        exit(1);
    }

    pcap_compile(pcapDescriptor, &bpfProg, filter, 1, netp); 
    pcap_setfilter(pcapDescriptor, &bpfProg);
    pcap_setnonblock(pcapDescriptor, 1, errBuf);

}

int main(int argc, char **argv)
{
    int clientSocket, rc, i, n, flags = 0, error, timeOut;
    socklen_t echoLen;
    struct sockaddr_in cliAddr, remoteServAddr1, remoteServAddr2, servAddr, localHostAddr;
    struct hostent *host1, *host2, *localhostEnt;
    char msg[MAX_MSG];
    vector<struct sockaddr_in> remoteServAddresses;

    string hostNameFile = argv[1];
    string outputDirectory = argv[2];
    string localHostName = argv[3];

    int timeout = 2000; // 1 second
    int probeRate = 10; // Hz
    int probeDuration = 15000; // 15 seconds
    vector<string> hostList;

    ifstream inputFileHandle;

    localhostEnt = gethostbyname(argv[3]);
    memcpy((char *) &localHostAddr.sin_addr.s_addr, 
            localhostEnt->h_addr_list[0], localhostEnt->h_length);
    init_pcap(inet_ntoa(localHostAddr.sin_addr));
    int pcapfd = pcap_get_selectable_fd(pcapDescriptor);

    // Create the output directory.
    string commandString = "mkdir " + outputDirectory;
    system(commandString.c_str());

    // Read the input file having all the planetlab node IDs.

    inputFileHandle.open(hostNameFile.c_str(), std::ios::in);

    char tmpStr[81] = "";
    string tmpString;
    bool enableClientFlag = false;

    while(!inputFileHandle.eof())
    {
        inputFileHandle.getline(tmpStr, 80); 
        tmpString = tmpStr;

        if(tmpString.size() < 3)
            continue;
        if(tmpString != localHostName)
        {
            hostList.push_back(tmpString);
            cout << tmpString << "\n";
        }
        else
            enableClientFlag = true;
    }
    inputFileHandle.close();

    // We don't need to run the probe client on this node - just exit.
    if(enableClientFlag == false)
        exit(0);

    int numHosts = hostList.size();

    remoteServAddresses.resize(numHosts);
    for(int i = 0;i < numHosts; i++)
    {
        host1 = NULL;
        host1 = gethostbyname(hostList[i].c_str());

        remoteServAddresses[i].sin_family = host1->h_addrtype;
        memcpy((char *) &remoteServAddresses[i].sin_addr.s_addr, 
                host1->h_addr_list[0], host1->h_length);
        remoteServAddresses[i].sin_port = htons(REMOTE_SERVER_PORT);
    }

    int targetSleepTime = (1000/probeRate) - 1;

    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliAddr.sin_port = htons(0);

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    rc = bind(clientSocket, (struct sockaddr *) &cliAddr, sizeof(cliAddr));

    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

    delaySequenceArray.resize(numHosts);
    packetTimeMaps.resize(numHosts);
    actualTimeMaps.resize(numHosts);

    /////////////////////////////
    // For each destination, send a train of UDP packets.
    int packetCounter = 0;
    unsigned long long startTime = getTimeMilli();
    unsigned long long lastSentTime = startTime;
    bool endProbesFlag = false;
    bool readTimeoutFlag = false;

    while ((( lastSentTime - startTime) < probeDuration) || !(readTimeoutFlag))
    {

        // Stop waiting for probe replies after a timeout - calculated from the
        // time the last probe was sent out.
        if (endProbesFlag && ( (getTimeMilli() - lastSentTime) > timeout))
            readTimeoutFlag = 1;
        // Stop sending probes after the given probe duration.
        if (!(endProbesFlag) && (lastSentTime - startTime) > probeDuration)
            endProbesFlag = 1;

        if (endProbesFlag)
            usleep(timeout*100);

        fd_set socketReadSet, socketWriteSet;
        FD_ZERO(&socketReadSet);
        FD_SET(clientSocket,&socketReadSet);
        FD_SET(pcapfd,&socketReadSet);
        FD_ZERO(&socketWriteSet);
        FD_SET(clientSocket,&socketWriteSet);

        struct timeval timeoutStruct;

        timeoutStruct.tv_sec = 0;
        timeoutStruct.tv_usec = 0;

        if (!endProbesFlag)
        {
            select(clientSocket+pcapfd+1,&socketReadSet,&socketWriteSet,0,&timeoutStruct);
        }
        else
        {
            select(clientSocket+pcapfd+1,&socketReadSet,0,0,&timeoutStruct);
        }

        if (FD_ISSET(pcapfd,&socketReadSet) )
        {
            pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL);
        }
        if (!readTimeoutFlag)
        {
            if (FD_ISSET(clientSocket,&socketReadSet) != 0)
            {
                while (true)
                {
                    int flags = 0;
                    if( recvfrom(clientSocket, msg, MAX_MSG, flags,
                                (struct sockaddr *) &servAddr, &echoLen) != -1)
                    {
                        while(pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL) != 0);
                    }
                    else
                    {
                        if(endProbesFlag)
                            usleep(timeout*100);
                        break;
                    }
                }
            }
        }

        if (!endProbesFlag)
        {
            if (FD_ISSET(clientSocket,&socketWriteSet) != 0)
            {
                char messageString[60];
                int flags = 0;
                short int hostIndex;
                for (int i = 0; i < numHosts; i++)
                {
                    // Send the probe packets.
                    unsigned long long sendTime = getTimeMilli();
                    messageString[0] = '0';
                    hostIndex = i;

                    memcpy(&messageString[1], &hostIndex, sizeof(short int));
                    memcpy(&messageString[1 + sizeof(short int)], &sendTime, sizeof(unsigned long long));

                    rc = sendto(clientSocket, messageString, 1 + sizeof(short int) + sizeof(unsigned long long), flags, 
                            (struct sockaddr *) &remoteServAddresses[i], sizeof(remoteServAddresses[i]));
                            
                    if(rc < 0)
                        printf("ERROR sending %dth udp message, packetCounter = %d\n", i, packetCounter);

                    packetTimeMaps[i][sendTime] = packetCounter;
                    delaySequenceArray[i].push_back(-9999);

                    cout<< "TO " << hostList[i] << " :Counter=" << packetCounter << " :SendTime= " << sendTime << endl;


                }
                while(pcap_dispatch(pcapDescriptor, -1, pcapCallback, NULL) != 0);
                packetCounter++;
                lastSentTime = getTimeMilli();
                // Sleep for 99 msec for a 10Hz target probing rate.
                usleep(targetSleepTime*1000);
            }
            else
            {
                if (!(getTimeMilli() - lastSentTime > targetSleepTime))
                {
                    cout << " About to sleep for " << ( targetSleepTime - (getTimeMilli() - lastSentTime) )*1000 <<"\n";
                    usleep(  ( targetSleepTime - (getTimeMilli() - lastSentTime) )*1000) ;
                }
            }
        }
    }
    //////////////////////////////

    usleep(20000000);

    while(pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL) != 0);

    for (int i = 0; i < numHosts; i++)
    {
        string dirPath = outputDirectory + "/" + hostList[i];
        commandString = "mkdir " + dirPath;
        system(commandString.c_str());

        ofstream tmpFileHandle;
        string tmpFilePath = dirPath + "/" + "debug.log";
        tmpFileHandle.open(tmpFilePath.c_str(), std::ios::out);

        for(int k = 0; k < delaySequenceArray[i].size(); k++)
        {
            tmpFileHandle << delaySequenceArray[i][k] << "\n"; 
        }
        tmpFileHandle.close();

        // If we lost some replies/packets, linearly interpolate their delay values.
        int delaySeqLen = delaySequenceArray[i].size();
        int firstSeenIndex = -1;
        int lastSeenIndex = -1;

        for (int k = 0; k < delaySeqLen; k++)
        {
            if (delaySequenceArray[i][k] != -9999) 
            {
                lastSeenIndex = k;
                if (firstSeenIndex == -1)
                    firstSeenIndex = k;
            }
        }

        if (lastSeenIndex != -1)
        {
            for (int k = firstSeenIndex; k < lastSeenIndex + 1; k++)
            {
                if (delaySequenceArray[i][k] == -9999)
                {
                    // Find the number of missing packets in this range.
                    int numMissingPackets = 0;
                    int lastInRange = 0;
                    for (int l = k; l < lastSeenIndex + 1; l++)
                    {
                        if(delaySequenceArray[i][l] == -9999)
                        {
                            numMissingPackets++;
                        }
                        else
                        {
                            lastInRange = l;
                            break;
                        }
                    }

                    int step = (delaySequenceArray[i][lastInRange] - delaySequenceArray[i][k-1])/(numMissingPackets + 1);

                    // Interpolate delays for the missing packets in this range.
                    int y = 0;
                    for (int x = k, y = 1; x < lastInRange; x++, y++)
                        delaySequenceArray[i][x] = delaySequenceArray[i][k-1] + y*step ;
                }
            }
        }

        ofstream outputFileHandle;
        string delayFilePath = dirPath + "/" + "delay.log";
        outputFileHandle.open(delayFilePath.c_str(), std::ios::out);
        if (lastSeenIndex != -1)
        {
            for (int k = firstSeenIndex; k < lastSeenIndex + 1; k++)
            {
                outputFileHandle << delaySequenceArray[i][k] <<  "\n"; 
            }
        }
        outputFileHandle.close();
        if (lastSeenIndex == -1)
            cout<< "ERROR: No samples were seen for host: " << hostList[i] << endl;
    }
}

