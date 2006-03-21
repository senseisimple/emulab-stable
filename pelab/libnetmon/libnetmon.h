/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 *
 * Header for libnetmon, a library for monitoring network traffic sent by a
 * process. See README for instructions.
 */

#ifdef __linux__
/* Needed to get RTLD_NEXT on Linux */
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>

#include "netmon.h"

#define DEBUGGING

#ifdef DEBUGGING
#define DEBUG(x) (x)
#else
#define DEBUG(x)
#endif

/*
 * Just a few things for convience
 */
typedef enum {true = 1, false = 0} bool;
const unsigned int FD_ALLOC_SIZE = 8;

/*
 * Initialization for the library - dies if initialization fails. Safe to call
 * more than once, just skips intialization if it's already been done.
 */
static void lnm_init();

/*
 * Handle packets on the control socket, if any
 */
static void lnm_control();

/*
 * Wait for a control message, then process it
 */
static void lnm_control_wait();

/*
 * Allocate space for the monitorFDs - increases the allocation by
 * FD_ALLOC_SIZE slots.
 */
static void allocFDspace();

/*
 * Make sure there's enough room for the given FD in the file descriptor
 * table
 */
static void allocFDspaceFor(int);

/*
 * Predicate function: Are we monitoring this file descriptor?
 */
static bool monitorFD_p(int);

/*
 * Predicate funtion: Is this file descriptor connected?
 */
static bool connectedFD_p(int);

/*
 * Die! Takes printf-style format and args
 */
static void croak(char *, ...);

/*
 * Log that a packet has been sent to the kernel on a given FD with a given
 * size
 */
static void log_packet(int, size_t);

/*
 * The information we keep about each FD we're monitoring
 */
typedef struct { 
    bool monitoring;
    char *remote_hostname; /* We keep the char* so that we don't have to
                              convert every time we want to report */
    int remote_port;
    int local_port;

    bool connected; /* Is this FD currently connected or not? */

    /*
     * Socket options we keep track of
     */
    int sndbuf;
    int rcvbuf;
    bool tcp_nodelay;
    int tcp_maxseg;

    
} fdRecord;

/*
 * List of which file descriptors we're monitoring
 */
static fdRecord* monitorFDs;
static unsigned int fdSize;

/*
 * Find out what size socket buffer the kernel just gave us
 */
static int getNewSockbuf(int,int);

/*
 * Stream on which to write reports
 */
static FILE *outstream;

/*
 * File descriptor for the control socket - < 0 if we have none
 */
static int controlfd;

/*
 * Force the socket buffer size
 */
static int forced_bufsize;

/*
 * Give a maximum socket buffer size
 */
static int max_bufsize;

/*
 * Manipulate the monitorFDs structure
 */
static void startFD(int);
static void nameFD(int, const struct sockaddr *, const struct sockaddr *);
static void stopFD(int);
static void stopWatchingAll();

/*
 * Print unique identifier for an FD
 */
static void fprintID(FILE *, int);

/*
 * Process a packet from the control socket
 */
static void process_control_packet(generic_m *);

/*
 * Sed out a query on the control socket
 */
static void control_query();

/*
 * Which version of the output format are we using?
 */
static unsigned int output_version;

/*
 * Prototypes for the real library functions - just makes it easier to declare
 * function pointers for them.
 */
typedef int open_proto_t(const char *, int, ...);
typedef int stat_proto_t(const char *, struct stat *sb);
typedef int socket_proto_t(int,int,int);
typedef int close_proto_t(int);
typedef int connect_proto_t(int, const struct sockaddr*, socklen_t);
typedef ssize_t write_proto_t(int, const void *, size_t);
typedef ssize_t send_proto_t(int, const void *, ssize_t, int);
typedef int setsockopt_proto_t(int, int, int, const void*, socklen_t);
typedef ssize_t read_proto_t(int, void *, size_t);
typedef ssize_t recv_proto_t(int, void *, size_t, int);
typedef ssize_t recvmsg_proto_t(int,struct msghdr *, int);
typedef ssize_t accept_proto_t(int,struct sockaddr *, socklen_t *);

/*
 * Locations of the real library functions
 */
static socket_proto_t     *real_socket;
static close_proto_t      *real_close;
static connect_proto_t    *real_connect;
static write_proto_t      *real_write;
static send_proto_t       *real_send;
static setsockopt_proto_t *real_setsockopt;
static read_proto_t       *real_read;
static recv_proto_t       *real_recv;
static recvmsg_proto_t    *real_recvmsg;
static accept_proto_t     *real_accept;

/*
 * Note: Functions that we're wrapping are in the .c file
 */

/*
 * Functions we use to inform the user about various events
 */
static void informNodelay(int);
static void informMaxseg(int);
static void informBufsize(int, int);
static void informConnect(int);
