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
#include <syslog.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <syslog.h>
#include <varargs.h>
#include <utmp.h>
#ifdef __linux__
#include <net/if.h>
#include <netinet/ether.h>
#include <sys/ioctl.h>
#endif

#define SLOTHD_PATH_ENV "/bin:/usr/bin:/sbin:/usr/sbin:/etc/testbed"
#define UTMP_PATH "/var/run/utmp"
#define WTMP_PATH "/var/log/wtmp"
#define PIDFILE "/var/run/slothd"
#define MACADDRLEN 18
#define MAXNUMIFACES 10
#define MAXIFNAMELEN 10
#define LINEBUFLEN 256
#define MAXTTYS 2000
#define MAXDEVLEN 50
#define MIN_INTVL 1
#define DEF_INTVL 3600  /* 1 hour */

#define SLOTHD_DEF_SERV "boss"
#define SLOTHD_DEF_PORT 8509 /* XXX change */

#ifdef __FreeBSD__
#define CNTFMTSTR "%s %*s %*s %s %lu %*s %lu"
#define NUMSCAN 4
#endif

#ifdef __linux__
#define CNTFMTSTR "%s %*s %*s %lu %*s %*s %*s %lu"
#define NUMSCAN 3
#endif

typedef struct {
  u_long minidle;
  double loadavg[3];
  int ifcnt;
  int sd;
  struct sockaddr_in servaddr;
  struct {
    u_long ipkts;
    u_long opkts;
    char ifname[MAXIFNAMELEN];
    char addr[MACADDRLEN];
  } ifaces[MAXNUMIFACES];
} SLOTHD_PACKET;

typedef struct {
  u_int interval;
  u_short numttys;
  u_char debug;
  u_char actterms;
  u_char first;
  char *servname;
  u_short port;
  char *ttys[MAXTTYS];
} SLOTHD_OPTS;

int parse_args(int, char**);
int init_slothd(void);

void get_min_tty_idle(void);
void utmp_enum_terms(void);
time_t wtmp_get_last(void);
void get_load(void);
void get_packet_counts(void);

int get_macaddrs(char*,void*);
int get_counters(char*,void*);

int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data);
void send_pkt(void);

#endif /* #ifndef _SLOTHD_H */
