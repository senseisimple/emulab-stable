/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

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
#include <assert.h>
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
#include <algorithm>
#include <iomanip>

// Udp-CHANGES-Begin
#include <functional>
#include <climits>
#include <limits.h>
#include <sys/param.h>
#include <endian.h>

#if !defined(BIG_ENDIAN) && defined(__BIG_ENDIAN)
#define BIG_ENDIAN __BIG_ENDIAN
#endif

#if !defined(LITTLE_ENDIAN) && defined(__LITTLE_ENDIAN)
#define LITTLE_ENDIAN __LITTLE_ENDIAN
#endif

#if !defined(__BYTEORDER) && !defined(BYTEORDER)

#if defined(i386) || defined(__i386__) || defined(__x86__)
#define BYTEORDER LITTLE_ENDIAN
#else
#define BYTEORDER BIG_ENDIAN
#endif

#endif


#if BYTEORDER == BIG_ENDIAN

#define htonll(A) (A)

#elif BYTEORDER == LITTLE_ENDIAN

#define htonll(A) (     (((uint64_t)(A) & 0xff00000000000000ULL) >> 56) | \
                        (((uint64_t)(A) & 0x00ff000000000000ULL) >> 40) | \
                        (((uint64_t)(A) & 0x0000ff0000000000ULL) >> 24) | \
                        (((uint64_t)(A) & 0x000000ff00000000ULL) >> 8) | \
                        (((uint64_t)(A) & 0x00000000ff000000ULL) << 8) | \
                        (((uint64_t)(A) & 0x0000000000ff0000ULL) << 24) | \
                        (((uint64_t)(A) & 0x000000000000ff00ULL) << 40) | \
                        (((uint64_t)(A) & 0x00000000000000ffULL) << 56)   )
#endif

// Udp-CHANGES-End

#include "Time.h"

void setDescriptor(int fd);
void clearDescriptor(int fd);
std::string ipToString(unsigned int ip);
int createServer(int port, std::string const & debugString);
int acceptServer(int acceptfd, struct sockaddr_in * remoteAddress,
                 std::string const & debugString);

class PacketInfo;

void replayWriteCommand(char * head, char * body, unsigned short bodySize);
void replayWritePacket(PacketInfo * packet);


// Enum of header types -- to-monitor (Events, etc.)
enum
{
  // In these two cases, the body of the message is a string to be
  // sent as an opaque event.
  EVENT_FORWARD_PATH = 0,
  EVENT_BACKWARD_PATH = 1,

  // In these two cases, the body of the message is a string
  // consisting only of the number of estimated throughput or
  // bandwidth in kbps.

  // This is the bandwidth number to send if it exceeds the current
  // bandwidth on the monitor side.
  TENTATIVE_THROUGHPUT = 2,
  // This is a bandwidth number to use even if it is smaller than the
  // bandwidth on the monitor side
  AUTHORITATIVE_BANDWIDTH = 3,

  // This is used to let the monitor know which connections we are
  // keeping track of for synchronization purposes.
  SYNC_REPLY = 4
};

// Enum of header types -- from-monitor (Commands)
enum
{
  NEW_CONNECTION_COMMAND = 0,
  TRAFFIC_MODEL_COMMAND = 1,
  CONNECTION_MODEL_COMMAND = 2,
  SENSOR_COMMAND = 3,
  CONNECT_COMMAND = 4,
  TRAFFIC_WRITE_COMMAND = 5,
  DELETE_CONNECTION_COMMAND = 6,
  // A new monitor uses this to ensure a blank slate.
  DELETE_ALL_CONNECTIONS_COMMAND = 7,
  // A monitor is sending a heartbeat request to ensure that our
  // connection lists are synced up and the command connection is
  // still active.
  SYNC_REQUEST_COMMAND = 8,
  // These are pseudo-commands for replay.
  PACKET_INFO_SEND_COMMAND = 255,
  PACKET_INFO_ACK_COMMAND = 254
};

// This is used for the type field in a SensorCommand
enum SensorType
{
  NULL_SENSOR = 0,
  STATE_SENSOR = 1,
  PACKET_SENSOR = 2,
  DELAY_SENSOR = 3,
  MIN_DELAY_SENSOR = 4,
  MAX_DELAY_SENSOR = 5,
  THROUGHPUT_SENSOR = 6,
  EWMA_THROUGHPUT_SENSOR = 7,
  LEAST_SQUARES_THROUGHPUT = 8,
  TSTHROUGHPUT_SENSOR = 9,
  AVERAGE_THROUGHPUT_SENSOR = 10,
// Udp-CHANGES-Begin
  UDP_STATE_SENSOR = 11,
  UDP_PACKET_SENSOR = 12,
  UDP_THROUGHPUT_SENSOR = 13,
  UDP_MINDELAY_SENSOR = 14,
  UDP_MAXDELAY_SENSOR = 15,
  UDP_RTT_SENSOR = 16,
  UDP_LOSS_SENSOR = 17,
  UDP_AVG_THROUGHPUT_SENSOR = 18
// Udp-CHANGES-End
};

// This is used for the type field in the ConnectionModelCommand.
enum
{
  CONNECTION_SEND_BUFFER_SIZE = 0,
  CONNECTION_RECEIVE_BUFFER_SIZE = 1,
  CONNECTION_MAX_SEGMENT_SIZE = 2,
  CONNECTION_USE_NAGLES = 3
};

enum
{
  TCP_CONNECTION = 0,
  UDP_CONNECTION = 1
};

struct ElabOrder
{
  static const size_t idSize = 32;
  char id[idSize];

  ElabOrder()
  {
    memset(id, '\0', idSize);
  }
  bool operator<(ElabOrder const & right) const
  {
    return memcmp(id, right.id, idSize) < 0;
  }
  bool operator==(ElabOrder const & right) const
  {
    return memcmp(id, right.id, idSize) == 0;
  }
  bool operator!=(ElabOrder const & right) const
  {
    return memcmp(id, right.id, idSize) != 0;
  }
  std::string toString(void) const
  {
    return std::string(id, idSize);
  }
};

struct PlanetOrder
{
  unsigned char transport;
  unsigned int ip;
  unsigned short localPort;
  unsigned short remotePort;

  PlanetOrder()
  {
    transport = TCP_CONNECTION;
    ip = 0;
    localPort = 0;
    remotePort = 0;
  }
  bool operator<(PlanetOrder const & right) const
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
  bool operator==(PlanetOrder const & right) const
  {
    return transport == right.transport
      && ip == right.ip
      && localPort == right.localPort
      && remotePort == right.remotePort;
  }
  bool operator!=(PlanetOrder const & right) const
  {
    return !(*this == right);
  }
  std::string toString(void) const
  {
    std::ostringstream buffer;
    if (transport == TCP_CONNECTION)
    {
      buffer << "TCP,";
    }
    else
    {
      buffer << "UDP,";
    }
    buffer << localPort << ":" << ipToString(htonl(ip)) << ":" << remotePort;
    return buffer.str();
  }
};

struct WriteResult
{
  bool isConnected;
  bool bufferFull;
  PlanetOrder planet;
  Time nextWrite;
};

struct Option
{
  Option() : type(0), length(0), buffer(NULL) {}
  unsigned char type;
  unsigned char length;
  unsigned char const * buffer;
};

struct PacketInfo
{
  size_t census(void) const;

  // This should come first because it determines which of the other
  // two options are selected.
  unsigned char transport;
  Time packetTime;
  int packetLength;
  /* ---IMPORTANT NOTE--- The tcp_info structure is built at the time of
   * packet processing, not at the time that a packet is
   * captured. Most of the time it doesn't matter if the data here
   * is a little stale. But things like tcp state that can change
   * fundamentally on a packet by packet basis cannot be used!  */
  struct tcp_info const * kernel;
  struct ip const * ip;
  std::list<Option> * ipOptions;
  struct tcphdr const * tcp;
  std::list<Option> * tcpOptions;
  struct udphdr const * udp;
  // The size of the captured payload. This may be less than the
  // payload actually received (due to snaplen).
  unsigned int payloadSize;
  // A pointer to the first character of a packet after the headers
  unsigned char const * payload;
  ElabOrder elab;
  bool bufferFull;
  unsigned char packetType;
};

enum
{
  CONNECTION_MODEL_NULL = 0,
  CONNECTION_MODEL_KERNEL = 1
};

enum
{
  NO_REPLAY = 0,
  REPLAY_LOAD = 1,
  REPLAY_SAVE = 2
};

enum
{
  SNAPLEN_SIZE = 250
};

class ConnectionModel;
class Connection;
class CommandInput;
class CommandOutput;

namespace global
{
  extern int connectionModelArg;
  extern unsigned short peerUdpServerPort;
  extern unsigned short peerServerPort;
  extern unsigned short monitorServerPort;
  extern bool doDaemonize;
  extern int replayArg;
  extern int replayfd;
  extern std::list<int> replaySensors;

  extern int peerAccept;
  extern std::auto_ptr<ConnectionModel> connectionModelExemplar;
  extern std::string interface;
  // file descriptor, IP address string
  extern std::list< std::pair<int, std::string> > peers;

  extern std::map<ElabOrder, Connection> connections;
  // A connection is in this map only if it is currently connected.
  extern std::map<PlanetOrder, Connection *> planetMap;

  extern fd_set readers;
  extern int maxReader;

  extern std::auto_ptr<CommandInput> input;
  extern std::auto_ptr<CommandOutput> output;

  extern int logFlags;

  extern unsigned char CONTROL_VERSION;

// Udp-CHANGES-Begin
  extern const short int USHORT_INT_SIZE;
  extern const short int ULONG_LONG_SIZE;
  extern const short int UCHAR_SIZE;

  extern const int udpRedunAckSize;
  extern const int udpSeqNumSize;
  extern const int udpMinAckPacketSize;
  extern const int udpMinSendPacketSize;
// Udp-CHANGES-End


}

// Udp-CHANGES-Begin
struct UdpPacketInfo
{
        unsigned short int seqNum;
        unsigned short int packetSize;
        unsigned long long timeStamp;
        bool isFake;
};

struct UdpPacketCmp{
        unsigned short seqNum;
        unsigned long long timeStamp;
};

class equalSeqNum:public std::binary_function<UdpPacketInfo , unsigned short int, bool>
{
  public:
  bool operator()(const UdpPacketInfo& packet, unsigned short int seqNum) const
  {
    return (packet.seqNum == seqNum);
  }
};

class lessSeqNum:public std::binary_function<UdpPacketInfo , UdpPacketCmp *,bool>
{
  public:
  bool operator()(const UdpPacketInfo& packet,UdpPacketCmp *cmpPacket) const
  {
    return ( (packet.timeStamp < cmpPacket->timeStamp));
  }
};


// Udp-CHANGES-End
#endif
