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

#define REMOTE_SERVER_PORT 9831
#define MAX_MSG 100


#define SOCKET_ERROR -1
pcap_t *pcapDescriptor = NULL;

using namespace std;

vector< vector< vector< vector<int> > > > delaySequenceArray;
vector< vector< vector< map<unsigned long long, int> > > > packetTimeMaps;
vector< vector< vector< map<string, unsigned long long> > > > actualTimeMaps;

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
        int xIndex =  *(short int *)(dataPtr + 1 + sizeof(short int));
        int yIndex =  *(short int *)(dataPtr + 1 + 2*sizeof(short int));
        cout <<"SEND: xIndex = "<<xIndex<<" ,yIndex = "<<yIndex<<"\n";
        unsigned long long origTimestamp;
        unsigned long long secVal = pcap_info->ts.tv_sec;
        unsigned long long usecVal = pcap_info->ts.tv_usec;

        memcpy(&origTimestamp, ( unsigned long long *)(dataPtr + 1 + 3*sizeof(short int)), sizeof(unsigned long long));
        ostringstream tmpStrStream;
        tmpStrStream << origTimestamp;
        actualTimeMaps[xIndex][yIndex][hostIndex][tmpStrStream.str()] = (unsigned long long)(secVal*1000 + usecVal/1000);
        printf("Recording Timestamp = %s for Host = %d, value = %llu\n", tmpStrStream.str().c_str(), hostIndex, actualTimeMaps[xIndex][yIndex][hostIndex][tmpStrStream.str()]);
    }
    else if(packetType == '1')
    {
        // We received an ACK, pass it on to the sensors.
        int hostIndex = ( *(short int *)(dataPtr + 1));
        int xIndex =  * (short int *)(dataPtr + 1 + sizeof(short int));
        int yIndex =  * (short int *)(dataPtr + 1 + 2*sizeof(short int));
        cout <<"RECV: xIndex = "<<xIndex<<" ,yIndex = "<<yIndex<<"\n";
        unsigned long long origTimestamp;
        long long oneWayDelay;
        memcpy(&origTimestamp, ( unsigned long long *)(dataPtr + 1 + 3*sizeof(short int)), sizeof(unsigned long long));
        memcpy(&oneWayDelay, ( long long *)(dataPtr + 1 + 3*sizeof(short int) + sizeof(unsigned long long)), sizeof(long long));
        ostringstream tmpStrStream;
        tmpStrStream << origTimestamp;
        cout << " Onewaydelay for the ACK = " << oneWayDelay << ", host Index = "<< hostIndex << "\n";
        cout <<" Orig timestamp was "<< tmpStrStream.str() << " , actual time = "<< actualTimeMaps[xIndex][yIndex][hostIndex][tmpStrStream.str()]<<"\n";
        cout <<"Packet time map Index = "<< packetTimeMaps[xIndex][yIndex][hostIndex][origTimestamp] << ", host index = " << hostIndex << " \n";
        delaySequenceArray[xIndex][yIndex][hostIndex][packetTimeMaps[xIndex][yIndex][hostIndex][origTimestamp]] = oneWayDelay - ( actualTimeMaps[xIndex][yIndex][hostIndex][tmpStrStream.str()] - origTimestamp);

        if(oneWayDelay < 50000 && oneWayDelay > -50000 && (delaySequenceArray[xIndex][yIndex][hostIndex][packetTimeMaps[xIndex][yIndex][hostIndex][origTimestamp]] > 100000 || delaySequenceArray[xIndex][yIndex][hostIndex][packetTimeMaps[xIndex][yIndex][hostIndex][origTimestamp]] < -100000 ) )
        {
            cout<<"ERROR: Incorrect delay value, one way delay = "<< oneWayDelay<<", Calculated delay = "<< delaySequenceArray[xIndex][yIndex][hostIndex][packetTimeMaps[xIndex][yIndex][hostIndex][origTimestamp]]<<"\n";


        }
        if(actualTimeMaps[xIndex][yIndex][hostIndex].count(tmpStrStream.str()) == 0)
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

int main(int argc, char **argv)
{
    int clientSocket, rc, i, n, flags = 0, error, timeOut;
    socklen_t echoLen;
    struct sockaddr_in cliAddr, remoteServAddr1, remoteServAddr2, servAddr, localHostAddr;
    struct hostent *host1, *host2, *localhostEnt;
    char msg[MAX_MSG];

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
            hostList.push_back(tmpString);
        else
            enableClientFlag = true;
    }

    printf("Num Hosts = %d\n", hostList.size());
    for(int test = 0;test < hostList.size(); test++)
        printf("%s\n", hostList[test].c_str());

    
    // We don't need to run the probe client on this node - just exit.
    if(enableClientFlag == false)
        exit(0);

    inputFileHandle.close();


    int numHosts = hostList.size();
    int targetSleepTime = (1000/probeRate) - 1;

    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliAddr.sin_port = htons(0);

    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    rc = bind(clientSocket, (struct sockaddr *) &cliAddr, sizeof(cliAddr));

    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);

    delaySequenceArray.resize(numHosts-1);
    packetTimeMaps.resize(numHosts-1);
    actualTimeMaps.resize(numHosts-1);

    for (int i = 0; i < numHosts - 1; i++)
    {
        host1 = NULL;
        host1 = gethostbyname(hostList[i].c_str());
        if(host1 == NULL) 
        {
            printf("ERROR: Unknown host %s\n", hostList[i].c_str());
            exit(1);
        }

        remoteServAddr1.sin_family = host1->h_addrtype;
        memcpy((char *) &remoteServAddr1.sin_addr.s_addr, 
                host1->h_addr_list[0], host1->h_length);
        remoteServAddr1.sin_port = htons(REMOTE_SERVER_PORT);
        cout << "First host IP = "<<inet_ntoa(remoteServAddr1.sin_addr)<<"\n";


        delaySequenceArray[i].resize(numHosts - i - 1);
        packetTimeMaps[i].resize(numHosts - i - 1);
        actualTimeMaps[i].resize(numHosts -i - 1);


        for( int j = i+1; j < numHosts; j++)
        {
            printf("PROGRESS: i = %d/%d, j = %d/%d\n",i, numHosts - 2, j, numHosts - 1); 

            host2 = NULL;
            host2 = gethostbyname(hostList[j].c_str());
            if(host2 == NULL) 
            {
                printf("ERROR: Unknown host %s\n", hostList[j].c_str());
                exit(1);
            }
            ///////////////////////////////
            //string iperfCmd1, iperfCmd2;

            //iperfCmd1 = "/usr/bin/iperf -c " + hostList[i] + " -t 15 -P 3 -p 9999 &";
            //iperfCmd2 = "/usr/bin/iperf -c " + hostList[j] + " -t 15 -P 3 -p 9999 &";

            //system(iperfCmd1.c_str());
            //system(iperfCmd2.c_str());
            //////////////////////////////


            remoteServAddr2.sin_family = host2->h_addrtype;
            memcpy((char *) &remoteServAddr2.sin_addr.s_addr, 
                    host2->h_addr_list[0], host2->h_length);
            remoteServAddr2.sin_port = htons(REMOTE_SERVER_PORT);

            delaySequenceArray[i][j-i-1].resize(2);
            packetTimeMaps[i][j-i-1].resize(2);
            actualTimeMaps[i][j-i-1].resize(2);

            delaySequenceArray[i][j-i-1][0].resize(0);
            delaySequenceArray[i][j-i-1][1].resize(0);

            packetTimeMaps[i][j-i-1][0].clear();
            packetTimeMaps[i][j-i-1][1].clear();

            actualTimeMaps[i][j-i-1][0].clear();
            actualTimeMaps[i][j-i-1][1].clear();

            int packetCounter = 0;
            unsigned long long startTime = getTimeMilli();
            unsigned long long lastSentTime = startTime;
            bool endProbesFlag = false;
            bool readTimeoutFlag = false;


            // For each combination(pair), send a train of UDP packets.
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
                        // Send the probe packets.
                        unsigned long long sendTime = getTimeMilli();
                        messageString[0] = '0';
                        hostIndex = 0;

                        short int xIndex, yIndex;
                        xIndex = i;
                        yIndex = j-i-1;
                        memcpy(&messageString[1], &hostIndex, sizeof(short int));
                        memcpy(&messageString[1 + sizeof(short int)], &xIndex, sizeof(short int));
                        memcpy(&messageString[1 + 2*sizeof(short int)], &yIndex, sizeof(short int));
                        memcpy(&messageString[1 + 3*sizeof(short int)], &sendTime, sizeof(unsigned long long));
                        rc = sendto(clientSocket, messageString, 1 + 3*sizeof(short int) + sizeof(unsigned long long), flags, 
                                (struct sockaddr *) &remoteServAddr1, 
                                sizeof(remoteServAddr1));
                        if(rc < 0)
                            printf("ERROR sending first udp message\n");
                        packetTimeMaps[i][j-i-1][0][sendTime] = packetCounter;
                        delaySequenceArray[i][j-i-1][0].push_back(-9999);
                        cout<< "TO " << hostList[i] << " :Counter=" << packetCounter << " :SendTime= " << sendTime << endl;

                        sendTime = getTimeMilli();
                        messageString[0] = '0';
                        hostIndex = 1;
                        memcpy(&messageString[1], &hostIndex, sizeof(short int));
                        memcpy(&messageString[1 + sizeof(short int)], &xIndex, sizeof(short int));
                        memcpy(&messageString[1 + 2*sizeof(short int)], &yIndex, sizeof(short int));
                        memcpy(&messageString[1 + 3*sizeof(short int)], &sendTime, sizeof(unsigned long long));
                        rc = sendto(clientSocket, messageString, 1 + 3*sizeof(short int) + sizeof(unsigned long long), flags, 
                                (struct sockaddr *) &remoteServAddr2, 
                                sizeof(remoteServAddr2));

                        if(rc < 0)
                            printf("ERROR sending second udp message\n");
                        packetTimeMaps[i][j-i-1][1][sendTime] = packetCounter;
                        delaySequenceArray[i][j-i-1][1].push_back(-9999);
                        cout<< "TO " << hostList[j] << " :Counter=" << packetCounter << " :SendTime= " << sendTime << endl;

                        lastSentTime = getTimeMilli();

                        while(pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL) != 0);
                        //pcap_dispatch(pcapDescriptor, 10000, pcapCallback, NULL);

                        // Sleep for 99 msec for a 10Hz target probing rate.
                        usleep(targetSleepTime*1000);
                        packetCounter++;
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
        }

    }

    usleep(20000000);
    while(pcap_dispatch(pcapDescriptor, 1, pcapCallback, NULL) != 0);
    short int xIndex, yIndex;
    for (int i = 0; i < numHosts - 1; i++)
    {
        for( int j = i+1; j < numHosts; j++)
        {
            xIndex = i;
            yIndex = j - i - 1;

            string dirPath = outputDirectory + "/" + hostList[i];
            dirPath = dirPath + "/" + hostList[j];
            commandString = "mkdir -p " + dirPath;
            system(commandString.c_str());

            ofstream tmpFileHandle;
            string tmpFilePath = dirPath + "/" + "debug.log";
            tmpFileHandle.open(tmpFilePath.c_str(), std::ios::out);

            for(int k = 0; k < delaySequenceArray[xIndex][yIndex][0].size(); k++)
            {
                tmpFileHandle << delaySequenceArray[xIndex][yIndex][0][k] << " " << delaySequenceArray[xIndex][yIndex][1][k] << "\n";
            }
            tmpFileHandle.close();

            // If we lost some replies/packets, linearly interpolate their delay values.
            int delaySeqLen = delaySequenceArray[xIndex][yIndex][0].size();
            int firstSeenIndex = -1;
            int lastSeenIndex = -1;

            for (int k = 0; k < delaySeqLen; k++)
            {
                if (delaySequenceArray[xIndex][yIndex][0][k] != -9999 && delaySequenceArray[xIndex][yIndex][1][k] != -9999)
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
                    if (delaySequenceArray[xIndex][yIndex][0][k] == -9999)
                    {
                        // Find the number of missing packets in this range.
                        int numMissingPackets = 0;
                        int lastInRange = 0;
                        for (int l = k; l < lastSeenIndex + 1; l++)
                        {
                            if(delaySequenceArray[xIndex][yIndex][0][l] == -9999)
                            {
                                numMissingPackets++;
                            }
                            else
                            {
                                lastInRange = l;
                                break;
                            }
                        }

                        int step = (delaySequenceArray[xIndex][yIndex][0][lastInRange] - delaySequenceArray[xIndex][yIndex][0][k-1])/(numMissingPackets + 1);

                        // Interpolate delays for the missing packets in this range.
                        int y = 0;
                        for (int x = k, y = 1; x < lastInRange; x++, y++)
                            delaySequenceArray[xIndex][yIndex][0][x] = delaySequenceArray[xIndex][yIndex][0][k-1] + y*step ;
                    }
                    if (delaySequenceArray[xIndex][yIndex][1][k] == -9999)
                    {
                        // Find the number of missing packets in this range.
                        int numMissingPackets = 0;
                        int lastInRange = 0;
                        for (int l = k; l < lastSeenIndex + 1; l++)
                        {
                            if(delaySequenceArray[xIndex][yIndex][1][l] == -9999)
                            {
                                numMissingPackets++;
                            }
                            else
                            {
                                lastInRange = l;
                                break;
                            }
                        }

                        int step = (delaySequenceArray[xIndex][yIndex][1][lastInRange] - delaySequenceArray[xIndex][yIndex][1][k-1])/(numMissingPackets + 1);

                        // Interpolate delays for the missing packets in this range.
                        int y = 0;
                        for (int x = k, y = 1; x < lastInRange; x++, y++)
                            delaySequenceArray[xIndex][yIndex][1][x] = delaySequenceArray[xIndex][yIndex][1][k-1] + y*step ;
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
                    outputFileHandle << delaySequenceArray[xIndex][yIndex][0][k] <<  " " << delaySequenceArray[xIndex][yIndex][1][k]  << "\n";
                }
            }
            outputFileHandle.close();
            if (lastSeenIndex == -1)
                cout<< "ERROR: No samples were seen for hosts " << hostList[i] << " " << hostList[j] << endl;
        }
    }
}

