/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* sdcollectd.h - header file for Slothd Collection Daemon.
*/
#ifndef SDCOLLECTD_H
#define SDCOLLECTD_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <pwd.h>
#include <grp.h>
#include "tbdb.h"
#include "log.h"

#define SDPROTOVERS 2
#define SDCOLLECTD_PORT 8509
#define NODENAMESIZE 100
#define BUFSIZE 1500
#define MAXNUMIFACES 10
#define MACADDRLEN 12
#define RUNASUSER "nobody"

#define NUMACTTYPES 4
#define ACTSTRARRAY {"last_tty_act", "last_cpu_act", "last_net_act", "last_ext_act"}

typedef struct {
  u_char popold;
  u_char debug;
  u_short port;
} SDCOLLECTD_OPTS;

SDCOLLECTD_OPTS *opts;

typedef struct {
  long   mis;
  double l1m;
  double l5m;
  double l15m;
  u_char ifcnt;
  u_int  actbits;
  u_int  version;
  struct {
    char mac[MACADDRLEN+1];
    long ipkts;
    long opkts;
  }      ifaces[MAXNUMIFACES];
  char   id[NODENAMESIZE];      /* Host identifier - probably local part of hostname */
  char   buf[BUFSIZE];    /* String containing monitor output values */
} IDLE_DATA;

extern int errno;

int CollectData(int, IDLE_DATA*);
int ParseRecord(IDLE_DATA*);
void PutDBRecord(IDLE_DATA*);

char *tbmac(char*, char**);


#endif
