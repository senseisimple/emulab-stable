/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* canaryd.h
   Utah Network Testbed project
   Header file for node idle detector daemon.
*/

#ifndef _CANARYD_H
#define _CANARYD_H

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
#include <sys/param.h>
#include <arpa/inet.h>
#include <sys/rtprio.h>
#ifdef __linux__
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#endif

#include "event.h"
#include "tbdefs.h"
#include "auxfuncs.h"

#define CDPROTOVERS "1"

#define CANARYD_PATH_ENV "/bin:/usr/bin:/sbin:/usr/sbin:" CLIENT_BINDIR
#define PIDFILE "/var/run/canaryd.pid"
#define SYNCSERVFILE "/var/emulab/boot/syncserver"
#define EVENTKEYFILE "/var/emulab/boot/eventkey"
#define MACADDRLEN 18
#define MAXNUMIFACES 1000
#define MAXIFNAMELEN 10
#define MAXLINELEN 256
#define MAXPROCNUM 100000
#define MAXNUMVNODES 100
#define MAXDEVLEN 50
#define MAXHNAMELEN 256
#define MIN_INTVL 1     /* 1 second */
#define DEF_INTVL 60    /* one minute */
#define DEF_OLREPINTVL 10 /* 10 seconds */
#define OFFSET_FRACTION 0.5

#define CANARYD_DEF_PORT 8501

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_DAEMON
#endif

#ifdef __FreeBSD__
#define NETSTAT_ARGS "-nib"
#define CNTFMTSTR "%s %*s %*s %s %lu %*s %llu %lu %*s %llu"
#define NUMSCAN 6
#endif

#ifdef __linux__
#define NETSTAT_ARGS = "-ni";
#define CNTFMTSTR "%s %*s %*s %lu %*s %*s %*s %lu"
#define NUMSCAN 3
#endif

#define DEF_CDLOG "/var/emulab/logs/canaryd.log"

typedef struct {
  char *key;
  int defmin;
  int defmax;
} EVARGITEM;

#define NUMRANGES 7
enum {
  PCTCPU      = 0,
  PCTMEM      = 1,
  PGOUT       = 2,
  PCTDISK     = 3,
  PCTNETMEM   = 4,
  NETMEMWAITS = 5,
  NETMEMDROPS = 6
};

EVARGITEM evargitems[] = {
  {"PCT_CPU",      0, 100},
  {"PCT_MEM",      0, 100},
  {"PGOUT",        0, 0},
  {"PCT_DISK",     0, 100},
  {"PCT_NETMEM",   0, 100},
  {"NETMEM_WAITS", 0, 50},
  {"NETMEM_DROPS", 0, 50}
};

typedef struct {
  double min;
  double max;
} RANGE;

typedef struct {
  char *hname;
  u_int pid;
  float cpu;
  float mem;
} VPROCENT;

typedef struct {
  int sd;
#ifdef __linux__
  int ifd; /* IOCTL file descriptor */
#endif
  u_int cnt;
  char *cifname;
  u_char dolast;
  time_t lastrpt;
  time_t lastolrpt;
  time_t startup;
  struct sockaddr_in servaddr;
  FILE *log;
  char *pideid;
  char *ev_server;
  event_handle_t ev_handle;
  u_char ol_detect;
  u_char send_ev_report;
  u_short  numvnodes;
  RANGE olr[NUMRANGES];
} CANARYD_PARAMS;

typedef struct {
  int ifcnt;
  double loadavg[3];
  u_short overload;
  u_int olmet[NUMRANGES];
  struct {
    u_long ipkts;
    u_long opkts;
    unsigned long long ibytes;
    unsigned long long obytes;
    char ifname[MAXIFNAMELEN];
    char addr[MACADDRLEN];
  } ifaces[MAXNUMIFACES];
  VPROCENT vnodes[MAXNUMVNODES];
} CANARYD_PACKET;

typedef struct {
  time_t interval;
  time_t ol_rep_interval;
  u_char debug;
  u_char once;
  char *servname;
  u_short port;
  u_char log;
  u_int run_len;
} CANARYD_OPTS;

int parse_args(int, char**);
int init_canaryd(void);
void do_exit(int);
int send_pkt(CANARYD_PACKET*);
int process_ps(char*, void*);
int process_vmstat(char*, void*);
int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data);

void get_load(CANARYD_PACKET*);
void get_packet_counts(CANARYD_PACKET*);
void get_vnode_stats (CANARYD_PACKET*);
void get_vmem_stats(CANARYD_PACKET*);
void check_overload(CANARYD_PACKET*);
void send_ol_event(CANARYD_PACKET*);

int get_counters(char*,void*);
int grab_cifname(char*,void*);
int grab_pideid(char*, void*);
void ev_callback(event_handle_t, event_notification_t, void*);
void parse_ev_args(char*);

#endif /* #ifndef _CANARYD_H */
