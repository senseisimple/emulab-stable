// lib.h

#ifndef LIB_H_PELAB_2
#define LIB_H_PELAB_2

extern "C"
{
#include <stdio.h>
#include <getopt.h>
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

extern char * optarg;
}

#include <utility>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include "Time.h"

// Enum of header types -- to-monitor (Events, etc.)
enum
{
  EVENT_FORWARD_PATH = 0,
  EVENT_BACKWARD_PATH
};

// Enum of header types -- from-monitor (Commands)
enum
{
  NEW_CONNECTION_COMMAND = 0,
  TRAFFIC_MODEL_COMMAND,
  CONNECTION_MODEL_COMMAND,
  SENSOR_COMMAND,
  CONNECT_COMMAND,
  TRAFFIC_WRITE_COMMAND,
  DELETE_CONNECTION_COMMAND
};

// This is used for the type field in a SensorCommand
enum SensorType
{
  NULL_SENSOR = 0,
  PACKET_SENSOR,
  DELAY_SENSOR,
  MIN_DELAY_SENSOR,
  MAX_DELAY_SENSOR,
  THROUGHPUT_SENSOR
};

// This is used for the type field in the ConnectionModelCommand.
enum
{
  CONNECTION_SEND_BUFFER_SIZE = 0,
  CONNECTION_RECEIVE_BUFFER_SIZE,
  CONNECTION_MAX_SEGMENT_SIZE,
  CONNECTION_USE_NAGLES
};

enum
{
  TCP_CONNECTION = 0,
  UDP_CONNECTION = 1
};

struct Order
{
  unsigned char transport;
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

struct WriteResult
{
  bool isConnected;
  bool bufferFull;
  Order planet;
  Time nextWrite;
};

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
