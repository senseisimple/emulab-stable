/* hmcd.h - header file for Healthd Master Collection Daemon, and peer
   query program hmcdgrab.
*/
#ifndef HMCD_H
#define HMCD_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>

#define PUSH_PORT 8505
#define MAX_HOSTS 1000
#define SOCKPATH "/tmp/hwcollectsock"
#define MAXCLIENTS 5

typedef struct {
  char *id;      /* Host identifier - probably local part of hostname */
  char *data;    /* String containing monitor output values */
} HMONENT;

extern int errno;

#endif
