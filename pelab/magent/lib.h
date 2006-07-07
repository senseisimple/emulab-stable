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
    return make_pair(transport,
                     make_pair(ip,
                               make_pair(localPort,
                                         remotePort)))
      < make_pair(right.transport,
                  make_pair(right.ip,
                            make_pair(right.localPort,
                                      right.remotePort)));
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

enum
{
  CONNECTION_MODEL_KERNEL = 0
};

namespace global
{
  extern int connectionModelArg;
  extern int peerAccept;
  extern std::list<int> peers;
  extern std::auto_ptr<ConnectionModel> connectionModelExemplar;

  extern std::list<Connection> connections;
  extern std::map<Order, Connection> elabMap;
  // A connection is in this map only if it is currently connected.
  extern std::map<Order, Connection *> planetMap;

  extern fd_set readers;
  extern int maxReader;

  extern std::auto_ptr<ControlInput> input;
  extern std::auto_ptr<ControlOutput> output;
}

void addDescriptor(int fd);

#endif
