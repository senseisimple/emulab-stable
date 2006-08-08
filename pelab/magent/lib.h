// lib.h

#ifndef LIB_H_PELAB_2
#define LIB_H_PELAB_2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netdb.h>
#include <math.h>
#include <pcap.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <utility>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <string>

#include "Time.h"

enum
{
  NAGLES_ALGORITHM,
  SEND_BUFFER,
  RECEIVE_BUFFER
};

enum
{
  EVENT_TO_MONITOR = 0
};

enum TransportProtocol
{
  TCP_CONNECTION = 0,
  UDP_CONNECTION = 1
};

enum SensorType
{
  NULL_SENSOR
};

struct Order
{
  TransportProtocol transport;
  unsigned int ip;
  unsigned short localPort;
  unsigned short remotePort;

  Order()
  {
    transport = TCP_CONNECTION;
    ip = 0;
    localPort = 0;
    remotePort = 0;
  }
  bool operator<(Order const & right) const
  {
    return std::make_pair(transport,
                          std::make_pair(ip,
                                         std::make_pair(localPort,
                                                        remotePort)))
      < std::make_pair(right.transport,
                       std::make_pair(right.ip,
                                      std::make_pair(right.localPort,
                                                     right.remotePort)));
  }
  bool operator==(Order const & right) const
  {
    return transport == right.transport
      && ip == right.ip
      && localPort == right.localPort
      && remotePort == right.remotePort;
  }
  bool operator!=(Order const & right) const
  {
    return !(*this == right);
  }
};
/*
struct PlanetOrder
{
  unsigned long localPort;

  bool operator<(PlanetOrder const & right) const
  {
    return localPort < right.localPort;
  }
};
*/
struct WriteResult
{
  bool isConnected;
  bool bufferFull;
  Order planet;
  Time nextWrite;
};

struct PacketInfo
{
  Time packetTime;
  int packetLength;
  struct tcp_info const * kernel;
  IpHeader const * ip;
  struct tcphdr const * tcp;
  Order elab;
  bool bufferFull;
};

enum
{
  FORWARD_EDGE = 0,
  BACKWARD_EDGE
};

enum
{
  CONNECTION_SEND_BUFFER_SIZE = 0,
  CONNECTION_RECEIVE_BUFFER_SIZE,
  CONNECTION_MAX_SEGMENT_SIZE,
  CONNECTION_USE_NAGLES
};

enum
{
  CONNECTION_MODEL_NULL = 0,
  CONNECTION_MODEL_KERNEL = 1
};

class ConnectionModel;
class Connection;
class CommandInput;
class CommandOutput;

namespace global
{
  extern int connectionModelArg;
  extern unsigned short peerServerPort;
  extern unsigned short monitorServerPort;

  extern int peerAccept;
  extern std::auto_ptr<ConnectionModel> connectionModelExemplar;
  extern std::string interface;
  // file descriptor, IP address string
  extern std::list< std::pair<int, std::string> > peers;

  extern std::map<Order, Connection> connections;
  // A connection is in this map only if it is currently connected.
  extern std::map<Order, Connection *> planetMap;

  extern fd_set readers;
  extern int maxReader;

  extern std::auto_ptr<CommandInput> input;
  extern std::auto_ptr<CommandOutput> output;
}

void setDescriptor(int fd);
void clearDescriptor(int fd);
std::string ipToString(unsigned int ip);

#endif
