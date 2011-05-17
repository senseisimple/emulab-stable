/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "slothd.h"
#include "config.h"

SLOTHD_OPTS   *opts;
SLOTHD_PARAMS *parms;

void lerror(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_ERR, "%s: %m", msgstr);
    fprintf(stderr, "slothd: %s: %s\n", msgstr, strerror(errno));
  }
}

void lwarn(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_WARNING, "%s", msgstr);
    fprintf(stderr, "slothd: %s\n", msgstr);
  }
}

void lnotice(const char *msgstr) {
  if (msgstr) {
    syslog(LOG_NOTICE, "%s", msgstr);
    printf("slothd: %s\n", msgstr);
  }
}

void sigunkhandler(int signum) {
  int status;
  char message[50];

  sprintf(message, "Unhandled signal: %d.  Exiting.", signum);
  lerror(message);
  if (!opts->debug)
    unlink(PIDFILE);
  while (wait(&status) != -1);
  exit(signum);
}

void siginthandler(int signum) {
  parms->dolast = 1;
  signal(SIGTERM, siginthandler);
  signal(SIGINT, siginthandler);
  signal(SIGQUIT, siginthandler);
}

void usage(void) {  

  printf("Usage:\tslothd -h\n"
         "\tslothd [-o] [-a] [-d] [-i <interval>] [-p <port>] [-s <server>]\n"
         "\t       [-g <interval>] [-l <thresh>] [-c <thresh>]\n"
         "\t -h\t\t This message.\n"
         "\t -o\t\t Run once (collect a single report).\n"
         "\t -a\t\t Only scan active terminal special files.\n"
         "\t -d\t\t Debug mode; do not fork into background.\n"
         "\t -i <interval>\t Regular run interval, in seconds.\n"
         "\t -g <interval>\t Aggressive run interval, in seconds.\n"
         "\t -l <thresh>\t load threshold (default 1 @ 1 minute).\n"
         "\t -c <thresh>\t experimental network packet difference threshold (pps).\n"
         "\t -n <thresh>\t control network packet difference threshold (pps)\n"
         "\t -p <port>\t Send on port <port>\n"
         "\t -s <server>\t Send data to <server>\n");
}


int main(int argc, char **argv) {

  int exitcode = -1;
  u_int myabits, span;
  time_t curtime, lasttime = 0;
  static SLOTHD_OPTS mopts;
  static SLOTHD_PARAMS mparms;
  static SLOTHD_PACKET mpkt;
  static SLOTHD_PACKET mopkt;
  SLOTHD_PACKET *pkt, *opkt, *tmppkt;

  extern char build_info[];

  /* pre-init */
  bzero(&mopts, sizeof(SLOTHD_OPTS));
  bzero(&mparms, sizeof(SLOTHD_PARAMS));
  bzero(&mpkt, sizeof(SLOTHD_PACKET));
  bzero(&mopkt, sizeof(SLOTHD_PACKET));
  opts = &mopts;
  parms = &mparms;
  pkt = &mpkt;
  opkt = &mopkt;

  if (parse_args(argc, argv) < 0) {
    fprintf(stderr, "Error processing arguments.\n");
  }
  else {
    if (init_slothd() < 0) {
      lerror("Problem initializing, bailing out.");
    }
    else {
      exitcode = 0;
      lnotice("Slothd started");
      lnotice(build_info);
      for (;;) {
        
	time(&curtime);
	if (opts->debug) {
	  printf("The time is now %s", ctime(&curtime));
	  fflush(stdout);
	}

	if (curtime < lasttime) {
	  /*
	   * The clock was set back on us, most likely on Windows as NTP
	   * changed the clock from UTC time to local time.  Set the
	   * timestamps back by the same amount to compensate.
	   */
	  int delta = lasttime - curtime;
	  parms->startup -= delta;
	  mparms.lastrpt -= delta;
	  if (opts->debug) {
	    printf("The clock was set back by %d seconds.\n\n", delta);
	    fflush(stdout);
	  }
	}
	lasttime = curtime;

        /* Collect current machine stats */
        mparms.cnt++;
        get_load(pkt);
        get_min_tty_idle(pkt);
        get_packet_counts(pkt);
        myabits = get_active_bits(pkt,opkt);

        /*
         * Time to send a packet?
         * Yes, if:
         * 1) We've been idle, and now we see activity (aggressive mode)
         * 2) Its been over <reg_interval> seconds since the last report
         */
        if ((!opkt->actbits && pkt->actbits) ||
            (curtime >=  mparms.lastrpt + mopts.reg_interval) ||
            parms->dolast) {
          if (send_pkt(pkt)) {
            mparms.lastrpt = curtime;
          }
          if (parms->dolast) {
            do_exit();
          }
          tmppkt = pkt;
          pkt = opkt;
          opkt = tmppkt;
        }

        if (mopts.once) {
          break;
        }

        /* 
         * Figure out, based on run count, and activity, how long
         * to sleep.
         */
        if (mparms.cnt == 1) {
          span = mopts.reg_interval - 
            (rand() / (float) RAND_MAX) * (OFFSET_FRACTION*mopts.reg_interval);
        }
        else if (myabits) {
          span = mopts.reg_interval;
        }
        else {
          span = mopts.agg_interval;
        }
        
        if (opts->debug) {
          printf("About to sleep for %u seconds.\n\n", span);
          fflush(stdout);
        }

        if (parms->dolast) {
          continue;
        }

        sleep(span);
      }
    }
  }
  return exitcode;
}

int parse_args(int argc, char **argv) {

  int ch;

  /* setup defaults. */
  opts->once = 0;
  opts->reg_interval = DEF_RINTVL;
  opts->agg_interval = DEF_AINTVL;
  opts->debug = 0;
  opts->port = SLOTHD_DEF_PORT;
  opts->servname = BOSSNODE;
  opts->load_thresh = DEF_LTHRSH;
  opts->pkt_thresh = DEF_CTHRSH;
  opts->cif_thresh = DEF_CTHRSH;

  while ((ch = getopt(argc, argv, "oi:g:dp:s:l:c:n:h")) != -1) {
    switch (ch) {

    case 'o': /* run once */
      opts->once = 1;
      break;

    case 'i':
      if ((opts->reg_interval = atol(optarg)) < MIN_RINTVL) {
        lwarn("Warning!  Regular interval set too low, defaulting.");
        opts->reg_interval = MIN_RINTVL;
      }
      break;

    case 'g':
      if ((opts->agg_interval = atol(optarg)) < MIN_AINTVL) {
        lwarn("Warning! Aggressive interval set too low, defaulting.");
        opts->agg_interval = MIN_AINTVL;
      }
      break;

    case 'd':
      lnotice("Debug mode requested; staying in foreground.");
      opts->debug = 1;
      break;

    case 'p':
      opts->port = (u_short)atoi(optarg);
      break;

    case 's':
      if (optarg && *optarg) {
        opts->servname = strdup(optarg);
      }
      else {
        lwarn("Invalid server name, default used.");
      }
      break;

    case 'l':
      if ((opts->load_thresh = atof(optarg)) < MIN_LTHRSH) {
        lwarn("Warning! Load threshold set too low, defaulting.");
        opts->load_thresh = DEF_LTHRSH;
      }
      break;

    case 'c':
      if ((opts->pkt_thresh = atol(optarg)) < MIN_CTHRSH) {
        lwarn("Warning! Experimental net packet diff threshold too low, defaulting.");
        opts->pkt_thresh = DEF_CTHRSH;
      }
      break;

    case 'n':
      if ((opts->cif_thresh = atol(optarg)) < MIN_CTHRSH) {
        lwarn("Warning! Control net packet diff threshold too low, defaulting.");
        opts->cif_thresh = DEF_CTHRSH;
      }
      break;
      
    case 'h':
    default:
      usage();
      return -1;
      break;
    }
  }
  return 0;
}


int init_slothd(void) {

  int pfd;
  char pidbuf[10];
  struct hostent *hent;
  char *ciprog[] = {"control_interface", NULL};

  /* init internal vars */
  parms->dolast = 0;  /* init send-last-report-before-exiting variable */
  parms->numttys = 0; /* will enum terms in this func */
  parms->cnt = 0;     /* daemon iter count */
  parms->lastrpt = 0; /* the last time a report pkt was sent */
  parms->startup = time(NULL); /* Make sure we don't report < invocation */

  /* Setup signals */
  signal(SIGTERM, siginthandler);
  signal(SIGINT, siginthandler);
  signal(SIGQUIT, siginthandler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGBUS, sigunkhandler);
  signal(SIGSEGV, sigunkhandler);
  signal(SIGFPE, sigunkhandler);
  signal(SIGILL, sigunkhandler);
  signal(SIGSYS, sigunkhandler);

  /* Setup logging facil. */
  openlog("slothd", LOG_NDELAY, LOG_TESTBED);

  /* Setup path */
  if (setenv("PATH", SLOTHD_PATH_ENV, 1) < 0) {
    lerror("Couldn't set path env");
  }

  /* Seed the random number generator */
  srand(time(NULL));

  /* Grab control net iface */
  if (procpipe(ciprog, &grab_cifname, parms)) {
    lwarn("Failed to get control net iface name");
  }

#ifdef __linux__
  /* Open socket for SIOCGHWADDR ioctl (to get mac addresses) */
  parms->ifd = socket(PF_INET, SOCK_DGRAM, 0);
#endif

  /* prepare UDP connection to server */
  if ((parms->sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    lerror("Could not alloc socket");
    return -1;
  }
  if (!(hent = gethostbyname(opts->servname))) {
    lerror("Can't resolve server hostname"); /* XXX use herror */
    return -1;
  }
  bzero(&parms->servaddr, sizeof(struct sockaddr_in));
  parms->servaddr.sin_family = AF_INET;
  parms->servaddr.sin_port = htons(opts->port);
  bcopy(hent->h_addr_list[0], &parms->servaddr.sin_addr.s_addr, 
        sizeof(struct in_addr));
  if (connect(parms->sd, (struct sockaddr*)&parms->servaddr, 
              sizeof(struct sockaddr_in)) < 0) {
    lerror("Couldn't connect to server");
    return -1;
  }

  /* Daemonize, unless in debug, or once-only mode. */
  if (!opts->debug && !opts->once) {
    if (daemon(0,0) < 0) {
      lerror("Couldn't daemonize");
      return -1;
    }
    /* Try to get lock.  If can't, then bail out. */
    if ((pfd = open(PIDFILE, O_EXCL | O_CREAT | O_RDWR, 0644)) < 0) {
      lerror("Can't create lock file.");
      return -1;
    }
    fchmod(pfd, S_IRUSR | S_IRGRP | S_IROTH);
    sprintf(pidbuf, "%d", getpid());
    write(pfd, pidbuf, strlen(pidbuf));
    close(pfd);
  }
  return 0;
}


void do_exit(void) {
  int status;
  if (!opts->debug)
    unlink(PIDFILE);
  while (wait(&status) != -1);
  lnotice("exiting.");
  exit(0);
}

int grab_cifname(char *buf, void *data) {

  int retval = -1;
  char *tmpptr;
  SLOTHD_PARAMS *myparms = (SLOTHD_PARAMS*) data;

#ifdef __CYGWIN__
  if (buf && isalnum(buf[0])) {

    /* The MAC addr precedes the cifname on Cygwin. */
    tmpptr = myparms->cifaddr = strdup(buf);
    strsep(&tmpptr, " ");

    /* Allow embedded spaces in the cifname on Cygwin. */    
    myparms->cifname = tmpptr;
    while ( isalnum(*tmpptr) || *tmpptr == ' ') tmpptr++;
    *tmpptr = '\0';
#else
    /* Trim trailing whitespace from the cifname. */
  if (buf && isalpha(buf[0])) {
    tmpptr = myparms->cifname = strdup(buf);
    while (isalnum(*tmpptr)) tmpptr++;
    *tmpptr = '\0';
#endif /* __CYGWIN__ */

    retval = 0;
  }
  else {
    myparms->cifname = NULL;
  }

  return retval;
}


int add_tty(char *path) {

  if (!path) {
    return 0;
  }

  strncpy(parms->ttys[parms->numttys], path, MAXDEVLEN);
  parms->numttys++;

  return 1;
}

int clear_ttys() {

  parms->numttys = 0;
  return 1;
}   

int enum_ttys() {

#ifndef __CYGWIN__
  DIR *devs;
  char bufstr[MAXDEVLEN];
  struct dirent *dptr;
#endif /* __CYGWIN__ */

  clear_ttys();

#ifndef __CYGWIN__
  /* enum tty special files */
  if ((devs = opendir("/dev")) == 0) {
    lerror("Can't open directory /dev for processing");
    return -1;
  }

  add_tty("/dev/console");

#ifdef __linux__  
  /* 
     Include the pts mux device to check for activity on
     dynamically allocated linux pts devices:
     (/dev/pts/<num>)
  */
  add_tty("/dev/ptmx");
#endif

  while (parms->numttys < MAXTTYS && (dptr = readdir(devs))) {
    if (strstr(dptr->d_name, "tty") || strstr(dptr->d_name, "pty")) {
      snprintf(bufstr, MAXDEVLEN, "/dev/%s", dptr->d_name);
      add_tty(bufstr);
    }
  }
  closedir(devs);

#else /* __CYGWIN__ */

  /* On Cygwin, `tty` returns /dev/console under RDP, and /dev/tty$n under SSH.
   * However, stat on anything under /dev always returns the current time, so
   * it's no help detecting user input.  Instead, we patch sshd to change the
   * modtime on a file when input is received and stat that.
   */
  add_tty("/var/run/ssh_input");

  /* Likewise, idlemon is started up at user login on Windows to monitor
   * desktop events for slothd under RDP logins and touches a time tag file.
   */
  add_tty("/var/run/rdp_input");

#endif /* __CYGWIN__ */

  return 1;
}

int send_pkt(SLOTHD_PACKET *pkt) {

  int i, numsent, retval = 1;
  static char pktbuf[1500];
  static char minibuf[50];

  /* flatten data into packet buffer */
  sprintf(pktbuf, "vers=%s mis=%lu lave=%.10f,%.10f,%.10f abits=0x%x ",
          SDPROTOVERS,
          pkt->minidle,
          pkt->loadavg[0],
          pkt->loadavg[1],
          pkt->loadavg[2],
          pkt->actbits);
  
  /* get all the interfaces too */
  for (i = 0; i < pkt->ifcnt; ++i) {
    sprintf(minibuf, "iface=%s,%lu,%lu ",
            pkt->ifaces[i].addr,
            pkt->ifaces[i].ipkts,
            pkt->ifaces[i].opkts);
    strcat(pktbuf, minibuf);
  }
  
  if (opts->debug) {
    printf("packet: %s\n", pktbuf);
  }
  
  /* send it */
  else {
    if ((numsent = send(parms->sd, pktbuf, strlen(pktbuf)+1, 0)) < 0) {
      lerror("Problem sending slothd packet");
      retval = 0;
    }
    
    else if (numsent < strlen(pktbuf)+1) {
      lwarn("Warning!  Slothd packet truncated");
      retval = 0;
    }
  }

  return retval;
}


void get_min_tty_idle(SLOTHD_PACKET *pkt) {
  int i;
  time_t mintime = 0;
  struct stat sb;

  enum_ttys();

  for (i = 0; i < parms->numttys; ++i) {
    if (stat(parms->ttys[i], &sb) < 0) {
      fprintf(stderr, "Can't stat %s:  %s\n", 
              parms->ttys[i], strerror(errno)); /* XXX change. */
    }
    else {
#ifndef __CYGWIN__
      /* The time of last reading keyboard input. */
      time_t tty_time = sb.st_atime;
      if (opts->debug)
	printf("Input on %s: %s", parms->ttys[i], ctime(&tty_time));
#else
      /* We're modding time tag files on Cygwin, not reading them. */
      time_t tty_time = sb.st_mtime;

      /* Ignore a time tag from the future when the clock gets set back. */
      if ( tty_time > time(NULL) ) {
	if (opts->debug)
	  printf("Ignored future time on %s: %s", 
		 parms->ttys[i], ctime(&tty_time));
	tty_time = mintime;
      }
#endif
      if (opts->debug)
	printf("Input on %s: %s", parms->ttys[i], ctime(&tty_time));
      if (tty_time > mintime) {
	mintime = tty_time;
      }
    }
  }

  /* Assign TTY activity, ensuring we don't go older than initial startup. */
  pkt->minidle = (mintime > parms->startup) ? mintime : parms->startup;
  if (opts->debug)
    printf("Minidle: %s", ctime(&mintime));
  return;
}

#ifndef __CYGWIN__

void get_load(SLOTHD_PACKET *pkt) {

  int retval;

  if ((retval = getloadavg(pkt->loadavg, 3)) < 0 || retval < 3) {
    lerror("unable to obtain load averages");
    pkt->loadavg[0] = pkt->loadavg[1] = pkt->loadavg[2] = -1;
  }
  else if (opts->debug) {
    printf("load averages: %f, %f, %f\n", pkt->loadavg[0], pkt->loadavg[1], 
           pkt->loadavg[2]);
  }
  return;
}

#else /* ifndef __CYGWIN__ */

/* A perfmon log is dribbling out the processor non-idle load average once a
 * minute.  We would rather have something more like a Unix run-queue length,
 * but the System/Processor Queue Length counter is sampled rather than
 * averaged by system logging so it's no good for us.  As long as we set our
 * threshold of CPU busyness below 1.0, this will work fine.
 */
char *ldavg_prog[] = {"tail", "-1", "/var/run/ldavg.csv", NULL};
int get_ldavg(char *, void *);
int ncpu = -1;

void get_load(SLOTHD_PACKET *pkt) {
  pkt->loadavg[0] = pkt->loadavg[1] = pkt->loadavg[2] = -1.0;

  if ( ncpu < 0 ) {
    /* Hyper-threading on the pc3000's convinces NT that it has 2 processors,
     * so it considers a busy process to be 50% utilization.  One busy process
     * should be 100% load average for us.  Use "cpu count" in /proc/cpuinfo
     * to get a multiplier.
     */
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    char line[80];

    while (ncpu < 0 && fgets(line, 80, cpuinfo) 
	   && ! feof(cpuinfo) && ! ferror(cpuinfo))
      sscanf(line, "cpu count : %d", &ncpu);

    if (ncpu < 1)    /* One-cpu machines have a cpu count of zero. */
      ncpu = 1;
  }

  if (procpipe(ldavg_prog, &get_ldavg, (void*)pkt))
    lwarn("get_ldavg exec failed.");
}

/* Process the output of the ldavg_prog command. */
int get_ldavg(char *buf, void *data) {
  SLOTHD_PACKET *pkt = (SLOTHD_PACKET*)data;
  double load = -1.0;

  if (sscanf(buf, "%*[^,],\"%lf\"", &load) != 1) {
    lerror("unable to obtain processor load.");
    return -1;
  }

  /* NT reports load as a percent, e.g. 44.5; we want a fraction, e.g. 0.445 */
  pkt->loadavg[0] = pkt->loadavg[1] = pkt->loadavg[2] = load * ncpu / 100.0;
  if (opts->debug)
    printf("load averages: %f, %f, %f\n", 
           pkt->loadavg[0], pkt->loadavg[1], pkt->loadavg[2]);

  return 0;
}
  
#endif /* __CYGWIN__ */

int get_active_bits(SLOTHD_PACKET *pkt, SLOTHD_PACKET *opkt) {

  u_long od, id;
  int i;

  pkt->actbits = 0;

  if (pkt->minidle > opkt->minidle) {
    pkt->actbits |= TTYACT;
  }
  else {
    pkt->actbits &= ~TTYACT;
  }

  /* XXX: whats the best way to compare load averages to a threshold? */
  if (pkt->loadavg[0] > opts->load_thresh) {
    pkt->actbits |= LOADACT;
  }
  else {
    pkt->actbits &= ~LOADACT;
  }

  /* 
   * Have the packet counters exceeded the threshold?  Make sure we don't
   * count the incoming packets on the control net interface.
   */
  for (i = 0; i < pkt->ifcnt; ++i) {
    if (strcmp(parms->cifname, pkt->ifaces[i].ifname) == 0) {
      if ((od = pkt->ifaces[i].opkts - opkt->ifaces[i].opkts) >= 
          opts->cif_thresh) {
        if (opts->debug) {
          printf ("Packet threshold exceeded on %s interface %s, %lu>=%lu\n",
                  "ctl net", pkt->ifaces[i].ifname, od, opts->cif_thresh);
        }
        break;
      }
    }
    else if (((od = pkt->ifaces[i].opkts - opkt->ifaces[i].opkts) >= 
              opts->pkt_thresh) || 
             ((id = pkt->ifaces[i].ipkts - opkt->ifaces[i].ipkts) >= 
              opts->pkt_thresh)) {
      printf ("Packet threshold exceeded on %s interface %s, %lu|%lu>=%lu\n",
              "exp net", pkt->ifaces[i].ifname, od, id, opts->pkt_thresh);
      break;
    }
  }

  if (i < pkt->ifcnt) {
    pkt->actbits |= PKTACT;
  }
  else {
    pkt->actbits &= ~PKTACT;
  }

  if (opts->debug) {
    printf("Active bits: 0x%04x\n", pkt->actbits);
  }

  return pkt->actbits;
}


#ifndef __CYGWIN__

void get_packet_counts(SLOTHD_PACKET *pkt) {
  int i;
  char *niprog[] = {"netstat", "-ni", NULL};

  pkt->ifcnt = 0;

  if (procpipe(niprog, &get_counters, (void*)pkt)) {
    lwarn("Netinfo exec failed.");
    pkt->ifcnt = 0;
  }
  else if (opts->debug) {
    for (i = 0; i < pkt->ifcnt; ++i) {
      printf("IFACE: %s  ipkts: %ld  opkts: %ld\n", 
             pkt->ifaces[i].ifname, 
             pkt->ifaces[i].ipkts,
             pkt->ifaces[i].opkts);
    }
  }
  return;
}

#ifdef __FreeBSD__
int get_counters(char *buf, void *data) {

  SLOTHD_PACKET *pkt = (SLOTHD_PACKET*)data;

  if (pkt->ifcnt < MAXNUMIFACES
      && !strstr(buf, "lo")
#if __FreeBSD__ >= 5
      && !strstr(buf, "plip")
#endif
      && !strstr(buf, "*")
      && strstr(buf, "<Link"))
  {
    if (sscanf(buf, "%s %*s %*s %s %lu %*s %lu",
               pkt->ifaces[pkt->ifcnt].ifname,
               pkt->ifaces[pkt->ifcnt].addr,
               &pkt->ifaces[pkt->ifcnt].ipkts,
               &pkt->ifaces[pkt->ifcnt].opkts) != 4) {
      printf("Failed to parse netinfo output.\n");
      return -1;
    }
    pkt->ifcnt++;
  }
  return 0;
}
#endif  /* __FreeBSD__ */

#ifdef __linux__
int get_counters(char *buf, void *data) {

  SLOTHD_PACKET *pkt = (SLOTHD_PACKET*)data;
  struct ifreq ifr;
  bzero(&ifr, sizeof(struct ifreq));

  if (pkt->ifcnt < MAXNUMIFACES
      && !strstr(buf, "lo")
      && (strstr(buf, "eth") || strstr(buf, "wlan") || strstr(buf, "ath"))) {

    if (sscanf(buf, "%s %*s %*s %lu %*s %*s %*s %lu",
               pkt->ifaces[pkt->ifcnt].ifname,
               &pkt->ifaces[pkt->ifcnt].ipkts,
               &pkt->ifaces[pkt->ifcnt].opkts) != 3) {
      printf("Failed to parse netinfo output.\n");
      return -1;
    }
    strcpy(ifr.ifr_name, pkt->ifaces[pkt->ifcnt].ifname);
    if (ioctl(parms->ifd, SIOCGIFHWADDR, &ifr) < 0) {
      perror("error getting HWADDR");
      return -1;
    }

    strcpy(pkt->ifaces[pkt->ifcnt].addr, 
           ether_ntoa((struct ether_addr*)&ifr.ifr_hwaddr.sa_data[0]));

    if (opts->debug) {
      printf("macaddr: %s\n", pkt->ifaces[pkt->ifcnt].addr);
    }
    pkt->ifcnt++;
  }
  return 0;
}
#endif  /* __linux__ */

#else /* __CYGWIN__ */

void get_packet_counts(SLOTHD_PACKET *pkt) {
  static DWORD dwSize;
  DWORD ret;
  int i;

  /* Call GetIfTable(), an MS IP Helper Function, to get packet counters. */
  PMIB_IFTABLE iftable;
  PMIB_IFROW ifrow;
  dwSize = sizeof(MIB_IFTABLE);
  iftable = (PMIB_IFTABLE) malloc(dwSize);
  while ((ret = GetIfTable(iftable, &dwSize, 0)) == ERROR_INSUFFICIENT_BUFFER)
     iftable = (PMIB_IFTABLE) realloc(iftable, dwSize);
  if (ret != NO_ERROR) {
    char msg[LINEBUFLEN];
    sprintf(msg, "get_packet_counts: GetIfTable error %lu", ret);
    free(iftable);
    lerror(msg);
  }

  /* Scan through the interface table. */
  pkt->ifcnt = 0;
  for (i = 0; i < min(iftable->dwNumEntries, MAXNUMIFACES); i++) {
    ifrow = &(iftable->table[i]);

    if (ifrow->dwType != MIB_IF_TYPE_LOOPBACK && /* Exclude loopback. */
        ifrow->dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL) {

      /* Format the MAC address.  We don't have ether_ntoa(). */
      snprintf(pkt->ifaces[pkt->ifcnt].addr, MACADDRLEN, 
               "%02x:%02x:%02x:%02x:%02x:%02x", 
               ifrow->bPhysAddr[0], ifrow->bPhysAddr[1], ifrow->bPhysAddr[2],
               ifrow->bPhysAddr[3], ifrow->bPhysAddr[4], ifrow->bPhysAddr[5]);
      if (opts->debug)
        printf("macaddr: %s\n", pkt->ifaces[pkt->ifcnt].addr);

      /* Grumble.  ifrow->wszName is empty.  
       * Recognize the control interface by its MAC address.
       */
      if (strncmp(pkt->ifaces[pkt->ifcnt].addr, parms->cifaddr, MACADDRLEN)==0)
        strncpy(pkt->ifaces[pkt->ifcnt].ifname, parms->cifname, MAXIFNAMELEN);
      else
        /* If it isn't the control interface, just show the MAC address. */
        strncpy(pkt->ifaces[pkt->ifcnt].ifname, parms->cifaddr, MAXIFNAMELEN);
      if (opts->debug)
        printf("ifname: %s\n", pkt->ifaces[pkt->ifcnt].ifname);

      /* Packet counters, including unicast, broadcast, and multicast. */
      pkt->ifaces[pkt->ifcnt].ipkts = 
        ifrow->dwInUcastPkts + ifrow->dwInNUcastPkts;
      pkt->ifaces[pkt->ifcnt].opkts = 
        ifrow->dwOutUcastPkts + ifrow->dwOutNUcastPkts;

      pkt->ifcnt++;
    }
  }
  free (iftable);
}
#endif /* __CYGWIN__ */


/* XXX change to combine last return value of procfunc with exec'ed process'
   exit status & write macros for access.
*/
int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data) {

  int fdes[2], retcode, cpid, status;
  char buf[LINEBUFLEN];
  FILE *in;

  if ((retcode=pipe(fdes)) < 0) {
    lerror("Couldn't alloc pipe");
  }
  
  else {
    switch ((cpid = fork())) {
    case 0:
      close(fdes[0]);
      dup2(fdes[1], STDOUT_FILENO);
      if (execvp(prog[0], prog) < 0) {
        lerror("Couldn't exec program");
        exit(1);
      }
      break;
      
    case -1:
      lerror("Error forking child process");
      close(fdes[0]);
      close(fdes[1]);
      retcode = -1;
      break;
      
    default:
      close(fdes[1]);
      in = fdopen(fdes[0], "r");
      while (!feof(in) && !ferror(in)) {
        if (fgets(buf, sizeof(buf), in)) {
          if ((retcode = procfunc(buf, data)) < 0)  break;
        }
      }
      fclose(in);
      wait(&status);
      if (retcode > -1)  retcode = WEXITSTATUS(status);
      break;
    } /* switch ((cpid = fork())) */
  }
  return retcode;
}
