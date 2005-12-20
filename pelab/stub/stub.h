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

#define QUANTA 5000000    // feed-loop interval in usec
#define CONTROL_PORT 4200 // the port users will be connecting to
#define TRAFFIC_PORT 3491 // the port client will be connecting to 
#define CONCURRENT_SENDERS   50	 // how many pending connections queue will hold
#define CONCURRENT_RECEIVERS 50	 // how many pending connections queue will hold
#define MAX_PKTSIZE         100  // conservative for now 
#define SIZEOF_LONG    sizeof(long)


struct connection {
  short  valid;
  int    sockfd;
  unsigned long ip;
  time_t last_usetime;
};
typedef struct connection connection;

#endif













