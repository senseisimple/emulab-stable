
#ifndef _STUB_H
#define _STUB_H

#ifdef __cplusplus
extern "C"
{
#endif

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

#define STDIN 0 // file descriptor for standard input
#define QUANTA 500    //feed-loop interval in msec
#define MONITOR_PORT 4200 //the port the monitor connects to
#define SENDER_PORT  3491 //the port the stub senders connect to 
#define PENDING_CONNECTIONS  10	 //the pending connections the queue will hold
#define CONCURRENT_SENDERS   50	 //concurrent senders the stub maintains
#define CONCURRENT_RECEIVERS 50	 //concurrent receivers the stub maintains
#define MAX_PAYLOAD_SIZE     64000 //size of the traffic payload 

#define MAX_TCPDUMP_LINE     256 //the max line size of the tcpdump output
#define SIZEOF_LONG sizeof(long) //message bulding block
#define BANDWIDTH_OVER_THROUGHPUT 0 //the safty margin for estimating the available bandwidth
#define SNIFF_WINSIZE 131071 //from min(net.core.rmem_max, max(net.ipv4.tcp_rmem)) on Plab linux
#define SNIFF_TIMEOUT 0 //in msec 

//magic numbers
#define CODE_BANDWIDTH  0x00000001 
#define CODE_DELAY      0x00000002 
#define CODE_LOSS       0x00000003 
#define CODE_LIST_DELAY 0x00000004

//magic numbers for alternative algorithms
#define BANDWIDTH_AVERAGE 0
#define BANDWIDTH_MAX 1
#define BANDWIDTH_VEGAS 2
#define BANDWIDTH_BUFFER 3

#define MONITOR_RECORD_SIZE (sizeof(long)*3 + sizeof(unsigned short)*3)

#define PACKET_WRITE 1
#define PACKET_SEND_BUFFER 2
#define PACKET_RECEIVE_BUFFER 3

enum { FAILED_LOOKUP = -1 };
// The int is the index of the connection.
typedef void (*handle_index)(int);
// The first int is the socket of the monitor.
// The second int is the index of the connection.
// This returns 1 for success and 0 for failure.
typedef int (*send_to_monitor)(int, int);

// Information about each write that is going to happen.
typedef struct
{
  long size;
  // delta is a time difference in milliseconds
  long delta;
} pending_write;

// The total number of writes we will queue up before discarding.
enum { PENDING_SIZE = 40 };

// The data structure which hold the writes pending for a particular connection
typedef struct
{
  // This should start out false. If it is false, then the rest of the
  // data in this struct is undefined.
  int is_pending;
  // When is the earliest moment at which we should try a write?
  struct timeval deadline;
  // When did the last write occur? Used to determine inter-write times.
  struct timeval last_write;
  // The list of the actual writes themselves. This is a circular
  // queue. When it runs out of room it overwrites the oldest pending
  // write.
  pending_write writes[PENDING_SIZE];
  // The index of the current write under consideration. 
  int current_index;
  // The index of the next free slot. This may be the same as
  // 'current_index'. If this is so, then the write indexed by
  // 'current_index' is the oldest write pending and is to be
  // overridden if another write comes along.
  int free_index;
} pending_list;

// Initializes the pending write structure.
void init_pending_list(int index, long size, struct timeval time);

// Adds a pending write onto the tail of the list.
void push_pending_write(int index, pending_write current);

// Removes the oldest pending write.
void pop_pending_write(int index);

typedef struct {
  short  valid;
  int    sockfd;
  unsigned long ip;
  unsigned short stub_port;
  unsigned short source_port;
  unsigned short dest_port;
  time_t last_usetime; //last monitor access time
  pending_list pending; // What writes are pending?
} connection;
typedef struct {
  struct timeval captime;
  unsigned long  seq_start;
  unsigned long  seq_end;
  unsigned int   pkt_size;
} sniff_record;
typedef struct {
  sniff_record records[SNIFF_WINSIZE];
  int start; //circular buffer pointers
  int end;
} sniff_path;
typedef struct {
  unsigned int loss_counter; //in terms of packet
  unsigned int total_counter;
} loss_record;
typedef struct delay_sample {
  struct delay_sample *next;
  unsigned long       value;
  struct timeval       time;
} delay_sample;
typedef struct {
  delay_sample *head;
  delay_sample *tail;
  int  sample_number;
} delay_record;

extern short  flag_debug;
extern short flag_standalone;
extern int bandwidth_method;
extern int pcapfd;
extern int maxfd;
extern connection snddb[CONCURRENT_SENDERS];
extern connection rcvdb[CONCURRENT_RECEIVERS];
extern sniff_path sniff_rcvdb[CONCURRENT_RECEIVERS];
extern unsigned long delays[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
extern unsigned long last_delays[CONCURRENT_RECEIVERS];
extern unsigned long delay_count[CONCURRENT_RECEIVERS];
extern loss_record loss_records[CONCURRENT_RECEIVERS]; //loss is calculated at the sender side
extern unsigned long last_loss_rates[CONCURRENT_RECEIVERS]; //loss per billion
extern delay_record delay_records[CONCURRENT_RECEIVERS]; //delay is calculated at the sender side
extern int is_live;

extern int last_through[CONCURRENT_RECEIVERS]; 
extern int buffer_full[CONCURRENT_RECEIVERS];

extern unsigned long max_throughput[CONCURRENT_RECEIVERS];
extern unsigned long base_rtt[CONCURRENT_RECEIVERS];

extern void sniff(void);
extern void init_pcap(int to_ms, unsigned short port, char * device);
extern void append_delay_sample(int path_id, long sample_value,
				struct timeval const * timestamp);
extern void remove_delay_samples(int path_id);
extern void clean_exit(int);
extern void update_stats(void);
extern unsigned int received_stat(void);
extern unsigned int dropped_stat(void);

typedef struct
{
  unsigned int firstUnknown;
  unsigned int nextSequence;
  unsigned int ackSize;
  unsigned int fullAckSize; //full packet size
  unsigned int repeatSize;
  struct timeval beginTime;
  struct timeval endTime;
  int isValid;
} ThroughputAckState;

extern ThroughputAckState throughput[CONCURRENT_RECEIVERS];

// Returns the number of acknowledged bytes since the last
// throughputTick() call.
extern unsigned int throughputTick(ThroughputAckState * state);
extern void throughputInit(ThroughputAckState * state, unsigned int sequence,
			   struct timeval const * firstTime);
extern unsigned int bytesThisTick(ThroughputAckState * state);

// Add a potential sender to the pool.
void add_empty_sender(int index);

// Initialize a receiver or sender connection.
void init_connection(connection * conn);

// Run function on the index of every valid sender that is readable.
void for_each_readable_sender(handle_index function, fd_set * read_fds_copy);

// Try to find the sender by the IP address and the source port of the
// actual connection being created by the seinding peer. If there is a
// sender, the socket is closed and replaced with the sockfd
// argument. If the sender cannot be found, create a new sender based
// on the sockfd argument.
// Returns FAILED_LOOKUP if the sender cannot be found and there are
// no empty slots.
int replace_sender_by_stub_port(unsigned long ip, unsigned short stub_port,
                                int sockfd, fd_set * read_fds);

// Remove the index from the database, invalidating it.
void remove_sender_index(int index, fd_set * read_fds);

// Add a potential receiver to the pool.
void add_empty_receiver(int index);

// Run function on the index of each writable receiver which has some
// bytes pending.
void for_each_pending(handle_index function, fd_set * write_fds_copy);

// Run function on each index which will send an update to the
// monitor. Returns 1 for success and 0 for failure.
int for_each_to_monitor(send_to_monitor function, int monitor);

// Find the index of the receiver based on the IP address of the
// receiver, and the source and destination ports of the Emulab
// connection.
// Returns FAILED_LOOKUP on failure or the index of the receiver.
int find_by_address(unsigned long ip, unsigned short source_port,
                    unsigned short dest_port);


// Find the index of the receiver based on the above criteria. If this
// fails, create a new connection to the IP address.
// Returns FAILED_LOOKUP on failure or the index of the receiver.
// Failure happens when the receiver is not already in the database
// and there are no more empty slots in the database.
int insert_by_address(unsigned long ip, unsigned short source_port,
                      unsigned short dest_port);

// Insert a fake entry for purely monitoring purposes.
int insert_fake(unsigned long ip, unsigned short port);

// Reconnect to a receiver. Resets the socket, the stub_port, and the
// sniff records.
void reconnect_receiver(int index);


// Reset a receiver to point to the ip source_port and dest_port
// specified. Note that this does not change the socket or stub_port
// at all (call reconnect for that). Nor does it change the sniff records.
void reset_receive_records(int index, unsigned long ip,
                           unsigned short source_port,
                           unsigned short dest_port);

// Find the index of the receiver based on the IP address and the
// source port number of the stub connection.
// Returns FAILED_LOOKUP on failure.
int find_by_stub_port(unsigned long ip, unsigned short stub_port);

// Put the index into the pending category.
void set_pending(int index, fd_set * write_fds);

// Remove the index from the pending category.
void clear_pending(int index, fd_set * write_fds);

// Remove a receiver from the database, invalidating it.
void remove_index(int index, fd_set * write_fds);

#ifdef __cplusplus
}
#endif

#endif
