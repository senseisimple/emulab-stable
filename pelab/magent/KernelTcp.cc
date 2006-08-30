// KernelTcp.cc

#include "lib.h"
#include "log.h"
#include "KernelTcp.h"
#include "Command.h"

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
  void handleTcp(struct pcap_pkthdr const * pcapInfo,
                 struct ip const * ipPacket,
                 struct tcphdr const * tcpPacket,
                 unsigned char const * tcpPacketStart,
                 list<Option> & ipOptions);
  bool handleKernel(Connection * conn, struct tcp_info * kernel);
  void parseOptions(unsigned char const * buffer, int size,
                    list<Option> * options);
}

pcap_t * KernelTcp::pcapDescriptor = NULL;
int KernelTcp::pcapfd = -1;

KernelTcp::KernelTcp()
  : state(DISCONNECTED)
  , peersock(-1)
  , sendBufferSize(0)
  , receiveBufferSize(0)
  , maxSegmentSize(0)
  , useNagles(1)
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

void KernelTcp::connect(Order & planet)
{
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
  if (state == DISCONNECTED)
  {
    logWrite(CONNECTION_MODEL,
             "writeMessage() called while disconnected from peer.");
    connect(result.planet);
  }
  if (state == CONNECTED)
  {
    // Create a different random write each time.
    vector<char> buffer;
    buffer.resize(size);
    size_t i = 0;
    for (i = 0; i < buffer.size(); ++i)
    {
      buffer[i] = static_cast<char>(random() & 0xff);
    }
    // Actually write the darn thing.
    int error = send(peersock, & buffer[0], buffer.size(), 0);
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
        result.bufferFull = true;
      }
      else
      {
        logWrite(EXCEPTION, "Failed write to peer: %s", strerror(errno));
      }
      return -1;
    }
    else
    {
      return error;
    }
  }
  result.bufferFull = false;
  result.isConnected = false;
  return -1;
}

bool KernelTcp::isConnected(void)
{
  return state == CONNECTED;
}

int KernelTcp::getSock(void) const
{
  return peersock;
}

enum { SNIFF_WAIT = 10 };

void KernelTcp::init(void)
{
  // Set up the peerAccept socket
  global::peerAccept = createServer(global::peerServerPort,
                                    "Peer accept socket (No incoming peer "
                                    "connections will be accepted)");
  logWrite(PEER_CYCLE, "Created peer server on port %d",
           global::peerServerPort);

  // Set up the connectionModelExemplar
  global::connectionModelExemplar.reset(new KernelTcp());

  if (global::replayArg != REPLAY_LOAD) {
    // Set up packet capture
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;      /* hold compiled program     */
    bpf_u_int32 maskp;          /* subnet mask               */
    bpf_u_int32 netp;           /* ip                        */
    ostringstream filter;

    /* ask pcap for the network address and mask of the device */
    pcap_lookupnet(global::interface.c_str(), &netp, &maskp, errbuf);
    filter << "port " << global::peerServerPort << " and tcp";

    /* open device for reading.
     * NOTE: We use non-promiscuous */
    pcapDescriptor = pcap_open_live(global::interface.c_str(), BUFSIZ, 0,
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
    logWrite(PCAP, "Captured a packet");
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
    if (ipPacket->ip_p != IPPROTO_TCP)
    {
      logWrite(ERROR, "A non TCP packet was captured");
      return;
    }

    list<Option> ipOptions;
    unsigned char const * optionsBegin = packet + sizeof(struct ether_header)
      + sizeof(struct ip);
    int optionsSize = ipHeaderLength*4 - sizeof(struct ip);
    parseOptions(optionsBegin, optionsSize, &ipOptions);

    // ipHeaderLength is multiplied by 4 because it is a
    // length in 4-byte words.
    unsigned char const * tcpPacketStart = packet + sizeof(struct ether_header)
      + ipHeaderLength*4;
    tcpPacket = reinterpret_cast<struct tcphdr const *>(tcpPacketStart);
    bytesRemaining -= ipHeaderLength*4;
    if (bytesRemaining < sizeof(struct tcphdr))
    {
      logWrite(ERROR, "A captured packet was to short to contain "
               "a TCP header");
      return;
    }
    handleTcp(pcapInfo, ipPacket, tcpPacket, tcpPacketStart, ipOptions);
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

  void handleTcp(struct pcap_pkthdr const * pcapInfo,
                 struct ip const * ipPacket,
                 struct tcphdr const * tcpPacket,
                 unsigned char const * tcpPacketStart,
                 list<Option> & ipOptions)
  {
    logWrite(PCAP, "Captured a TCP packet");
    struct tcp_info kernelInfo;

    list<Option> tcpOptions;
    unsigned char const * tcpOptionsBegin = tcpPacketStart
      + sizeof(struct tcphdr);
    int tcpOptionsSize = tcpPacket->doff*4 - sizeof(struct tcphdr);
    parseOptions(tcpOptionsBegin, tcpOptionsSize, &tcpOptions);

    PacketInfo packet;
    packet.packetTime = Time(pcapInfo->ts);
    packet.packetLength = pcapInfo->len;
    packet.kernel = &kernelInfo;
    packet.ip = ipPacket;
    packet.ipOptions = &ipOptions;
    packet.tcp = tcpPacket;
    packet.tcpOptions = &tcpOptions;

    /*
     * Classify this packet as outgoing or incoming, by trying to look it up
     * with the the local and remote ports in the same order as they appear in
     * the packet, and reversed.
     * NOTE: We put the destination IP address in the planetMap, so we
     * reverse the sense of ip_dst and ip_src
     */
    Order key;
    key.transport = TCP_CONNECTION;
    key.ip = ntohl(ipPacket->ip_dst.s_addr);
    key.localPort = ntohs(tcpPacket->source);
    key.remotePort = ntohs(tcpPacket->dest);

    bool outgoing;

    logWrite(PCAP,"Looking up key (outgoing): i=%s,lp=%i,rp=%i",
        inet_ntoa(ipPacket->ip_dst),key.localPort,key.remotePort);
    map<Order, Connection *>::iterator pos;
    pos = global::planetMap.find(key);

    if (pos != global::planetMap.end()) {
      outgoing = true;
    } else {
      key.ip = ntohl(ipPacket->ip_src.s_addr);
      key.localPort = ntohs(tcpPacket->dest);
      key.remotePort = ntohs(tcpPacket->source);
      logWrite(PCAP,"Looking up key (incoming): i=%s,lp=%i,rp=%i",
          inet_ntoa(ipPacket->ip_src),key.localPort,key.remotePort);
      pos = global::planetMap.find(key);
      if (pos != global::planetMap.end()) {
        outgoing = false;
      } else {
        logWrite(ERROR,"Unable to look find packet in planetMap");
        return;
      }
    }

    /*
     * Next, determine if this packet is an ACK (note that it can also be a
     * data packet, even if it's an ACK. This is called piggybacking)
     */
    bool isAck = (tcpPacket->ack & 0x0001);

    /*
     * Finally, figure out if this packet contains any data
     */
    bool hasData;
    uint32_t totalHeaderLength = ipPacket->ip_hl * 4 + tcpPacket->doff * 4;
    uint32_t totalIPLen = ntohs(ipPacket->ip_len);
    if (totalHeaderLength == totalIPLen) {
      hasData = false;
    } else {
      if (totalHeaderLength < totalIPLen) {
        hasData = true;
      } else {
        logWrite(ERROR,"Internal error in packet data size computation: hl=%i, il=%i",
            totalHeaderLength, totalIPLen);
      }
    }

    /*
     * Now that we've classified this packet, do something about it
     */
    logWrite(PCAP,"Packet classified: outgoing=%i, isAck=%i, hasData=%i",
        outgoing, isAck, hasData);
    if (!handleKernel(pos->second, &kernelInfo)) {
      logWrite(ERROR,"handleKernel() failed");
      return;
    }

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
