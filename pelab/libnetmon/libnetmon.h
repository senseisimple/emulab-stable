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
#include <arpa/inet.h>
#include <sys/un.h>

/* #define DEBUGGING */

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
 * Allocate space for the monitorFDs - increases the allocation by
 * FD_ALLOC_SIZE slots.
 */
static void allocFDspace();

/*
 * Predicate function: Are we monitoring this file descriptor?
 */
static bool monitorFD_p(int);

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
} fdRecord;

/*
 * List of which file descriptors we're monitoring
 */
static fdRecord* monitorFDs;
static unsigned int fdSize;

/*
 * Stream on which to write reports
 */
FILE *outstream;

/*
 * Force the socket buffer size
 */
int forced_bufsize;

/*
 * Manipulate the monitorFDs structure
 */
static void startFD(int, const struct sockaddr *);
static void stopFD(int);

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

/*
 * Locations of the real library functions
 */
static socket_proto_t  *real_socket;
static close_proto_t   *real_close;
static connect_proto_t *real_connect;
static write_proto_t   *real_write;
static send_proto_t    *real_send;
static setsockopt_proto_t *real_setsockopt;

/*
 * Note: Functions that we're wrapping are in the .c file
 */
