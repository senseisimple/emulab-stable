/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* hmcd.h - header file for Healthd Master Collection Daemon, and peer
   query program hmcdgrab.
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
#include <tbdb.h>

#define SDCOLLECTD_PORT 8509
#define NODENAMESIZE 100
#define BUFSIZE 1500
#define MAXNUMIFACES 10
#define MACADDRLEN 12

typedef struct {
  u_char debug;
  u_short port;
} SDCOLLECTD_OPTS;

SDCOLLECTD_OPTS *opts;

typedef struct {
  long mis;
  double l1m;
  double l5m;
  double l15m;
  u_char ifcnt;
  struct {
    char mac[MACADDRLEN+1];
    long ipkts;
    long opkts;
  } ifaces[MAXNUMIFACES];
  char id[NODENAMESIZE];      /* Host identifier - probably local part of hostname */
  char buf[BUFSIZE];    /* String containing monitor output values */
} IDLE_DATA;

extern int errno;

int CollectData(int, IDLE_DATA*);
void ParseRecord(IDLE_DATA*);
void PutDBRecord(IDLE_DATA*);

char *tbmac(char*, char**);


#endif
