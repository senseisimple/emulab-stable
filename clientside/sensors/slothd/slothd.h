/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* slothd.h
   Utah Network Testbed project
   Header file for node idle detector daemon.
*/

#ifndef _SLOTHD_H
#define _SLOTHD_H

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <syslog.h>
#include <utmp.h>

#ifdef __linux__
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#endif

#ifdef __CYGWIN__
#include <windows.h>	/* For GetLastInputInfo() and GetTickCount(). */
#include <iphlpapi.h>	/* For GetIfTable(). */
#endif /* __CYGWIN__ */

#define SDPROTOVERS "2"

#define SLOTHD_PATH_ENV "/bin:/usr/bin:/sbin:/usr/sbin:" CLIENT_BINDIR
#define UTMP_PATH "/var/run/utmp"
#define WTMP_PATH "/var/log/wtmp"
#define PIDFILE "/var/run/slothd.pid"
#define MACADDRLEN 18
#define MAXNUMIFACES 10
#define MAXIFNAMELEN 30  /* On Windows, "Local Area Connection 16". */
#define LINEBUFLEN 256
#define MAXTTYS 2000
#define MAXDEVLEN 50
#define MIN_RINTVL 1     /* 1 minute */
#define DEF_RINTVL 3600  /* 1 hour */
#define MIN_AINTVL 1     /* 1 second */
#define DEF_AINTVL 10    /* 10 seconds */
#define MIN_LTHRSH 0.1   /* Load avg. threshold */
#define DEF_LTHRSH 1     /* Fairly high - could get false pos/neg */
#define MIN_CTHRSH 0     /* No packets */
#define DEF_CTHRSH 1     /* At least 1 packet */
#define OFFSET_FRACTION 0.5

#define SLOTHD_DEF_PORT 8509 /* XXX change */

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_DAEMON
#endif

#define TTYACT  (1<<0)
#define LOADACT (1<<1)
#define PKTACT  (1<<2)

typedef struct {
  int sd;
#ifdef __linux__
  int ifd; /* IOCTL file descriptor */
#endif
  u_int cnt;
  char *cifname;
#ifdef __CYGWIN__
  char *cifaddr;
#endif /* __CYGWIN__ */
  u_char dolast;
  time_t lastrpt;
  time_t startup;
  struct sockaddr_in servaddr;
  u_short numttys;
  char ttys[MAXTTYS][MAXDEVLEN];
} SLOTHD_PARAMS;

typedef struct {
  int ifcnt;
  u_long minidle;
  double loadavg[3];
  u_short actbits;
  struct {
    u_long ipkts;
    u_long opkts;
    char ifname[MAXIFNAMELEN];
    char addr[MACADDRLEN];
  } ifaces[MAXNUMIFACES];
} SLOTHD_PACKET;

typedef struct {
  time_t reg_interval;
  time_t agg_interval;
  double load_thresh;
  u_long pkt_thresh;
  u_long cif_thresh;
  u_char debug;
  u_char once;
  char *servname;
  u_short port;
} SLOTHD_OPTS;

int parse_args(int, char**);
int init_slothd(void);
void do_exit(void);
int send_pkt(SLOTHD_PACKET*);
int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data);

void get_min_tty_idle(SLOTHD_PACKET*);
void get_load(SLOTHD_PACKET*);
void get_packet_counts(SLOTHD_PACKET*);
int get_active_bits(SLOTHD_PACKET*, SLOTHD_PACKET*);

int get_counters(char*,void*);
int grab_cifname(char*,void*);
int clear_ttys(void);
int add_tty(char*);
int enum_ttys(void);

#endif /* #ifndef _SLOTHD_H */
