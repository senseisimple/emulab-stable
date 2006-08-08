// KernelTcp.cc

#include "lib.h"
#include "log.h"
#include "KernelTcp.h"

using namespace std;

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
  result.sendBufferSize = sendBufferSize;
  result.receiveBufferSize = receiveBufferSize;
  result.maxSegmentSize = maxSegmentSize;
  result.useNagles = useNagles;
  return result;
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
                                              sendBufferSize,  "SO_SNDBUF"))
        || (receiveBufferSize != 0 && !changeSocket(sockfd, SOL_SOCKET,
                                                    SO_RCVBUF,
                                                    receiveBufferSize,
                                                    "SO_RCVBUF"))
        || !changeSocket(sockfd, IPPROTO_TCP, TCP_NODELAY, !useNagles,
                         "TCP_NODELAY"))
    {
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
    if (error == =1)
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

    peersock = sockfd;
    state = CONNECTED;
  }
}

void KernelTcp::addParam(ConnectionModelCommand const & param)
{
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

bool KernelTcp::changeSocket(int sockfd, int optname, int value,
                             string optstring)
{
  int error = setsockopt(sockfd, SOL_SOCKET, optname, &value, sizeof(value));
  if (error == -1)
  {
    logWrite(ERROR, "Cannot set socket option %s", optstring.c_str());
    return false;
  }
  int newValue = 0;
  int newValueLength = sizeof(newValue);
  error = getsockopt(sockfd, SOL_SOCKET, optname, &newValue, &newValueLength);
  if (error == -1)
  {
    logWrite(ERROR, "Cannot read back socket option %s", optstring.c_str());
    return false;
  }
  logWrite(CONNECTION_MODEL, "Socket option %s is now %d", optstring.c_str(),
           newValue);
  return true;
}

int KernelTcp::writeMessage(int size, WriteResult & result)
{
  if (state == DISCONNECTED)
  {
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
      result.fullBuffer = false;
      result.isConnected = false;
      return 0;
    }
    else if (error == -1)
    {
      logWrite(EXCEPTION, "Failed write to peer: %s", strerror(errno));
      return -1;
    }
    else
    {
      return error;
    }
  }
  result.fullBuffer = false;
  result.isConnected = false;
  return -1;
}

bool KernelTcp::isConnected(void)
{
  return state == CONNECTED;
}

enum { SNIFF_WAIT = 10 };

void KernelTcp::init(void)
{
  int error = 0;
  // Set up the peerAccept socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1)
  {
    logWrite(ERROR, "Unable to generate a peer accept socket. "
             "No incoming peer connections will ever be accepted: %s",
             strerror(errno));
  }
  else
  {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(global::peerServerPort);
    address.sin_addr.s_addr = INADDR_ANY;
    error = bind(sockfd, reinterpret_cast<struct sockaddr *>(&address),
                 sizeof(struct sockaddr));
    if (error == -1)
    {
      logWrite(ERROR, "Unable to bind peer accept socket. "
             "No incoming peer connections will ever be accepted: %s",,
               strerror(errno));
      close(sockfd);
    }
    else
    {
      setDescriptor(sockfd);
      global::peerAccept = sockfd;
    }
  }

  // Set up the connectionModelExemplar
  global::connectionModelExemplar.reset(new KernelTcp());

  // Set up packet capture
  char errorbuf[PCAP_ERRBUF_SIZE];
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
  else if(pcap_compile(pcapDescriptor, &fp, filter, 1, maskp) == -1)
  {
    logWrite(ERROR, "pcap_compile() failed");
  }
  // set the compiled program as the filter
  else if(pcap_setfilter(pcapDescriptor,&fp) == -1)
  {
    logWrite(ERROR, "pcap_filter() failed");
  }
  else
  {
    pcapfd = pcap_get_selectable_fd(pcapDescriptor);
    if (pcapfd == -1)
    {
      logWrite(ERROR, "Failed to get a selectable file descriptor for pcap");
    }
    else
    {
      setDescriptor(pcapfd);
    }
  }
}

void KernelTcp::addNewPeer(fd_set * readable)
{
  if (global::peerAccept != -1
      && FD_ISSET(global::peerAccept, readable))
  {
    struct sockaddr_in remoteAddress;
    socklen_t addressSize = sizeof(remoteAddress);
    int fd = accept(global::peerAccept,
                    reinterpret_cast<struct sockaddr *>(&remoteAddress),
                    &addressSize);
    if (fd != -1)
    {
      // Add the peer.
      int flags = fcntl(fd, F_GETFL);
      if (flags != -1)
      {
        int error = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (error != -1)
        {
          global::peers.push_back(
            make_pair(fd, ipToString(remoteAddress.sin_addr.s_addr)));
          setDescriptor(fd);
          logWrite(PEER_CYCLE,
                   "Peer connection %d from %s was accepted normally.",
                   global::peers.back().first,
                   global::peers.back().second.c_str());
        }
        else
        {
          logWrite(EXCEPTION, "fctl(F_SETFL) failed: %s", strerror(errno));
          close(fd);
        }
      }
      else
      {
        logWrite(EXCEPTION, "fctl(F_GETFL) failed: %s", strerror(errno));
        close(fd);
      }
    }
    else
    {
      logWrite(EXCEPTION, "accept() called on a peer connection failed: %s",
               strerror(errno));
    }
  }
}

void KernelTcp::readFromPeers(fd_set * readable)
{
  list< pair<int, string> >::iterator pos = global::peers.begin();
  while (pos != global::peers.end())
  {
    if (FD_ISSET(pos->first, readable))
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
  if (FD_ISSET(pcapfd, readable))
  {
    pcap_dispatch(descr,-1,kernelTcpCallback,args);
  }
}

struct IpHeader
{
  unsigned char ip_vhl;         /* header length, version */
#define IP_V(ip)        (((ip)->ip_vhl & 0xf0) >> 4)
#define IP_HL(ip)       ((ip)->ip_vhl & 0x0f)
  unsigned char ip_tos;         /* type of service */
  unsigned short ip_len;         /* total length */
  unsigned short ip_id;          /* identification */
  unsigned short ip_off;         /* fragment offset field */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
  unsigned char ip_ttl;         /* time to live */
  unsigned char ip_p;           /* protocol */
  unsigned short ip_sum;         /* checksum */
  struct in_addr ip_src;
  struct in_addr ip_dst;  /* source and dest address */
};


void kernelTcpCallback(unsigned char *,
                       struct pcap_pkthdr const * pcapInfo,
                       unsigned char const * packet)
{
  int packetType = getLinkLayer(pcapInfo, packet);
  if (type == ETHERTYPE_IP)
  {
    IpHeader const * ipPacket;
    struct tcphdr const * tcpPacket;
    int bytesRemaining = pcapInfo->caplen - sizeof(struct ether_header);

    ipPacket = reinterpret_cast<IpHeader const *>
      (packet + sizeof(struct ether_header));
    if (bytesRemaining >= sizeof(IpHeader))
    {
      int ipLength = ipPacket->ip_len;
      int ipHeaderLength = IP_HL(ipPacket);
      int version = IP_V(ipPacket);
      if (version == 4)
      {
        if (ipHeaderLength >= 5)
        {
          if (ipHeader->ip_p == IPPROTO_TCP)
          {
            // ipHeaderLength is multiplied by 4 because it is a
            // length in 4-byte words.
            tcpPacket = reinterpret_cast<struct tcphdr const *>
              (packet + sizeof(struct ether_header)
               + ipHeaderLength*4);
            bytesRemaining -= ipHeaderLength*4;
            if (bytesRemaining >= sizeof(struct tcphdr))
            {
              handleTcp(pcapInfo, ipPacket, tcpPacket);
            }
            else
            {
              logWrite(ERROR, "A captured packet was to short to contain "
                       "a TCP header");
            }
          }
          else
          {
            logWrite(ERROR, "A non TCP packet was captured");
          }
        }
        else
        {
          logWrite(ERROR, "Bad IP header length: %d", ipHeaderLength);
        }
      }
      else
      {
        logWrite(ERROR, "A non IPv4 packet was captured");
      }
    }
    else
    {
      logWrite(ERROR, "A captured packet was too short to contain an "
               "IP header");
    }
  }
  else if (type != -1)
  {
    logWrite(ERROR, "Unknown link layer type: %d", packetType);
  }
}

int getLinkLayer(struct pcap_pkthdr const * pcapInfo,
                 unsigned char const * packet)
{
    unsigned int caplen = pcapInfo->caplen;

    if (caplen < ETHER_HDRLEN)
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
               IpHeader const * ipPacket,
               struct tcphdr const * tcpPacket)
{
  struct tcp_info kernelInfo;
  bool isAck;
  if (tcpPacket->ack & 0x0001)
  {
    isAck = true;
  }
  else
  {
    isAck = false;
  }
  PacketInfo packet;
  packet.packetTime = Time(pcapInfo->ts);
  packet.packetLength = pcapInfo->len;
  packet.kernel = &kernelInfo;
  packet.ip = ipPacket;
  packet.tcp = tcpPacket;

  Order key;
  // Assume that this is an outgoing packet.
  key.transport = TCP_CONNECTION;
  key.ip = ntohl(ipPacket->ip_dest);
  key.localPort = ntohs(tcpPacket->source);
  key.remotePort = ntohs(tcpPacket->dest);

  map<Order, Connection *>::iterator pos;
  pos = global::planetMap.find(key);
  if (pos != global::planetMap.end())
  {
    // This is an outgoing packet.
    if (!isAck)
    {
      // We only care about sent packets, not acked packets.
      handleKernel(pos->second, &kernelInfo);
      pos->second->captureSend(&packet);
    }
  }
  else
  {
    // Assume that this is an incoming packet.
    key.transport = TCP_CONNECTION;
    key.ip = ntohl(ipPacket->ip_source);
    key.localPort = ntohs(tcpPacket->dest);
    key.remotePort = ntohs(tcpPacket->source);

    pos = global::planetMap.find(key);
    if (pos != global::planetMap.end())
    {
      // This is an incoming packet.
      if (isAck)
      {
        // We only care about ack packets, not sent packets.
        handleKernel(pos->second, &kernelInfo);
        pos->second->captureAck(&packet);
      }
    }
  }
}

void handleKernel(Connection * conn, struct tcp_info * kernel)
{
  // This is a filthy filthy hack. Basically, I need the fd in order
  // to introspect the kernel for it. But I don't want that part of
  // the main interface because we don't even know that a random
  // connection model *has* a unique fd.
  ConnectionModel const * genericModel = conn->getConnectionModel();
  KernelTcp const * model = dynamic_cast<KernelTcp const *>(genericModel);
  if (model != NULL)
  {
    int infoSize = sizeof(tcp_info);
    int error = 0;
    error = getsockopt(model->getSock(), SOL_TCP, TCP_INFO, kernel, infoSize);
    if (error == -1)
    {
      logWrite(ERROR, "Failed to get the kernel TCP info: %s",
               strerror(errno));
    }
  }
  else
  {
    logWrite(ERROR, "handleKernel() called for KernelTcp, but the "
             "ConnectionModel on the actual connection wasn't of type "
             "KernelTcp. This inconsistency will lead to "
             "undefined/uninitialized behaviour"
  }
}
