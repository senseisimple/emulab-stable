/*-
 * Copyright (c) 1999-2000 James E. Housley <jim@thehousleys.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: healthd.c,v 1.4 2003-04-04 21:35:29 kwebb Exp $
 */
/*
 *
 * This code is loosely based upon Yoshifumi R. Shimizu's XMBmon 1.04
 * code.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <netdb.h>
#include "parameters.h"
#include "healthd.h"
#include "VERSION.h"

#define DEFAULT_SEC 5
#define PUSH_PORT 8505

void SIGHUP_handler(int);
void SIGTERM_handler(int);
void SIGALRM_handler(int);
void ReadConfigFile (const char *);
void ReadCurrentValues(void);
int GetAnswer(char *, char *);

#define MAX_LABELlen 63

int active[MAX_TYPES];
char label[MAX_TYPES][MAX_LABELlen+1];
int min_val[MAX_TYPES];
int max_val[MAX_TYPES];
int doWarn[MAX_TYPES];
int doFail[MAX_TYPES];
int warn_level[MAX_TYPES];
int warn_sent[MAX_TYPES];
int fail_level[MAX_TYPES];
int fail_sent[MAX_TYPES];
int fail_count[MAX_TYPES];
int new_value[MAX_TYPES];
char temp_warn[MAX_LABELlen+1];
char temp_fail[MAX_LABELlen+1];
char fan_warn[MAX_LABELlen+1];
char fan_fail[MAX_LABELlen+1];
char volt_warn[MAX_LABELlen+1];
char volt_fail[MAX_LABELlen+1];
int debug;
int local;
int count;
int tabout;
int iter;
enum eChip MonitorType;
char ConfigFile[MAXPATHLEN];
int ReReadConfigFile;
int ExitProgram;
int UseVbat;
int pushit;
int quiet;

/* 
 * Different options recognized by this program.
 */

enum Option {
  Temp0_active,
  Temp0_label,
  Temp0_min,
  Temp0_max,
  Temp0_doWarn,
  Temp0_doFail,

  Temp1_active,
  Temp1_label,
  Temp1_min,
  Temp1_max,
  Temp1_doWarn,
  Temp1_doFail,

  Temp2_active,
  Temp2_label,
  Temp2_min,
  Temp2_max,
  Temp2_doWarn,
  Temp2_doFail,

  Fan0_active,
  Fan0_label,
  Fan0_min,
  Fan0_max,
  Fan0_doWarn,
  Fan0_doFail,

  Fan1_active,
  Fan1_label,
  Fan1_min,
  Fan1_max,
  Fan1_doWarn,
  Fan1_doFail,

  Fan2_active,
  Fan2_label,
  Fan2_min,
  Fan2_max,
  Fan2_doWarn,
  Fan2_doFail,

  Volt0_active,
  Volt0_label,
  Volt0_min,
  Volt0_max,
  Volt0_doWarn,
  Volt0_doFail,

  Volt1_active,
  Volt1_label,
  Volt1_min,
  Volt1_max,
  Volt1_doWarn,
  Volt1_doFail,

  Volt2_active,
  Volt2_label,
  Volt2_min,
  Volt2_max,
  Volt2_doWarn,
  Volt2_doFail,

  Volt3_active,
  Volt3_label,
  Volt3_min,
  Volt3_max,
  Volt3_doWarn,
  Volt3_doFail,

  Volt4_active,
  Volt4_label,
  Volt4_min,
  Volt4_max,
  Volt4_doWarn,
  Volt4_doFail,

  Volt5_active,
  Volt5_label,
  Volt5_min,
  Volt5_max,
  Volt5_doWarn,
  Volt5_doFail,

  Volt6_active,
  Volt6_label,
  Volt6_min,
  Volt6_max,
  Volt6_doWarn,
  Volt6_doFail,

  Temp_warn,
  Temp_fail,

  Fan_warn,
  Fan_fail,

  Volt_warn,
  Volt_fail
};

enum Param {
  YesNo,
  Numeric,
  Float,
  String,
  None
};

/*
 * Option information structure (used by ParseOption).
 */

struct OptionInfo {
  enum Option	type;
  enum Param	parm;
  const char*	parmDescription;
  const char*	description;
  const char*	name;
  enum Param	doWarn;
  enum Param	doFail;
  const char*	shortName;
  const void*   value;
};

#include "optionTable.h"

void
SIGHUP_handler(int sig) {
  ReReadConfigFile = 1;
  signal(SIGHUP, SIGHUP_handler);
}

void
SIGTERM_handler(int sig) {
  ExitProgram = 1;
}

void
SIGALRM_handler(int sig) {
  /* do nothing. */
  signal(SIGALRM, SIGALRM_handler);
}

#define CHIP_STR_LEN 1024

int
main(int argc, char *argv[]) {
  int n;
  int ch;
  int method='I';
  int ConfigRead;
  char *DaemonName;
  char ChipInfoStr[CHIP_STR_LEN];
  unsigned int VendID;
  unsigned int ChipID;
  int LocalOnly;

  int sock;
  int sock6;
  int length;
  int ipv4_enable=1;
  struct sockaddr_in server;
  int msgsock;
  char buf[1024];
  char outbuf[1024];
  int rval;
  int selretval;
  int syserr;
  fd_set ready;
  struct servent *sp;
  unsigned long port;
  int psd = -1;
  struct sockaddr_in paddr;
  struct hostent *hent;

  struct itimerval tmo = {{0,0},{0,0}};
  struct itimerval tmooff = {{0,0},{0,0}};

  sock = 0;
  sock6 = 0;
  count = 0;
  tabout = 0;
  iter = 1;
  ConfigRead = 0;
  ReReadConfigFile = 0;
  ExitProgram = 0;
  UseVbat = 0;
  pushit = 0;
  quiet = 1;

  MonitorType = NO_CHIP;

  if ((sp = getservbyname("healthd", "tcp")) == NULL) {
    port = 1281;
  } else {
    port = ntohs(sp->s_port);
  }


  if (strchr(argv[0], '/')) {
    DaemonName = strrchr(argv[0], '/') + 1;
  } else {
    DaemonName=argv[0];
  }

  /*
   * Set some defaults
   */
  for (n=0; n<MAX_TYPES; n++) {
    active[n] = 0;
    fail_count[n] = 0;
    warn_level[n] = 2;
    warn_sent[n] = 0;
    fail_level[n] = 5;
    fail_sent[n] = 0;
    doWarn[n] = 0;
    doFail[n] = 0;
  }
  temp_warn[0] = '\0';
  temp_fail[0] = '\0';
  fan_warn[0] = '\0';
  fan_fail[0] = '\0';
  volt_warn[0] = '\0';
  volt_fail[0] = '\0';
  debug = 0;
  local = 0;
  LocalOnly = 0;

  while((ch=getopt(argc, argv, "12BDILSP:Vc:df:t:p:q")) != -1) {
    switch(ch){
    case '1':
      MonitorType = W83781D;
      break;

    case '2':
      MonitorType = W83782D;
      break;


    case 'B':
      UseVbat = 1;
      break;

    case 'D':
    case 'd':
      debug = 1;
      break;

    case 'I':
      method='I';
      break;

    case 'L':
      local = 1;
      break;

    case 'P':
      port = atoi(optarg);
      break;

    case 'S':
      method='S';
      break;

    case 'V':
      fprintf(stderr, "Version %s\n", hdVERSION);
      exit(0);
      break;

    case 't':
      tabout = 1;
      break;

    case 'p':
      if ((psd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        (void)fprintf(stderr, "%s: cannot create push socket: %s\n", DaemonName, strerror(errno));
        exit(1);
      }
      if (!(hent = gethostbyname(optarg))) {
        herror("Can't resolve hostname");
        exit(1);
      }
      bzero(&paddr, sizeof(struct sockaddr_in));
      paddr.sin_family = AF_INET;
      bcopy(hent->h_addr_list[0], &paddr.sin_addr.s_addr, sizeof(paddr.sin_addr.s_addr));
      paddr.sin_port = htons(PUSH_PORT);
      if (connect(psd, (struct sockaddr*)&paddr, sizeof(struct sockaddr_in)) < 0) {
        (void)fprintf(stderr, "%s: connect failed: %s\n", DaemonName, strerror(errno));
        exit(1);
      }
      pushit = 1;
      break;

    case 'c':
      count = atoi(optarg);
      if (count < 1) {
         count = 0;
      }
      debug = 1;
      break;

    case 'f':
      {
	int fd;

	if ((fd = open(optarg, O_RDONLY, 0)) < 0) {
	  (void)fprintf(stderr, "%s: %s: %s\n", DaemonName, optarg, strerror(errno));
	  exit(1);
	}
	close(fd);
	strncpy(ConfigFile, optarg, MAXPATHLEN);
	ReadConfigFile (ConfigFile);
	ConfigRead = 1;
      }
      break;

    case 'q':
      quiet = 0;
      break;
      
    default:
      fprintf(stderr, "Usage: %s -[1|2] -[I|S] [-dLV] [-f configfile] [-c|t count] <seconds for sleep>"\
	      " (default %d sec)\n", DaemonName, DEFAULT_SEC);
      exit(1);
    }
  } 
  if (ConfigRead == 0) {
    strncpy(ConfigFile, CONFIG_FILE, MAXPATHLEN);
    ReadConfigFile (ConfigFile);
    ConfigRead = 1;
  }

  if (argc > optind) {
    if((n = atoi(argv[optind])) > 0) {
      tmo.it_value.tv_sec = n;
    } else {
      fprintf(stderr, "Usage: %s -[1|2] -[I|S] [-dLV] [-f configfile] [-c|t count] <seconds for sleep>"\
	      " (default %d sec)\n", DaemonName, DEFAULT_SEC);
      exit(1);
    }
  } else {
    tmo.it_value.tv_sec = DEFAULT_SEC;
  }
  if (InitMBInfo(method) != 0) {
    perror("InitMBInfo");
    exit(1);
  }

  if (RstChip() < 0) {
    perror("RstChip");
    exit(1);
  }

  if (local || debug) {
    /* If debug mode or local mode don't open a socket */
    LocalOnly = 1;
  }
  VendID = GetVendorID();
  switch (VendID) {
  case 0x5CA3:
    ChipID = GetChipID_winbond();
    switch (ChipID) {
    case 0x10:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "WinBond Chip: W83781D");
      if (MonitorType == NO_CHIP)
	MonitorType = W83781D;
      break;

    case 0x11:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "Asus: AS97127F");
      if (MonitorType == NO_CHIP)
	MonitorType = W83781D;
      break;

    case 0x21:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "WinBond Chip: W83627HF");
      if (MonitorType == NO_CHIP)
	MonitorType = W83627HF;
      break;

    case 0x30:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "WinBond Chip: W83782D");
      if (MonitorType == NO_CHIP)
	MonitorType = W83782D;
      break;

    case 0x40:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "WinBond Chip: W83783S");
      if (MonitorType == NO_CHIP)
	MonitorType = W83783S;
      break;

    default:
      snprintf(ChipInfoStr, CHIP_STR_LEN, "WinBond Chip: (unknown)");
      if (MonitorType == NO_CHIP)
	MonitorType = W83781D;
      break;
    }
    break;

  case 0x12C3:
    snprintf(ChipInfoStr, CHIP_STR_LEN, "Asus: AS99127F");
    if (MonitorType == NO_CHIP)
      MonitorType = AS99127F;
    break;

  case 0xFFFF0040:
    snprintf(ChipInfoStr, CHIP_STR_LEN, "National Semi: LM78");
    if (MonitorType == NO_CHIP)
      MonitorType = LM78;
    break;

  case 0xFFFF00C0:
    snprintf(ChipInfoStr, CHIP_STR_LEN, "National Semi: LM79");
    if (MonitorType == NO_CHIP)
      MonitorType = LM79;
    break;

  default:
    snprintf(ChipInfoStr, CHIP_STR_LEN, "Unknown Vendor: ID = %X", VendID);
    if (MonitorType == NO_CHIP)
      MonitorType = W83781D;
    break;
  }
  if (debug) {
    printf("************************\n");
    printf("* Hardware Information *\n");
    printf("************************\n");
    printf("%s\n", ChipInfoStr);
    printf("************************\n\n");
  }

  (void) openlog(DaemonName, LOG_CONS|LOG_NDELAY, hdFACILITY);
  signal(SIGALRM, SIGALRM_handler);

  if (debug == 0) {
    /*
     * Okay so far.  Try and make ourself a daemon
     */
    if (daemon(0,0) < 0) {
      syslog(hdALERT, "daemon libray call failed: %m (aborting)");
      exit(2);
    }
    syslog(hdNOTICE, "Started");
  }

  if (debug == 0) {
    /* Don't set the signal handler if we are in debug mode */
    /* Since we don't check the ranges */
    signal(SIGHUP, SIGHUP_handler);
    signal(SIGTERM, SIGTERM_handler);
    siginterrupt(SIGALRM,1);
  }
  if (LocalOnly == 0) {
    /* Create name with wildcards. */
    if (ipv4_enable) {
      /* Create socket from which to read. */
      sock = socket(AF_INET, SOCK_STREAM, 0);
      if (sock < 0) {
        syslog(hdALERT, "opening datagram socket: %m");
	syslog(hdALERT, "Aborting");
        exit(1);
      }
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      server.sin_port = htons(port);
      if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) {
	close(sock);
        syslog(hdALERT, "binding datagram socket: %m");
	syslog(hdALERT, "Aborting");
        exit(1);
      }
      /* Find assigned port value and print it out. */
      length = sizeof(server);
      if (getsockname(sock, (struct sockaddr *)&server, &length)) {
	close(sock);
        syslog(hdALERT, "getting socket name: %m");
	syslog(hdALERT, "Aborting");
        exit(1);
      }
    }
  }
  ReadCurrentValues();
  if (LocalOnly == 0) {
    /* Start accepting connections */
    if (sock != 0) {
      listen(sock, 32);
    }
  }

  /* Main daemon loop */
  while ((iter<count) || (count==0)) {
    FD_ZERO(&ready);
    if (LocalOnly == 0) {
      if (sock != 0) {
        FD_SET(sock, &ready);
      }
    }

    /* Wait for connection, or timeout */
    setitimer(ITIMER_REAL,&tmo,0);
    selretval = select(getdtablesize(), &ready, 0, 0, 0);
    syserr = errno;
    setitimer(ITIMER_REAL,&tmooff,0);

    if ( ExitProgram ) {
      break;
    }

    switch (selretval) {
    case -1:
      if (syserr && syserr != EINTR) {
        syslog(hdCRITICAL, "OOPS! select returned a weird error, bailing out: %m");
        exit(1);
      }

      if (ReReadConfigFile) {
        /*
         * If we are updating the config file
         * then we should clear the error counts.
         */
        for (n=0; n<MAX_TYPES; n++) {
          active[n] = 0;
          fail_count[n] = 0;
          warn_level[n] = 2;
          warn_sent[n] = 0;
          fail_level[n] = 5;
          fail_sent[n] = 0;
        }
        ReadConfigFile (ConfigFile);
        ReReadConfigFile = 0;
        syslog(hdWARNING, "Restarted");
        break;
      }

      if (count > 0) {
        iter++;
      }
      
      ReadCurrentValues();
      if (pushit) {
        push_stat(psd);
      }
      break;

    default:
      if (LocalOnly == 0) {
        if (FD_ISSET(sock, &ready)) {
          msgsock = accept(sock, 0, 0);

          if (msgsock == -1) {
            /* Add error handling here */
          } else {
	    do {
	      bzero(buf, sizeof(buf));
	      bzero(outbuf, sizeof(outbuf));
	      if ((rval = read(msgsock, buf, 1024)) > 0) {
		rval = GetAnswer(buf, outbuf);
		write(msgsock, outbuf, strlen(outbuf));
	      }
	      else if (rval < 0) {
		break;
	      }
	    } while (rval != 0);
            close(msgsock);
          }
        }
      }
      break;
    }

    if (ExitProgram) {
      break;
    }
  }

  if (LocalOnly == 0) {
    close(sock);
  }
  if (debug == 0) {
    signal(SIGHUP, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
  }
  syslog(hdWARNING, "Exiting..");
  exit(0);
}

int
GetAnswer(char inbuf[], char outbuf[]) {
  if (strncmp(inbuf, "GET ", 4) == 0) {
    if (inbuf[4] == 'V') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '7')) {
	sprintf(outbuf, "%4.2f", (double)new_value[6+(int)inbuf[5]-(int)'1']/65536.0);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    }
    else if (inbuf[4] == 'T') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '3')) {
	sprintf(outbuf, "%4.1f", (double)new_value[0+(int)inbuf[5]-(int)'1']/65536.0);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    }
    else if (inbuf[4] == 'S') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '3')) {
	sprintf(outbuf, "%.4d", new_value[3+(int)inbuf[5]-(int)'1']/65536);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    } else {
      strcpy(outbuf, "ERROR: Unknown class\n");
    }
  }
  else if (strncmp(inbuf, "GTV ", 4) == 0) {
    if (inbuf[4] == 'V') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '7')) {
	int subscrpt = 6+(int)inbuf[5]-(int)'1';

	sprintf(outbuf, "%4.2f|%d|%d|%d|%d|", (double)new_value[subscrpt]/65536.0, active[subscrpt], fail_count[subscrpt], fail_sent[subscrpt], warn_sent[subscrpt]);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    }
    else if (inbuf[4] == 'T') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '3')) {
	int subscrpt = 0+(int)inbuf[5]-(int)'1';

	sprintf(outbuf, "%4.1f|%d|%d|%d|%d|", (double)new_value[subscrpt]/65536.0, active[subscrpt], fail_count[subscrpt], fail_sent[subscrpt], warn_sent[subscrpt]);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    }
    else if (inbuf[4] == 'S') {
      if (('1' <= inbuf[5]) && (inbuf[5] <= '3')) {
	int subscrpt = 3+(int)inbuf[5]-(int)'1';

	sprintf(outbuf, "%.4d|%d|%d|%d|%d|", new_value[subscrpt]/65536, active[subscrpt], fail_count[subscrpt], fail_sent[subscrpt], warn_sent[subscrpt]);
      } else {
	strcpy(outbuf, "ERROR: out of bounds\n");
      }
    } else {
      strcpy(outbuf, "ERROR: Unknown class\n");
    }
  }
  else if (strncmp(inbuf, "VER ", 4) == 0) {
    if (inbuf[4] == 'P') {
      sprintf(outbuf, "%s", hdPROTO);
    }
    else if (inbuf[4] == 'd') {
      sprintf(outbuf, "%s", hdVERSION);
    }
  }
  else if (strncmp(inbuf, "CFG ", 4) == 0) {
    int i;
    int max;
    struct OptionInfo* info;

    /*
     * Find option from table.
     */
    max = sizeof (optionTable) / sizeof (struct OptionInfo);
    for (i = 0, info = optionTable; i < max; i++, info++) {
      if (info->shortName) {
	if (!strncmp (info->shortName, inbuf+4, 6)) {
	  break;
	}
      }
    }
    if (i >= max) {
      strcpy(outbuf, "ERROR: Unknown class\n");
      return 1;
    }

    switch (info->parm) {
    case YesNo:
      if (*(int *)info->value) {
	strcpy(outbuf, "yes");
      } else {
	strcpy(outbuf, "no");
      }
      break;

    case Numeric:
      sprintf(outbuf, "%d", *(int *)info->value / 65536);
      break;

    case Float:
      sprintf(outbuf, "%5.2f", *(double *)info->value / 65536.0);
      break;

    case String:
      sprintf(outbuf, "%s", (char *)info->value);
      break;

    case None:
      strcpy(outbuf, "NONE");
      break;
    }
  }
  else if (strncmp(inbuf, "END", 3) == 0) {
    sprintf(outbuf, "Closing connection\n");
    return 0;
  } else {
    strcpy(outbuf, "ERROR: Unsupported command\n");
  }
  return 1;
}

#define CMD_BUFsz 1024

void
ReadCurrentValues(void) {
  double temp1 = 0.0, temp2 = 0.0, temp3 = 0.0;
  double vc0, vc1, v33, v50p, v12p, v12n, v50n;
  int rot1, rot2, rot3;
  int n;
  char message[128];
  char command[CMD_BUFsz+1];

  /* Temperature */
  getTemp(&temp1, &temp2, &temp3);
  if (debug) {
    if (tabout) {
      printf("%4.1f\t%4.1f\t%4.1f\t", temp1, temp2, temp3);
    } else {
      printf("Temp.= %4.1f, %4.1f, %4.1f;", temp1, temp2, temp3);
    }
  }
  new_value[0] = (int)(temp1 * 65536.0);
  new_value[1] = (int)(temp2 * 65536.0);
  new_value[2] = (int)(temp3 * 65536.0);

  /* Fan Speeds */
  getFanSp(&rot1, &rot2, &rot3);
  if (debug) {
    if (tabout) {
      printf("%4d\t%4d\t%4d\t", rot1, rot2, rot3);
    } else {
      printf(" Rot.= %4d, %4d, %4d\n", rot1, rot2, rot3);
    }
  }
  new_value[3] = rot1 * 65536;
  new_value[4] = rot2 * 65536;
  new_value[5] = rot3 * 65536;

  /* Voltages */
  getVolt(&vc0, &vc1, &v33, &v50p, &v50n, &v12p, &v12n);
  if (debug) {
    if (tabout) {
      printf("%4.2f\t%4.2f\t%4.2f\t%4.2f\t%5.2f\t%6.2f\t%5.2f\n", vc0, vc1, v33, v50p, v12p, v12n, v50n);
    } else {
      printf(" Vcore = %4.2f, %4.2f; Volt. = %4.2f, %4.2f, %5.2f, %6.2f, %5.2f\n", vc0, vc1, v33, v50p, v12p, v12n, v50n);
    }
  }
  new_value[6] = (int)(vc0 * 65536.0);
  new_value[7] = (int)(vc1 * 65536.0);
  new_value[8] = (int)(v33 * 65536.0);
  new_value[9] = (int)(v50p * 65536.0);
  new_value[10] = (int)(v12p * 65536.0);
  new_value[11] = (int)(v12n * 65536.0);
  new_value[12] = (int)(v50n * 65536.0);

  if (debug != 0) {
    /* If in debug mode, don't bounds check */
    return;
  }

  for (n=0; n<MAX_TYPES; n++) {
    if (active[n]) {
      if ((min_val[n] <= new_value[n]) && (new_value[n] <= max_val[n])) {
	fail_count[n] = 0;
	fail_sent[n] = 0;
	warn_sent[n] = 0;
      } else {
	sprintf(message, "A value of %.2f for %s with a range of (%.2f <= n <= %.2f)\n", (double)new_value[n]/65536.0, label[n], (double)min_val[n]/65536.0, (double)max_val[n]/65536.0);
	fail_count[n]++;
	if ((fail_count[n] >= fail_level[n]) && (fail_sent[n] == 0)) {
	  fail_sent[n] = 1;
          if (!quiet) {
            syslog(hdCRITICAL, "%s", message);
          }
	  if ((0 <= n) && (n <= 2)) {
	    if ((strlen(temp_fail) > 0) && (doFail[n])) {
	      snprintf(command, CMD_BUFsz, temp_fail, message);
	      system(command);
	    }
	  }
	  else if ((3 <= n) && (n <= 5)) {
	    if ((strlen(fan_fail) > 0) && (doFail[n])) {
	      snprintf(command, CMD_BUFsz, fan_fail, message);
	      system(command);
	    }
	  }
	  else if ((6 <= n) && (n <= 12)) {
	    if ((strlen(volt_fail) > 0) && (doFail[n])) {
	      snprintf(command, CMD_BUFsz, volt_fail, message);
	      system(command);
	    }
	  }
	}
	else if ((fail_count[n] >= warn_level[n]) && (warn_sent[n] == 0)) {
	  warn_sent[n] = 1;
          if (!quiet) {
            syslog(hdWARNING, "%s", message);
          }
	  if ((0 <= n) && (n <= 2)) {
	    if ((strlen(temp_warn) > 0) && (doWarn[n])) {
	      snprintf(command, CMD_BUFsz, temp_warn, message);
	      system(command);
	    }
	  }
	  else if ((3 <= n) && (n <= 5)) {
	    if ((strlen(fan_warn) > 0) && (doWarn[n])) {
	      snprintf(command, CMD_BUFsz, fan_warn, message);
	      system(command);
	    }
	  }
	  else if ((6 <= n) && (n <= 12)) {
	    if ((strlen(volt_warn) > 0) && (doWarn[n])) {
	      snprintf(command, CMD_BUFsz, volt_warn, message);
	      system(command);
	    }
	  }
	}
      }
    }
  }
}

static void 
ParseOption (const char* option, const char* parms, int cmdLine)
{
  int			i;
  struct OptionInfo*	info;
  int			yesNoValue;
  int			numValue;
  const char*		strValue;
  int			max;
  char*			end;
  /*
   * Find option from table.
   */

  max = sizeof (optionTable) / sizeof (struct OptionInfo);
  for (i = 0, info = optionTable; i < max; i++, info++) {

    if (!strcmp (info->name, option))
      break;

    if (info->shortName)
      if (!strcmp (info->shortName, option))
	break;
  }

  if (i >= max) {
    warnx ("unknown option on line #%d", cmdLine);
  }

  yesNoValue	= 0;
  numValue	= 0;
  strValue	= NULL;
  /*
   * Check parameters.
   */
  switch (info->parm) {
  case YesNo:
    if (!parms) {
      parms = "yes";
    }

    if (!strcmp (parms, "yes")) {
      yesNoValue = 1;
    } else {
      if (!strcmp (parms, "no")) {
	yesNoValue = 0;
      } else {
	errx (1, "%s needs yes/no parameter", option);
      }
    }
    break;

  case Numeric:
    if (parms) {
      numValue = strtol (parms, &end, 10) * 65536;
    } else {
      end = NULL;
    }

    if (end == parms) {
      errx (1, "%s needs numeric parameter", option);
    }
    break;

  case Float:
    if (parms) {
      numValue = (int)(strtod (parms, &end) * 65536.0);
    } else {
      end = NULL;
    }

    if (end == parms) {
      errx (1, "%s needs numeric parameter", option);
    }
    break;

  case String:
    strValue = parms;
    if (!strValue) {
      errx (1, "%s needs parameter", option);
    }
    break;

  case None:
    if (parms) {
      errx (1, "%s does not take parameters", option);
    }
    break;
  }
  switch (info->type) {
  case Temp0_active:
  case Temp1_active:
  case Temp2_active:
  case Fan0_active:
  case Fan1_active:
  case Fan2_active:
  case Volt0_active:
  case Volt1_active:
  case Volt2_active:
  case Volt3_active:
  case Volt4_active:
  case Volt5_active:
  case Volt6_active:
    active[info->type/6] = yesNoValue;
    break;

  case Temp0_label:
  case Temp1_label:
  case Temp2_label:
  case Fan0_label:
  case Fan1_label:
  case Fan2_label:
  case Volt0_label:
  case Volt1_label:
  case Volt2_label:
  case Volt3_label:
  case Volt4_label:
  case Volt5_label:
  case Volt6_label:
    strncpy(label[info->type/6], strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Temp0_min:
  case Temp1_min:
  case Temp2_min:
  case Fan0_min:
  case Fan1_min:
  case Fan2_min:
  case Volt0_min:
  case Volt1_min:
  case Volt2_min:
  case Volt3_min:
  case Volt4_min:
  case Volt5_min:
  case Volt6_min:
    min_val[info->type/6] = numValue;
    break;

  case Temp0_max:
  case Temp1_max:
  case Temp2_max:
  case Fan0_max:
  case Fan1_max:
  case Fan2_max:
  case Volt0_max:
  case Volt1_max:
  case Volt2_max:
  case Volt3_max:
  case Volt4_max:
  case Volt5_max:
  case Volt6_max:
    max_val[info->type/6] = numValue;
    break;

  case Temp0_doWarn:
  case Temp1_doWarn:
  case Temp2_doWarn:
  case Fan0_doWarn:
  case Fan1_doWarn:
  case Fan2_doWarn:
  case Volt0_doWarn:
  case Volt1_doWarn:
  case Volt2_doWarn:
  case Volt3_doWarn:
  case Volt4_doWarn:
  case Volt5_doWarn:
  case Volt6_doWarn:
    doWarn[info->type/6] = yesNoValue;
    break;

  case Temp0_doFail:
  case Temp1_doFail:
  case Temp2_doFail:
  case Fan0_doFail:
  case Fan1_doFail:
  case Fan2_doFail:
  case Volt0_doFail:
  case Volt1_doFail:
  case Volt2_doFail:
  case Volt3_doFail:
  case Volt4_doFail:
  case Volt5_doFail:
  case Volt6_doFail:
    doFail[info->type/6] = yesNoValue;
    break;

  case Temp_warn:
    strncpy(temp_warn, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Temp_fail:
    strncpy(temp_fail, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Fan_warn:
    strncpy(fan_warn, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Fan_fail:
    strncpy(fan_fail, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Volt_warn:
    strncpy(volt_warn, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;

  case Volt_fail:
    strncpy(volt_fail, strValue, ((strlen(strValue)>MAX_LABELlen)?strlen(strValue):MAX_LABELlen));
    break;
  }
}

void 
ReadConfigFile (const char* fileName)
{
  FILE*	file;
  char	buf[MAXPATHLEN];
  char*	ptr;
  char*	option;
  int LineNum;

  LineNum = 0;
  file = fopen (fileName, "r");
  if (!file) {
    snprintf (buf, sizeof(buf), "Cannot open config file %s.\n", fileName);
    fprintf(stderr, "%s", buf);
    exit (1);
  }

  while (fgets (buf, sizeof (buf), file)) {
    LineNum++;
    ptr = strchr (buf, '\n');
    if (!ptr) {
      errx (1, "Config line #%d too long!", LineNum);
    }
    
    *ptr = '\0';
    if (buf[0] == '#') {
      continue;
    }

    ptr = buf;
    /*
     * Skip white space at beginning of line.
     */
    while (*ptr && isspace (*ptr)) 
      ++ptr;

    if (*ptr == '\0')
      continue;
    /*
     * Extract option name.
     */
    option = ptr;
    while (*ptr && !isspace (*ptr))
      ++ptr;

    if (*ptr != '\0') {

      *ptr = '\0';
      ++ptr;
    }
    /*
     * Skip white space between name and parms.
     */
    while (*ptr && isspace (*ptr))
      ++ptr;

    ParseOption (option, *ptr ? ptr : NULL, LineNum);
  }

  fclose (file);
}

#define MTU 1024

int push_stat(int sd) {

  char buf[MTU];
  char *bufptr = buf;
  int i, numbytes;
  time_t cur_time;

  time(&cur_time);

  sprintf(bufptr, "%ld|", cur_time);
  bufptr = bufptr + strlen(bufptr);

  /* package up values. */
  for (i = 0; i < 3; ++i) {
    sprintf(bufptr, "%4.1f|", (double)new_value[i]/65536.0);
    bufptr = bufptr + strlen(bufptr);
  }

  for (i = 3; i < 6; ++i) {
    sprintf(bufptr, "%4d|", new_value[i]/65536);
    bufptr = bufptr + strlen(bufptr);
  }

  for (i = 6; i < MAX_TYPES; ++i) {
    sprintf(bufptr, "%6.2f|", (double)new_value[i]/65536.0);
    bufptr = bufptr + strlen(bufptr);
  }

  if (debug) {
    printf("buffer: %s\n", buf);
  }

  if ((numbytes = send(sd, buf, strlen(buf)+1, NULL)) < 0) {
    if (!quiet) {
      syslog(hdWARNING, "push_stat(): %m");
    }
  }
  else if (numbytes < strlen(buf)+1) {
    syslog(hdWARNING, "Runt packet pushed.");
  }

  free(buf);
  return 0;
}
