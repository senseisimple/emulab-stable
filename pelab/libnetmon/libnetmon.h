/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
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

// #define DEBUGGING

#ifdef DEBUGGING
#define DEBUG(x) (x)
#else
#define DEBUG(x)
#endif

/*
 * Just a few things for convience
 */
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
 * Parse a list of reports
 */
static void lnm_parse_reportopt(char *s);

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
 * Logging messages
 */

/*
 * The types of log messages which are currently defined, and the associated
 * ASCII strings
 */
typedef enum { LOG_NEW = 0,
               LOG_REMOTEIP,
               LOG_REMOTEPORT,
               LOG_LOCALPORT,
               LOG_TCP_NODELAY,
               LOG_TCP_MAXSEG,
               LOG_SO_RCVBUF,
               LOG_SO_SNDBUF,
               LOG_CONNECTED,
               LOG_ACCEPTED,
               LOG_SEND,
               LOG_SENDTO,
               LOG_CLOSED,
               LOG_INIT,
               LOG_EXIT,
               LOG_SENDMSG
} logmsg_t;
static char *log_type_names[] = {
    "New",
    "RemoteIP",
    "RemotePort",
    "LocalPort",
    "TCP_NODELAY",
    "TCP_MAXSEG",
    "SO_RCVBUF",
    "SO_SNDBUF",
    "Connected",
    "Accepted",
    "Send",
    "SendTo",
    "Closed",
    "Init",
    "Exit",
    "SendMsg"
};

/*
 * Print out a log message. First argument is the type of the log mesage, the
 * second is the file descriptor number, and the remaining arguments are
 * processed like arguments to printf() (ie. a format string, then zero or more
 * arguments to be interpolated into the format string)
 */
static void printlog(logmsg_t,int, ... );

/*
 * A constant passed to printlog() to indicate that there is no file descriptor
 * associated with this message
 */
static int NO_FD = -42;

/*
 * Log that a packet has been sent to the kernel on a given FD with a given
 * size. If the final argument is non-NULL, then we are logging a packet from
 * sendto(), and should report the IP address and port numbers involved
 */
static void log_packet(int, size_t, const struct sockaddr*);

/*
 * The information we keep about each FD we're monitoring
 */
typedef struct {
    bool monitoring;
//    char *remote_hostname; /* We keep the char* so that we don't have to
//                              convert every time we want to report */
    struct in_addr remote_hostname; /* We keep this as an in_addr
                                       because inet_ntoa is not
                                       re-entrant and uses static
                                       memory for its result. */
    int remote_port;
    int local_port;

    bool connected; /* Is this FD currently connected or not? */

    int socktype; /* Socket type - normally SOCK_STREAM or SOCK_DGRAM */

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
 * Print out the current time in standard format
 */
void fprintTime(FILE *);

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
 * Our PID
 */
static pid_t pid;

/*
 * These describe the types of sockets we're monitoring - basically, are we
 * monitoring TCP, UDP, or both?
 */
static bool monitor_tcp;
static bool monitor_udp;

/*
 * These describe which things we need to report on. report_all is sort of a
 * meta-option - at the end of initialization, if it's set, we turn on the
 * rest of the options, but it's not used after that. This is to cut down
 * on the number of places we have to remember all of them...
 */
static bool report_all;
static bool report_io;
static bool report_sockopt;
static bool report_connect;
static bool report_init;

/*
 * Prototypes for the real library functions - just makes it easier to declare
 * function pointers for them.
 */
typedef int open_proto_t(const char *, int, ...);
typedef int stat_proto_t(const char *, struct stat *sb);
typedef int socket_proto_t(int, int,int);
typedef int close_proto_t(int);
typedef int connect_proto_t(int, const struct sockaddr*, socklen_t);
typedef ssize_t write_proto_t(int, const void *, size_t);
typedef ssize_t send_proto_t(int, const void *, ssize_t, int);
typedef int setsockopt_proto_t(int, int, int, const void*, socklen_t);
typedef ssize_t read_proto_t(int, void *, size_t);
typedef ssize_t recv_proto_t(int, void *, size_t, int);
typedef ssize_t recvmsg_proto_t(int, struct msghdr *, int);
typedef ssize_t accept_proto_t(int, struct sockaddr *, socklen_t *);
typedef ssize_t sendto_proto_t(int, const void *, size_t, int,
                               const struct sockaddr *, socklen_t);
typedef ssize_t sendmsg_proto_t(int, const struct msghdr *, int);

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
static sendto_proto_t     *real_sendto;
static sendmsg_proto_t    *real_sendmsg;

/*
 * Note: Functions that we're wrapping are in the .c file
 */

/*
 * Functions we use to inform the user about various events
 */
static void informNodelay(int);
static void informMaxseg(int);
static void informBufsize(int, int);
typedef enum {INFORM_CONNECT, INFORM_ACCEPT} inform_which_t;
static void informConnect(int,inform_which_t);
