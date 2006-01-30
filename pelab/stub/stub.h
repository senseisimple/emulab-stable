#ifndef _STUB_H
#define _STUB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
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

#define STDIN 0 // file descriptor for standard input
#define QUANTA 500    //feed-loop interval in msec
#define MONITOR_PORT 4200 //the port the monitor connects to
#define SENDER_PORT  3491 //the port the stub senders connect to 
#define PENDING_CONNECTIONS  10	 //the pending connections the queue will hold
#define CONCURRENT_SENDERS   50	 //concurrent senders the stub maintains
#define CONCURRENT_RECEIVERS 50	 //concurrent receivers the stub maintains
#define MAX_PAYLOAD_SIZE     110000 //size of the traffic payload 

// This is the low water mark of the send buffer. That is, if select
// says that a write buffer is writable, this is the minimum amount of
// buffer space available.
#define LOW_WATER_MARK 110000

#define MAX_TCPDUMP_LINE     256 //the max line size of the tcpdump output
#define SIZEOF_LONG sizeof(long) //message bulding block
#define BANDWIDTH_OVER_THROUGHPUT 0 //the safty margin for estimating the available bandwidth
#define SNIFF_WINSIZE 131071 //from min(net.core.rmem_max, max(net.ipv4.tcp_rmem)) on Plab linux
#define SNIFF_TIMEOUT QUANTA/10 //in msec 

//magic numbers
#define CODE_BANDWIDTH 0x00000001 
#define CODE_DELAY     0x00000002 
#define CODE_LOSS      0x00000003 


struct connection {
  short  valid;
  int    sockfd;
  unsigned long ip;
  time_t last_usetime; //last monitor access time
  int pending; // How many bytes are pending to this peer?
};
typedef struct connection connection;
struct sniff_record {
  struct timeval captime;
  unsigned long  seq_start;
  unsigned long  seq_end;
};
typedef struct sniff_record sniff_record;
struct sniff_path {
  sniff_record records[SNIFF_WINSIZE];
  int start; //circular buffer pointers
  int end;
};
typedef struct sniff_path sniff_path;
struct loss_record {
  unsigned int loss_counter; //in terms of packet
  unsigned int total_counter;
};
typedef struct loss_record loss_record;


extern short  flag_debug;
extern int pcapfd;
extern char sniff_interface[128];
extern connection rcvdb[CONCURRENT_RECEIVERS];
extern sniff_path sniff_rcvdb[CONCURRENT_RECEIVERS];
extern unsigned long delays[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
extern loss_record loss_records[CONCURRENT_RECEIVERS]; //loss is calculated at the sender side

extern int search_rcvdb(unsigned long indexip);
extern void sniff(void);
extern void init_pcap(int to_ms);

typedef struct
{
  unsigned int firstUnknown;
  unsigned int nextSequence;
  unsigned int ackSize;
  unsigned int repeatSize;
  struct timeval lastTime;
  int isValid;
} ThroughputAckState;

extern ThroughputAckState throughput[CONCURRENT_RECEIVERS];

// Returns the number of acknowledged bytes since the last
// throughputTick() call.
extern unsigned int throughputTick(ThroughputAckState * state);
extern void throughputInit(ThroughputAckState * state, unsigned int sequence);
extern unsigned int bytesThisTick(ThroughputAckState * state);

#endif








