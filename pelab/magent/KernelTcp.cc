/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

// KernelTcp.cc

#include "lib.h"
#include "log.h"
#include "KernelTcp.h"
#include "Command.h"

#include "TSThroughputSensor.h"

using namespace std;

namespace
{
  bool changeSocket(int sockfd, int level, int optname, int value,
                    string optstring);
  void kernelTcpCallback(unsigned char *,
                         struct pcap_pkthdr const * pcapInfo,
                         unsigned char const * packet);
  int getLinkLayer(struct pcap_pkthdr const * pcapInfo,
                   unsigned char const * packet);
  void handleTransport(unsigned char transport,
                       struct pcap_pkthdr const * pcapInfo,
                       struct ip const * ipPacket,
                       struct tcphdr const * tcpPacket,
                       struct udphdr const * udpPacket,
                       unsigned char const * transportPacketStart,
                       list<Option> & ipOptions,
                       int bytesRemaining);
  bool handleKernel(Connection * conn, struct tcp_info * kernel);
  void parseOptions(unsigned char const * buffer, int size,
                    list<Option> * options);
}

pcap_t * KernelTcp::pcapDescriptor = NULL;
int KernelTcp::pcapfd = -1;
char  KernelTcp::udpPacketBuffer[66000] = "";
struct pcap_stat pcapStats;
unsigned int currentPcapLoss = 0;

KernelTcp::KernelTcp()
  : state(DISCONNECTED)
  , peersock(-1)
  , sendBufferSize(0)
  , receiveBufferSize(0)
  , maxSegmentSize(0)
  , useNagles(1)
  , debugPrevTime(getCurrentTime())
  , debugByteCount(0)
{
}

KernelTcp::~KernelTcp()
{
  if (peersock != -1)
  {
    close(peersock);
  }
}

auto_ptr<ConnectionModel> KernelTcp::clone(void)
{
  auto_ptr<KernelTcp> result(new KernelTcp());
  result->sendBufferSize = sendBufferSize;
  result->receiveBufferSize = receiveBufferSize;
  result->maxSegmentSize = maxSegmentSize;
  result->useNagles = useNagles;
  result->state = state;
  auto_ptr<ConnectionModel> modelResult(result.release());
  return modelResult;
}

void KernelTcp::connect(PlanetOrder & planet)
{
  if (planet.transport == UDP_CONNECTION)
  {
    // PRAMOD: Insert any additional setup code for UDP connections here.
    state = CONNECTED;
    // Udp - CHANGES - Begin
    udpCurSeqNum = 0;

    // Create a datagram socket, for use by later writeUdpMessage calls.
    if(peersock == -1)
    {
        peersock = socket(AF_INET, SOCK_DGRAM, 0);

        if(peersock < 0)
        {
            logWrite(ERROR, "Could not create a datagram socket in KernelTcp::connect");
        }

        int flags = fcntl(peersock, F_GETFL, 0);

        flags = flags | O_NONBLOCK;

        if( fcntl(peersock, F_SETFL, flags) < 0 )
        {
                logWrite(ERROR,"Could not set UDP socket to non-blocking");

        }

        udpLocalAddr.sin_family = AF_INET;
        udpLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        udpLocalAddr.sin_port = htons(0);

    }
    // Udp - CHANGES - End
  }
  if (state == DISCONNECTED)
  {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
      logWrite(EXCEPTION, "Cannot create a peer socket: %s", strerror(errno));
      return;
    }

    // Set up all parameters
    if ((sendBufferSize != 0 && !changeSocket(sockfd, SOL_SOCKET, SO_SNDBUF,
                                              sendBufferSize, "SO_SNDBUF"))
        || (receiveBufferSize != 0 && !changeSocket(sockfd, SOL_SOCKET,
                                                    SO_RCVBUF,
                                                    receiveBufferSize,
                                                    "SO_RCVBUF"))
        || !changeSocket(sockfd, IPPROTO_TCP, TCP_NODELAY, !useNagles,
                         "TCP_NODELAY"))
    {
      // Error message already printed
      close(sockfd);
      return;
    }


    struct sockaddr_in destAddress;
    destAddress.sin_family = AF_INET;
    destAddress.sin_port = htons(global::peerServerPort);
    destAddress.sin_addr.s_addr = htonl(planet.ip);
    int error = ::connect(sockfd, (struct sockaddr *)&destAddress,
                          sizeof(destAddress));
    if (error == -1)
    {
      logWrite(EXCEPTION, "Cannot connect to peer: %s", strerror(errno));
      close(sockfd);
      return;
    }

    if (maxSegmentSize != 0 && !changeSocket(sockfd, IPPROTO_TCP,
                                             TCP_MAXSEG, maxSegmentSize,
                                             "TCP_MAXSEG"))
    {
      close(sockfd);
      return;
    }


    int flags = fcntl(sockfd, F_GETFL);
    if (flags == -1)
    {
      logWrite(EXCEPTION, "Cannot get fcntl flags from a peer socket: %s",
               strerror(errno));
      close(sockfd);
      return;
    }

    error = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (error == -1)
    {
      logWrite(EXCEPTION, "Cannot set fcntl flags (nonblocking) "
               "on a peer socket: %s", strerror(errno));
      close(sockfd);
      return;
    }

    struct sockaddr_in sourceAddress;
    socklen_t len = sizeof(sourceAddress);
    error = getsockname(sockfd, (struct sockaddr *)&sourceAddress,
                        &len);
    if (error == -1)
    {
      logWrite(EXCEPTION, "Cannot find the source address for a peer: %s");
      close(sockfd);
      return;
    }
    planet.localPort = ntohs(sourceAddress.sin_port);

    logWrite(CONNECTION_MODEL, "Connected to peer at %s:%d",
             ipToString(htonl(planet.ip)).c_str(), global::peerServerPort);
    peersock = sockfd;
    state = CONNECTED;
  }
}

void KernelTcp::addParam(ConnectionModelCommand const & param)
{
  if (state == CONNECTED)
  {
    logWrite(ERROR, "A ConnectionModelCommand was received after connection. "
             "It will not be applied unless the connection dies and is "
             "re-established.");
  }
  switch (param.type)
  {
  case CONNECTION_SEND_BUFFER_SIZE:
    sendBufferSize = param.value;
    break;
  case CONNECTION_RECEIVE_BUFFER_SIZE:
    receiveBufferSize = param.value;
    break;
  case CONNECTION_MAX_SEGMENT_SIZE:
    maxSegmentSize = param.value;
    break;
  case CONNECTION_USE_NAGLES:
    useNagles = param.value;
    break;
  default:
    logWrite(ERROR, "Invalid ConnectionModelCommand type: %d", param.type);
  }
}

int KernelTcp::writeMessage(int size, WriteResult & result)
{
  int retval = 0;
  switch (result.planet.transport)
  {
  case TCP_CONNECTION:
    retval = writeTcpMessage(size, result);
    break;
  case UDP_CONNECTION:
    retval = writeUdpMessage(size, result);
    break;
  default:
    logWrite(ERROR, "Failed to write message because of unknown transport "
             "protocol %d", result.planet.transport);
    break;
  }
  return retval;
}

int KernelTcp::writeTcpMessage(int size, WriteResult & result)
{
  // MAX_WRITESIZE is in chars, and must be a multiple of 4
  static const int MAX_WRITESIZE = 8192;

  if (state == DISCONNECTED)
  {
    logWrite(CONNECTION_MODEL,
             "writeMessage() called while disconnected from peer.");
    connect(result.planet);
  }
  if (state == CONNECTED)
  {
    /*
     * Create a different random write each time.
     * We do this in terms of full words rather than chars because this is a
     * performance-critical piece of code.
     * XXX: What I really want here is a compile-time assert, not a runtime
     * assert...
     */
    assert(MAX_WRITESIZE % 4 == 0);
    uint32_t buffer[MAX_WRITESIZE / 4];
    int bufferwords = min(MAX_WRITESIZE / 4, size / 4);
    for (int i = 0; i < bufferwords; ++i)
    {
      buffer[i] = random();
    }

    /*
     * We only have MAX_WRITESIZE bytes of random data - therefore, we may have
     * to do more than one write if they asked for a bigger write than
     * MAX_WRITESIZE
     */
    int bytesToWrite;
    int bytesWritten = 0;
    result.bufferFull = false;
    for (bytesToWrite = size; bytesToWrite > 0; bytesToWrite -= MAX_WRITESIZE) {
      int writeSize = min(MAX_WRITESIZE,bytesToWrite);
      // Actually write the darn thing.
      logWrite(CONNECTION_MODEL, "About to write %d bytes", writeSize);
      int error = send(peersock, &buffer[0], writeSize, 0);
      logWrite(CONNECTION_MODEL,
               "Tried to write %d bytes, actually wrote %d", writeSize, error);

      if (error == 0)
      {
        close(peersock);
        peersock = -1;
        state = DISCONNECTED;
        result.bufferFull = false;
        result.isConnected = false;
        return 0;
      }
      else if (error == -1)
      {
        if (errno == EWOULDBLOCK)
        {
          logWrite(CONNECTION_MODEL,"Buffer is full");
          result.bufferFull = true;

          Time currentTime = getCurrentTime();
          uint32_t timeTotal = (currentTime - debugPrevTime).toMilliseconds();
          if (debugByteCount > 0 && timeTotal > 0)
          {
            logWrite(CONNECTION_MODEL,
                     "Load estimate: %d kbps, %d kilobits, %d millis",
                     TSThroughputSensor::getThroughputInKbps(timeTotal,
                                                             debugByteCount),
                     debugByteCount, timeTotal);
            debugPrevTime = currentTime;
            debugByteCount = 0;
          }

          // XXX This doesn't make sense. We know that error is -1.
          // No point in trying more writes
          return bytesWritten + error;
        }
        else
        {
          logWrite(EXCEPTION, "Failed write to peer: %s", strerror(errno));
        }
        return -1;
      }
      else
      {
        debugByteCount += error;
        // Write succeeded, all bytes got written
        // XXX Shouldn't this be error?
        bytesWritten += bytesToWrite;
      }
    }

    // Return the total number of bytes written
    return bytesWritten;

  } else {
    result.bufferFull = false;
    result.isConnected = false;
    return -1;
  }
}

int KernelTcp::writeUdpMessage(int size, WriteResult & result)
{
  // PRAMOD: Replace the following call with the UDP write code.
  // result.planet.ip and result.planet.remotePort denote the destination.
  // result.planet.ip is in host order
  // result.planet.remotePort is in host order
  // put the localport in host order in result.planet.localPort
  // set result.bufferFull to false (UDP 'buffers' are never full)
  // set result.isConnected to true (UDP 'connections' are always connected)

  if(size < global::udpMinSendPacketSize)
    size = global::udpMinSendPacketSize;

  // Indicate to the server the version of the format in which our data packets are.
  unsigned char serverVersion = 2;
  memcpy(&KernelTcp::udpPacketBuffer[0], &serverVersion, sizeof(unsigned char));

  // Put the sequence number of the packet.
  unsigned short int networkOrder_udpCurSeqNum = htons(udpCurSeqNum);
  memcpy(&KernelTcp::udpPacketBuffer[1],&networkOrder_udpCurSeqNum, global::USHORT_INT_SIZE);

  // Copy the size of the packet.. This can be used
  // by the sensors in case they miss this packet
  // because of libpcap buffer overflow.
  // The size of the packet & its timestamp at the
  // sender are echoed in the ACKs.

  unsigned short networkOrder_size = (size);
  memcpy(&KernelTcp::udpPacketBuffer[1 + global::USHORT_INT_SIZE],&networkOrder_size, global::USHORT_INT_SIZE);

  // Copy the timestamp of when this packet is being sent.

  // This timestamp will not be as accurate as
  // the one captured by libpcap, but can be
  // used as a fallback option in case we miss
  // this packet because of a libpcap buffer overflow.
  unsigned long long curTime = getCurrentTime().toMicroseconds();
  curTime = (curTime);
  memcpy(&KernelTcp::udpPacketBuffer[1 + 2*global::USHORT_INT_SIZE], &curTime, global::ULONG_LONG_SIZE);

  bool socketConnectedFlag = true;

  if(peersock == -1)
  {
          socketConnectedFlag = false;
          logWrite(EXCEPTION, "udpWriteMessage called in KernelTcp before connect()ing the socket");

          peersock = socket(AF_INET, SOCK_DGRAM, 0);

          if(peersock < 0)
          {
                  logWrite(ERROR, "Could not create a datagram socket in writeUdpMessage");
                  return -1;
          }

          int flags = fcntl(peersock, F_GETFL, 0);

          flags = flags | O_NONBLOCK;

          if( fcntl(peersock, F_SETFL, flags) < 0 )
          {
                  logWrite(ERROR,"Could not set UDP socket to non-blocking");
          }
    }
  // result.planet.ip and result.planet.remotePort denote the destination.
  // result.planet.ip is in host order
  // result.planet.remotePort is in host order
  struct sockaddr_in remoteServAddr;
  remoteServAddr.sin_family = AF_INET;
  remoteServAddr.sin_addr.s_addr = htonl(result.planet.ip);
  remoteServAddr.sin_port = htons(result.planet.remotePort);

  int flags = 0;
  int bytesWritten = sendto(peersock, KernelTcp::udpPacketBuffer, size, flags,
  (struct sockaddr *) &remoteServAddr,
  sizeof(remoteServAddr));

  // If this is the first sendto call, then get the port number
  // which was assigned to the socket.
  if(!socketConnectedFlag || ntohs(udpLocalAddr.sin_port) == 0 )
  {
          socklen_t udpLocalLen = sizeof(udpLocalAddr);
          getsockname(peersock, (sockaddr *)&udpLocalAddr, &udpLocalLen);
  }

  // put the localport in host order in result.planet.localPort
  // set result.bufferFull to false (UDP 'buffers' are never full)
  // set result.isConnected to true (UDP 'connections' are always connected)
  result.planet.localPort = ntohs(udpLocalAddr.sin_port);
  result.bufferFull = false;
  result.isConnected = true;

  if(bytesWritten < 0)
  {
          if(errno == EWOULDBLOCK)
          {
                  logWrite(EXCEPTION,"UDP sendto returned EWOULDBLOCK");
          }
          else
          {
              logWrite(EXCEPTION, "Error writing to UDP socket in KernelTcp, errno = %d",errno);
              return -1;
          }
  }

  udpCurSeqNum++;

  return bytesWritten;
}

bool KernelTcp::isConnected(void)
{
  return state == CONNECTED;
}

int KernelTcp::getSock(void) const
{
  return peersock;
}

namespace
{
  void addFilterProtocol(ostringstream & filter, char const * proto,
                         unsigned short port, char const * address)
  {
    filter << "(" << proto << " and ("
           << "(src port " << port << " and dst host " << address << ")"
           << " or "
           << "(dst port " << port << " and src host " << address << ")"
           << "))";
  }

  enum { SNIFF_WAIT = 10 };
}

void KernelTcp::init(void)
{
  // Set up the peerAccept socket
  if (global::isPeerServer) {
    global::peerAccept = createServer(global::peerServerPort,
                                      "Peer accept socket (No incoming peer "
                                      "connections will be accepted)");
    logWrite(PEER_CYCLE, "Created peer server on port %d",
             global::peerServerPort);
  }

  // Set up the connectionModelExemplar
  global::connectionModelExemplar.reset(new KernelTcp());

  if (global::replayArg != REPLAY_LOAD) {
    // Set up packet capture
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    ostringstream filter;
    struct in_addr tempAddr;

    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(global::interface.c_str(), &netp, &maskp, errbuf);
    tempAddr.s_addr = netp;

    char *localIPAddr = inet_ntoa(tempAddr);
    addFilterProtocol(filter, "tcp", global::peerServerPort, localIPAddr);
    filter << " or ";
    addFilterProtocol(filter, "udp", global::peerUdpServerPort, localIPAddr);

    /* open device for reading.
     * NOTE: We use non-promiscuous */
    pcapDescriptor = pcap_open_live(global::interface.c_str(), SNAPLEN_SIZE, 0,
                                    SNIFF_WAIT, errbuf);
    if(pcapDescriptor == NULL)
    {
      logWrite(ERROR, "pcap_open_live() failed: %s", errbuf);
    }
    // Lets try and compile the program, optimized
    else if(pcap_compile(pcapDescriptor, &fp,
                         const_cast<char *>(filter.str().c_str()),
                         1, maskp) == -1)
    {
      logWrite(ERROR, "pcap_compile() failed: %s", pcap_geterr(pcapDescriptor));
      pcap_close(pcapDescriptor);
    }
    // set the compiled program as the filter
    else if(pcap_setfilter(pcapDescriptor,&fp) == -1)
    {
      logWrite(ERROR, "pcap_filter() failed: %s", pcap_geterr(pcapDescriptor));
      pcap_close(pcapDescriptor);
    }
    else
    {
      pcapfd = pcap_get_selectable_fd(pcapDescriptor);
      if (pcapfd == -1)
      {
        logWrite(ERROR, "Failed to get a selectable file descriptor "
                 "for pcap: %s", pcap_geterr(pcapDescriptor));
        pcap_close(pcapDescriptor);
      }
      else
      {
        setDescriptor(pcapfd);
      }
    }
  }
}

void KernelTcp::addNewPeer(fd_set * readable)
{
  if (global::peerAccept != -1
      && FD_ISSET(global::peerAccept, readable))
  {
    struct sockaddr_in remoteAddress;
    int fd = acceptServer(global::peerAccept, &remoteAddress,
                          "Peer socket (Incoming peer connection was not "
                          "accepted)");
    if (fd != -1)
    {
      global::peers.push_back(make_pair(fd, ipToString(
        remoteAddress.sin_addr.s_addr)));
      logWrite(PEER_CYCLE,
               "Peer connection %d from %s was accepted normally.",
               global::peers.back().first,
               global::peers.back().second.c_str());
    }
  }
}

void KernelTcp::readFromPeers(fd_set * readable)
{
  list< pair<int, string> >::iterator pos = global::peers.begin();
  while (pos != global::peers.end())
  {
    if (pos->first != -1 && FD_ISSET(pos->first, readable))
    {
      static const int bufferSize = 8096;
      static char buffer[bufferSize];
      int size = read(pos->first, buffer, bufferSize);
      if (size == 0)
      {
        logWrite(PEER_CYCLE,
                 "Peer connection %d from %s is closing normally.",
                 pos->first, pos->second.c_str());
        close(pos->first);
        clearDescriptor(pos->first);
        list< pair<int, string> >::iterator temp = pos;
        ++pos;
        global::peers.erase(temp);
      }
      else if (size == -1 && errno != EAGAIN && errno != EINTR)
      {
        logWrite(EXCEPTION,
                 "Failed to read peer connection %d from %s so "
                 "I'm shutting it down: %s", pos->first, pos->second.c_str(),
                 strerror(errno));
        close(pos->first);
        list< pair<int, string> >::iterator temp = pos;
        ++pos;
        global::peers.erase(temp);
      }
      else
      {
        ++pos;
      }
    }
    else
    {
      ++pos;
    }
  }
}

void KernelTcp::packetCapture(fd_set * readable)
{
  unsigned char * args = NULL;
  if (pcapfd != -1 && FD_ISSET(pcapfd, readable))
  {
    pcap_dispatch(pcapDescriptor, 1, kernelTcpCallback, args);
  }
}

namespace
{
  bool changeSocket(int sockfd, int level, int optname, int value,
                    string optstring)
  {
    int error = setsockopt(sockfd, SOL_SOCKET, optname, &value, sizeof(value));
    if (error == -1)
    {
      logWrite(ERROR, "Cannot set socket option %s", optstring.c_str());
      return false;
    }
    int newValue = 0;
    socklen_t newValueLength = sizeof(newValue);
    error = getsockopt(sockfd, level, optname, &newValue,
                       &newValueLength);
    if (error == -1)
    {
      logWrite(ERROR, "Cannot read back socket option %s", optstring.c_str());
      return false;
    }
    logWrite(CONNECTION_MODEL, "Socket option %s is now %d", optstring.c_str(),
             newValue);
    return true;
  }

  void kernelTcpCallback(unsigned char *,
                         struct pcap_pkthdr const * pcapInfo,
                         unsigned char const * packet)
  {
//    logWrite(PCAP, "Captured a packet");
    int packetType = getLinkLayer(pcapInfo, packet);
    if (packetType == -1)
    {
      // Error message already printed in getLinkLayer();
      return;
    }
    if (packetType != ETHERTYPE_IP)
    {
      logWrite(ERROR, "Unknown link layer type: %d", packetType);
      return;
    }
    struct ip const * ipPacket;
    struct tcphdr const * tcpPacket;
    struct udphdr const * udpPacket;
    size_t bytesRemaining = pcapInfo->caplen - sizeof(struct ether_header);

    ipPacket = reinterpret_cast<struct ip const *>
      (packet + sizeof(struct ether_header));
    if (bytesRemaining < sizeof(struct ip))
    {
      logWrite(ERROR, "A captured packet was too short to contain an "
               "IP header");
      return;
    }
    // ipHeaderLength and version are in a one byte field so
    // endian-ness doesn't matter.
    int ipHeaderLength = ipPacket->ip_hl;
    int version = ipPacket->ip_v;
    if (version != 4)
    {
      logWrite(ERROR, "A non IPv4 packet was captured");
      return;
    }
    if (ipHeaderLength < 5)
    {
      logWrite(ERROR, "Bad IP header length: %d", ipHeaderLength);
      return;
    }

    list<Option> ipOptions;
    unsigned char const * optionsBegin = packet + sizeof(struct ether_header)
      + sizeof(struct ip);
    int optionsSize = ipHeaderLength*4 - sizeof(struct ip);
    parseOptions(optionsBegin, optionsSize, &ipOptions);

    // ipHeaderLength is multiplied by 4 because it is a
    // length in 4-byte words.
    unsigned char const * transportPacketStart = packet
        + sizeof(struct ether_header)
        + ipHeaderLength*4;
    bytesRemaining -= ipHeaderLength*4;
    switch (ipPacket->ip_p)
    {
    case IPPROTO_TCP:
      tcpPacket
        = reinterpret_cast<struct tcphdr const *>(transportPacketStart);
      if (bytesRemaining < sizeof(struct tcphdr))
      {
        logWrite(ERROR, "A captured packet was to short to contain "
                 "a TCP header");
      }
      else
      {
//        logWrite(PCAP, "Captured a TCP packet");
        bytesRemaining -= tcpPacket->doff*4;
        handleTransport(TCP_CONNECTION, pcapInfo, ipPacket, tcpPacket, NULL,
                        transportPacketStart, ipOptions, bytesRemaining);
      }
      break;
    case IPPROTO_UDP:
      udpPacket
        = reinterpret_cast<struct udphdr const *>(transportPacketStart);
      if (bytesRemaining < sizeof(struct udphdr))
      {
        logWrite(ERROR, "A captured packet was to short to contain "
                 "a UDP header");
      }
      else
      {
//        logWrite(PCAP, "Captured a UDP packet");
        bytesRemaining -= sizeof(struct udphdr);
        handleTransport(UDP_CONNECTION, pcapInfo, ipPacket, NULL, udpPacket,
                        transportPacketStart, ipOptions, bytesRemaining);
      }
      break;
    default:
      logWrite(ERROR, "I captured a packet, "
               "but don't know the transport protocol: %d", ipPacket->ip_p);
      break;
    }
  }

  int getLinkLayer(struct pcap_pkthdr const * pcapInfo,
                   unsigned char const * packet)
  {
    unsigned int caplen = pcapInfo->caplen;

    if (caplen < sizeof(struct ether_header))
    {
      logWrite(ERROR, "A captured packet was too short to contain "
               "an ethernet header");
      return -1;
    }
    else
    {
      struct ether_header * etherPacket = (struct ether_header *) packet;
      return ntohs(etherPacket->ether_type);
    }
  }

  void handleTransport(unsigned char transport,
                       struct pcap_pkthdr const * pcapInfo,
                       struct ip const * ipPacket,
                       struct tcphdr const * tcpPacket,
                       struct udphdr const * udpPacket,
                       unsigned char const * transportPacketStart,
                       list<Option> & ipOptions,
                       int bytesRemaining)
  {
    struct tcp_info kernelInfo;

    list<Option> tcpOptions;
    unsigned char const * payload = NULL;
    if (transport == TCP_CONNECTION)
    {
      unsigned char const * tcpOptionsBegin = transportPacketStart
        + sizeof(struct tcphdr);
      int tcpOptionsSize = tcpPacket->doff*4 - sizeof(struct tcphdr);
      if (bytesRemaining >= 0)
      {
        parseOptions(tcpOptionsBegin, tcpOptionsSize, &tcpOptions);
      }
      else
      {
        logWrite(ERROR, "TCP Packet too short to parse TCP options");
      }
      payload = transportPacketStart + tcpPacket->doff*4;
    }
    else if (transport == UDP_CONNECTION)
    {
      payload = transportPacketStart + sizeof(struct udphdr);
    }

    PacketInfo packet;
    packet.transport = transport;
    packet.packetTime = Time(pcapInfo->ts);
    packet.packetLength = pcapInfo->len;
    if (transport == TCP_CONNECTION)
    {
      packet.kernel = &kernelInfo;
      packet.tcp = tcpPacket;
      packet.tcpOptions = &tcpOptions;
    }
    else
    {
      packet.kernel = NULL;
      packet.tcp = NULL;
      packet.tcpOptions = NULL;
    }
    packet.ip = ipPacket;
    packet.ipOptions = &ipOptions;
    if (transport == UDP_CONNECTION)
    {
      packet.udp = udpPacket;
    }
    else
    {
      packet.udp = NULL;
    }
    if (bytesRemaining <= 0)
    {
      packet.payloadSize = 0;
      packet.payload = NULL;
    }
    else
    {
      packet.payloadSize = bytesRemaining;
      packet.payload = payload;
    }

    /*
     * Classify this packet as outgoing or incoming, by trying to look it up
     * with the the local and remote ports in the same order as they appear in
     * the packet, and reversed.
     * NOTE: We put the destination IP address in the planetMap, so we
     * reverse the sense of ip_dst and ip_src
     */
    PlanetOrder key;
    key.transport = transport;
    key.ip = ntohl(ipPacket->ip_dst.s_addr);
    if (transport == TCP_CONNECTION)
    {
      key.localPort = ntohs(tcpPacket->source);
      key.remotePort = ntohs(tcpPacket->dest);
    }
    else if (transport == UDP_CONNECTION)
    {
      key.localPort = ntohs(udpPacket->source);
      key.remotePort = ntohs(udpPacket->dest);
    }

    bool outgoing;

//    logWrite(PCAP,"Looking up key (outgoing): t=%d,i=%s,lp=%i,rp=%i",
//        transport, inet_ntoa(ipPacket->ip_dst),key.localPort,key.remotePort);
//    logWrite(PCAP, "Looking up key (outgoing): %s",
//             key.toString().c_str());
    map<PlanetOrder, Connection *>::iterator pos;
    pos = global::planetMap.find(key);

    if (pos != global::planetMap.end()) {
      outgoing = true;
    } else {
      key.ip = ntohl(ipPacket->ip_src.s_addr);
      swap(key.localPort, key.remotePort);
//      key.localPort = ntohs(tcpPacket->dest);
//      key.remotePort = ntohs(tcpPacket->source);
//      logWrite(PCAP, "Looking up key (incoming): %s",
//               key.toString().c_str());
//      logWrite(PCAP,"Looking up key (incoming): t=%d,i=%s,lp=%i,rp=%i",
//          transport,inet_ntoa(ipPacket->ip_src),key.localPort,key.remotePort);
      pos = global::planetMap.find(key);
      if (pos != global::planetMap.end()) {
        outgoing = false;
      } else {
        logWrite(ERROR,"Unable to find packet in planetMap");
        return;
      }
    }

    if (pos->second == NULL)
    {
      logWrite(EXCEPTION, "Packet captured after the end of a connection.");
      return;
    }

    /*
     * Next, determine if this packet is an ACK (note that it can also be a
     * data packet, even if it's an ACK. Ths is called piggybacking)
     */
    // JD: Not used any more.
//    bool isAck = (tcpPacket->ack & 0x0001);

    /*
     * Finally, figure out if this packet contains any data
     */
//    bool hasData;
//    uint32_t totalHeaderLength = ipPacket->ip_hl * 4 + tcpPacket->doff * 4;
//    uint32_t totalIPLen = ntohs(ipPacket->ip_len);
//    if (totalHeaderLength == totalIPLen) {
//      hasData = false;
//    } else {
//      if (totalHeaderLength < totalIPLen) {
//        hasData = true;
//      } else {
//        logWrite(ERROR,"Internal error in packet data size computation: hl=%i, il=%i",
//            totalHeaderLength, totalIPLen);
//      }
//    }

    /*
     * Now that we've classified this packet, do something about it
     */
//    logWrite(PCAP,"Packet classified: outgoing=%i, isAck=%i, hasData=%i",
//        outgoing, isAck, hasData);

    if (transport == TCP_CONNECTION)
    {
      if (!handleKernel(pos->second, &kernelInfo))
      {
        logWrite(ERROR,"handleKernel() failed");
        return;
      }
    }

    static int counter = 0;
        pcap_stats(KernelTcp::pcapDescriptor, &pcapStats);

        if(counter%2000 == 0)
        {
        if(pcapStats.ps_drop > currentPcapLoss)
        {
                 currentPcapLoss = pcapStats.ps_drop;
                 logWrite(SENSOR,"\nSTAT::Number of packets lost in libpcap = %d\n",currentPcapLoss);
        }
        }
    counter++;

    // We want to distinguish between packets that are outgoing and
    // packets that are incoming. All other separation can be done
    // inside the sensors themselves. We call these 'Send' and 'Ack'
    // packets because my thinking was originally muddied about this.
    if (outgoing) {
      /*
       * Outgoing packets
       */
      packet.packetType = PACKET_INFO_SEND_COMMAND;
      pos->second->capturePacket(&packet);
    } else {
      /*
       * Incoming packets
       */
      packet.packetType = PACKET_INFO_ACK_COMMAND;
      pos->second->capturePacket(&packet);
    }
  }

  bool handleKernel(Connection * conn, struct tcp_info * kernel)
  {
    bool result = true;
    // This is a filthy filthy hack. Basically, I need the fd in order
    // to introspect the kernel for it. But I don't want that part of
    // the main interface because we don't even know that a random
    // connection model *has* a unique fd.
    ConnectionModel const * genericModel = conn->getConnectionModel();
    KernelTcp const * model = dynamic_cast<KernelTcp const *>(genericModel);
    if (model != NULL)
    {
      socklen_t infoSize = sizeof(tcp_info);
      int error = 0;
      error = getsockopt(model->getSock(), SOL_TCP, TCP_INFO, kernel,
                         &infoSize);
      if (error == -1)
      {
        logWrite(ERROR, "Failed to get the kernel TCP info: %s",
                 strerror(errno));
        result = false;
      }
    }
    else
    {
      logWrite(ERROR, "handleKernel() called for KernelTcp, but the "
               "ConnectionModel on the actual connection wasn't of type "
               "KernelTcp. This inconsistency will lead to "
               "undefined/uninitialized behaviour");
      result = false;
    }
  return result;
  }

  void parseOptions(unsigned char const * buffer, int size,
                    list<Option> * options)
  {
//    logWrite(PCAP, "Parsing options");
    unsigned char const * pos = buffer;
    unsigned char const * limit = buffer + size;
    Option current;
    bool done = false;
    while (pos < limit && !done)
    {
      current.type = *pos;
      ++pos;
      if (current.type == 0)
      {
        // This is the ending marker. We're done here.
        current.length = 0;
        current.buffer = NULL;
        options->push_back(current);
        done = true;
      }
      else if (current.type == 1)
      {
        // This is the padding no-op marker. There is no length field.
        current.length = 0;
        current.buffer = NULL;
        options->push_back(current);
      }
      else
      {
        // This is some other code. There is a length field and length-2
        // other bytes.
        current.length = *pos - 2;
        ++pos;

        if (current.length > 0)
        {
          current.buffer = pos;
          pos += current.length;
        }
        else
        {
          current.buffer = NULL;
        }
        options->push_back(current);
      }
    }
//    logWrite(PCAP, "Finished parsing options");
  }
}
