/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "canaryd.h"
#include "config.h"

CANARYD_OPTS   *opts;
CANARYD_PARAMS *parms;

void lerror(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_ERR, "%s: %m", msgstr);
    fprintf(stderr, "canaryd: %s: %s\n", msgstr, strerror(errno));
  }
}

void lwarn(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_WARNING, "%s", msgstr);
    fprintf(stderr, "canaryd: %s\n", msgstr);
  }
}

void lnotice(const char *msgstr) {
  if (msgstr) {
    syslog(LOG_NOTICE, "%s", msgstr);
    printf("canaryd: %s\n", msgstr);
  }
}

void sigunkhandler(int signum) {
  int status;
  char message[50];

  sprintf(message, "Unhandled signal: %d.  Exiting.", signum);
  lerror(message);
  unlink(PIDFILE);
  while (wait(&status) != -1);
  exit(signum);
}

void siginthandler(int signum) {
  parms->dolast = 1;
}

void sigalrmhandler(int signum) {
  lnotice("Run length has expired; exiting.");
  parms->dolast = 1;
}

void usage(void) {  

  printf("Usage:\tcanaryd -h\n"
         "\tcanaryd [-o] [-d] [-i <interval>] [-p <port>] [-s <server>]\n"
         "\t        [-l] [-t <runlen>] [-r <interval>]\n"
         "\t -h\t\t This message.\n"
         "\t -o\t\t Run once (collect a single report).\n"
         "\t -d\t\t Debug mode; do not fork into background.\n"
         "\t -i <interval>\t Regular run interval, in seconds.\n"
         "\t -p <port>\t Send on port <port>\n"
         "\t -s <server>\t Send data to <server>\n"
         "\t -l\t\t Run in logging mode.  Will log to: " DEF_CDLOG "\n"
         "\t -t <runlen>\t Run for <runlen> seconds.\n"
         "\t -r <interval>\t Report overload at most every <interval> seconds \n");
  exit(0);
}


int main(int argc, char **argv) {

  int exitcode = -1;
  u_int span;
  time_t curtime;
  int rnd_off = 0;
  static CANARYD_OPTS mopts;
  static CANARYD_PARAMS mparms;
  static CANARYD_PACKET mpkt;
  CANARYD_PACKET *pkt;

  extern char build_info[];

  /* pre-init */
  bzero(&mopts, sizeof(CANARYD_OPTS));
  bzero(&mparms, sizeof(CANARYD_PARAMS));
  bzero(&mpkt, sizeof(CANARYD_PACKET));
  opts = &mopts;
  parms = &mparms;
  pkt = &mpkt;

  if (parse_args(argc, argv) < 0) {
    fprintf(stderr, "Error processing arguments.\n");
    return exitcode;
  }

  if (init_canaryd() < 0) {
    lerror("Problem initializing, bailing out.");
    do_exit(exitcode);
  }

  get_vmem_stats(pkt); /* XXX: doesn't belong here.. */
  exitcode = 0;
  lnotice("Canaryd started");
  lnotice(build_info);

  for (;;) {
    curtime = time(0);

    /* Collect current machine stats */
    mparms.cnt++;
    get_load(pkt);
    get_packet_counts(pkt);
    get_vnode_stats(pkt);
    get_vmem_stats(pkt);
    check_overload(pkt);
    
    if (pkt->overload && parms->numvnodes/2 > 1) {
      // (curtime >= mparms.lastolrpt + mopts.ol_rep_interval)) {
      send_ol_event(pkt);
      mparms.lastolrpt = curtime;
    }

    /* Poll the event system */
    event_poll(mparms.ev_handle);

    /*
     * Time to send a packet?
     * Yes, if:
     * 1) We've been idle, and now we see activity (aggressive mode)
     * 2) Its been over <reg_interval> seconds since the last report
     */
    if ((curtime >=  mparms.lastrpt + mopts.interval) ||
        parms->dolast) {
      if (send_pkt(pkt)) {
        mparms.lastrpt = curtime;
      }
      if (parms->dolast) {
        break;
      }
    }

    if (mopts.once) {
      break;
    }
    
    /* 
     * Figure out, based on run count, and activity, how long
     * to sleep.
     */
    if (mopts.log) {
      span = mopts.interval;
    }
    /* randomly offset the first packet to avoid packet implosion. */
    else if (mparms.cnt == 1) {
      rnd_off = (rand() / (float) RAND_MAX) * 
        (OFFSET_FRACTION*mopts.interval);
      rnd_off = (rnd_off > 0) ? rnd_off : 0;
      span = mopts.interval - rnd_off;
    }
    else {
      span = mopts.interval;
    }
    
    if (opts->debug) {
      printf("About to sleep for %u seconds.\n", span);
      fflush(stdout);
    }
    
    if (parms->dolast) {
      continue;
    }
    
    sleep(span);
  }

  return exitcode; /* NOTREACHED */
}

int parse_args(int argc, char **argv) {

  char ch;

  /* setup defaults. */
  opts->once = 0;
  opts->interval = DEF_INTVL;
  opts->debug = 0;
  opts->port = CANARYD_DEF_PORT;
  opts->servname = BOSSNODE;
  opts->log = 0;
  opts->run_len = 0;
  opts->ol_rep_interval = DEF_OLREPINTVL;

  while ((ch = getopt(argc, argv, "oi:g:dp:s:lt:r:h")) != -1) {
    switch (ch) {

    case 'o': /* run once */
      opts->once = 1;
      break;

    case 'i':
      if ((opts->interval = atol(optarg)) < MIN_INTVL) {
        lwarn("Warning!  Tnterval set too low, defaulting.");
        opts->interval = MIN_INTVL;
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
      lnotice("Logging mode enabled");
      opts->log = 1;
      break;

    case 't':
      opts->run_len = strtoul(optarg, NULL, 0);
      break;
      

    case 'r':
      opts->ol_rep_interval = strtoul(optarg, NULL, 0);
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


int init_canaryd(void) {

  int pfd, i;
  char pidbuf[10];
  char servbuf[MAXHNAMELEN];
  struct hostent *hent;
  char *ciprog[] = {"control_interface", NULL};
  char *stprog[] = {"tmcc", "status", NULL};
  struct rtprio myprio = {RTP_PRIO_REALTIME, 0};
  char *canaryd_evts = TBDB_EVENTTYPE_START ", " TBDB_EVENTTYPE_STOP ", "
                      TBDB_EVENTTYPE_RESET ", " TBDB_EVENTTYPE_REPORT;
  address_tuple_t tuple;
  char *driveargv[] = {"ad0", NULL};

  /* init internal vars */
  parms->dolast = 0;  /* init send-last-report-before-exiting variable */
  parms->cnt = 0;     /* daemon iter count */
  parms->lastrpt = 0; /* the last time a report pkt was sent */
  parms->startup = time(NULL); /* Make sure we don't report < invocation */
  parms->pideid = "";
  parms->ev_server = "boss.emulab.net"; /* XXX */
  parms->ol_detect = 0;
  parms->send_ev_report = 0;
  parms->numvnodes = 0;

  /* Event arg items */
  for (i = 0; i < NUMRANGES; ++i) {
    parms->olr[i].min = evargitems[i].defmin;
    parms->olr[i].max = evargitems[i].defmax;
  }

  /* Setup signals */
  signal(SIGTERM, siginthandler);
  signal(SIGINT, siginthandler);
  signal(SIGQUIT, siginthandler);
  signal(SIGALRM, sigalrmhandler);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGBUS, sigunkhandler);
  signal(SIGSEGV, sigunkhandler);
  signal(SIGFPE, sigunkhandler);
  signal(SIGILL, sigunkhandler);
  signal(SIGSYS, sigunkhandler);

  /* Set the priority to realtime */
  if (rtprio(RTP_PRIO_REALTIME, getpid(), &myprio) < 0) {
    lerror("Couldn't set RT priority!");
  }

  /* Setup logging facil. */
  openlog("canaryd", LOG_NDELAY, LOG_TESTBED);

  /* Setup path */
  if (setenv("PATH", CANARYD_PATH_ENV, 1) < 0) {
    lerror("Couldn't set path env");
  }

  /* Seed the random number generator */
  srand(time(NULL));

  /* Grab control net iface */
  if (procpipe(ciprog, &grab_cifname, parms)) {
    lwarn("Failed to get control net iface name");
  }

  /* Get the expt pid/eid. */
  if (procpipe(stprog, &grab_pideid, parms)) {
    lwarn("Failed to get pid/eid");
  }

  /* Open the logfile, if necessary */
  if (opts->log) {
    if ((parms->log = fopen(DEF_CDLOG, "w")) == NULL) {
      lerror("Failed to open log file");
      return -1;
    }
  }

  /************************************************************
   *                REGISTER WITH EVENT SYSTEM                *
   ************************************************************/

  /*
   * Find the local monitor node 
   */
#ifdef COMMENT /* XXX: fixup to use expt-specific mon node's elvind */
  if ((ssfile = fopen(SYNCSERVFILE, "r")) == NULL) {
    lerror("Failed to open sync server ident file");
    return -1;
  }

  if ((fgets(servbuf, sizeof(servbuf), ssfile) == NULL) ||
      ! *servbuf) {
    lerror("No sync server name found in file!");
    fclose(ssfile);
    return -1;
  }

  if (servbuf[strlen(servbuf)-1] == '\n') {
    servbuf[strlen(servbuf)-1] = '\0';
  }
  parms->ev_server = strdup(servbuf);
  fclose(ssfile);
#endif

  /*
   * Convert server/port to elvin thing.
   *
   * XXX This elvin string stuff should be moved down a layer. 
   */

  if (parms->ev_server) {
    snprintf(servbuf, sizeof(servbuf), "elvin://%s",
             parms->ev_server);
    /* free(parms->ev_server); */
    parms->ev_server = strdup(servbuf);
  }

  /*
   * Construct an address tuple for subscribing to events for
   * this node.
   */
  tuple = address_tuple_alloc();
  if (tuple == NULL) {
    lerror("could not allocate an address tuple");
    return -1;
  }

  printf("pid/eid: %s\n", parms->pideid);

  /*
   * Ask for canaryd agent events
   */
  tuple->host	   = ADDRESSTUPLE_ANY;
  tuple->site      = ADDRESSTUPLE_ANY;
  tuple->group     = ADDRESSTUPLE_ANY;
  tuple->expt      = parms->pideid;
  tuple->objtype   = TBDB_OBJECTTYPE_CANARYD;
  tuple->objname   = ADDRESSTUPLE_ANY;
  tuple->eventtype = canaryd_evts;
  
  /*
   * Register with the event system. 
   */
  parms->ev_handle = event_register_withkeyfile(parms->ev_server, 0,
                                                EVENTKEYFILE);
  if (parms->ev_handle == NULL) {
    lerror("could not register with event system");
    return -1;
  }
  
  /*
   * Subscribe to the event we specified above.
   */
  if (! event_subscribe(parms->ev_handle, ev_callback, tuple, NULL)) {
    lerror("could not subscribe to event");
    return -1;
  }

  address_tuple_free(tuple); /* XXX: keep around for sending evts. */

#ifdef __linux__
  /* Open socket for SIOCGHWADDR ioctl (to get mac addresses) */
  parms->ifd = socket(PF_INET, SOCK_DGRAM, 0);
#endif

  /* Setup "vmstat" stuff */
  getdrivedata(driveargv);

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
    if ((pfd = open(PIDFILE, O_EXCL | O_CREAT | O_RDWR)) < 0) {
      lerror("Can't create lock file.");
      return -1;
    }
    fchmod(pfd, S_IRUSR | S_IRGRP | S_IROTH);
    sprintf(pidbuf, "%d", getpid());
    write(pfd, pidbuf, strlen(pidbuf));
    close(pfd);
  }

  /* Setup run length alarm, if needed. */
  if (opts->run_len) {
    alarm(opts->run_len);
  }

  return 0;
}


void do_exit(int exval) {
  int status;

  unlink(PIDFILE);
  if (event_unregister(parms->ev_handle) == 0) {
    lnotice("could not unregister from event system");
  }
  while (wait(&status) != -1);
  lnotice("exiting.");
  exit(exval);
}

int grab_cifname(char *buf, void *data) {

  int retval = -1;
  char *tmpptr;
  CANARYD_PARAMS *myparms = (CANARYD_PARAMS*) data;

  if (buf && isalpha(buf[0])) {
    tmpptr = myparms->cifname = strdup(buf);
    while (isalnum(*tmpptr))  tmpptr++;
    *tmpptr = '\0';
    retval = 0;
  }
  else {
    myparms->cifname = NULL;
  }

  return retval;
}

int grab_pideid(char *buf, void *data) {
  int retval = -1;
  char *bp, *tmpbp;
  CANARYD_PARAMS *myparms = (CANARYD_PARAMS*) data;

  if (strstr(buf, "ALLOCATED=")) {
    bp = tmpbp = buf+10;
    while (!isspace(*tmpbp)) tmpbp++;
    *tmpbp = '\0';
    myparms->pideid = strdup(bp);
    retval = 0;
  }
  else {
    myparms->pideid = "";
  }

  return retval;
}

enum {
  SITEIDX    = 0,
  EXPTIDX    = 1,
  GROUPIDX   = 2,
  HOSTIDX    = 3,
  OBJTYPEIDX = 4,
  OBJNAMEIDX = 5,
  EVTYPEIDX  = 6,
  ARGSIDX  = 7
};

void ev_callback(event_handle_t handle, event_notification_t notification, 
                 void *data) {

  char		buf[8][128]; /* XXX: bad magic */
  int		len = 128;
  struct timeval	now;
  static int ecount;

  bzero(buf, sizeof(buf));

  event_notification_get_site(handle, notification, buf[SITEIDX], len);
  event_notification_get_expt(handle, notification, buf[EXPTIDX], len);
  event_notification_get_group(handle, notification, buf[GROUPIDX], len);
  event_notification_get_host(handle, notification, buf[HOSTIDX], len);
  event_notification_get_objtype(handle, notification, buf[OBJTYPEIDX], len);
  event_notification_get_objname(handle, notification, buf[OBJNAMEIDX], len);
  event_notification_get_eventtype(handle, notification, buf[EVTYPEIDX], len);
  event_notification_get_arguments(handle, notification, buf[ARGSIDX], len);
  
  if (opts->debug) {          
    gettimeofday(&now, NULL);
    fprintf(stderr,"Event %d: %lu.%03lu %s %s %s %s %s %s %s %s\n",
            ++ecount, now.tv_sec, now.tv_usec / 1000,
            buf[SITEIDX], buf[EXPTIDX], buf[GROUPIDX],
            buf[HOSTIDX], buf[OBJTYPEIDX], buf[OBJNAMEIDX], 
            buf[EVTYPEIDX], buf[ARGSIDX]);
  }

  if (!strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_START)) {
    if (! *buf[ARGSIDX]) {
      lnotice("Arguments not provided in start event - ignoring.");
    }
    else {
      parse_ev_args(buf[ARGSIDX]);
    }
  }
  
  else if (!strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_START)) {
    parms->ol_detect = 0;
  }

  else if (!strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_RESET)) {
    /* set_default_overload_params(); */
  }

  else if (!strcmp(buf[EVTYPEIDX], TBDB_EVENTTYPE_REPORT)) {
    parms->send_ev_report = 1;
  }

  else {
    lnotice("Unknown event type received.");
  }

  return;
}

void send_ol_event(CANARYD_PACKET *pkt) {

  int vnbufsiz, minibufsiz, i;
  char minibuf[50];
  char vnbuf[2000];
  event_notification_t notification;
  address_tuple_t tuple;
  struct timeval now;

  gettimeofday(&now,0);

  tuple = address_tuple_alloc();
  if (tuple == NULL) {
    lerror("could not allocate an address tuple");
    return;
  }

  /*
   * Setup tuple to send profile alert
   */
  tuple->host	   = ADDRESSTUPLE_ANY;
  tuple->site      = ADDRESSTUPLE_ANY;
  tuple->group     = ADDRESSTUPLE_ANY;
  tuple->expt      = parms->pideid;
  tuple->objtype   = TBDB_OBJECTTYPE_CANARYD;
  tuple->objname   = "canaryd";
  tuple->eventtype = TBDB_EVENTTYPE_ALERT;

  /* Generate the event */
  notification = event_notification_alloc(parms->ev_handle, tuple);
	
  if (notification == NULL) {
    lnotice("could not allocate notification");
    return;
  }

  strcpy(vnbuf, "vnodes=");
  vnbufsiz=7;

  /* add vnode info, if necessary */
  for (i = 0; pkt->vnodes[i].hname; ++i) {
    sprintf(minibuf, "%s,", pkt->vnodes[i].hname);
    minibufsiz = strlen(minibuf);
    if (vnbufsiz + minibufsiz > sizeof(vnbuf)) {
      lerror("vnbuf not big enough to hold all content!");
      abort();
    }
    strcat(vnbuf, minibuf);
    vnbufsiz += minibufsiz;
  }
  vnbuf[vnbufsiz-1] = '\0';

  event_notification_set_arguments(parms->ev_handle, notification, vnbuf);
  
  if (event_schedule(parms->ev_handle, notification, &now) == 0) {
    lnotice("could not send test event notification");
  }

  if (opts->debug) {
    printf("** Alert event sent **\n");
  }

  event_notification_free(parms->ev_handle, notification);
  address_tuple_free(tuple);
  return;
}

  

int SETRANGE(char *token, RANGE *range) {
  char *tok = token;
  char *ebp;

  range->min = strtod(tok, &ebp);
  if (ebp == tok) {
    lnotice("bad data in numeric conversion");
    goto bad;
  }
  if (*ebp != ',') {
    lnotice("unexpected/missing separator in range");
    goto bad;
  }
  tok = ebp+1;
  range->max = strtod(tok, &ebp);
  if (ebp == tok) {
    lnotice("bad data in numeric conversion");
    goto bad;
  }

  if (opts->debug) {
    printf("range min/max: %.03f/%.03f\n", range->min, range->max);
  }
  return 1;

 bad:
  range->min = 0;
  range->max = 0;
  return 0;
}

#define EVARGSEPS " \t"

void parse_ev_args(char *args) {

  int i;
  char *tok;

  tok = strtok(args, EVARGSEPS);
  do {
    if (tok && *tok) {
      if (opts->debug) {
        printf("ev arg token: %s\n", tok);
      }
      for(i = 0; i < NUMRANGES; ++i) {
        if (strstr(tok, evargitems[i].key)) {
          tok += strlen(evargitems[i].key)+1;
          if (! SETRANGE(tok, &parms->olr[i])) goto bad;
          break;
        }
      }
      if (i == NUMRANGES) {
        lnotice("Unknown token in args list");
        goto bad;
      }
    }
  } while ((tok = strtok(NULL, EVARGSEPS)) != NULL);

  parms->ol_detect = 1;
  return;

 bad:
  lnotice("Problem encountered while parsing event args");
  parms->ol_detect = 0;
  return;
}


int send_pkt(CANARYD_PACKET *pkt) {

  int i, numsent, retval = 1;
  static char pktbuf[8192];
  static char minibuf[128];
  int pktbufsiz, minibufsiz;
  struct timeval now;

  /* Get current time. */
  gettimeofday(&now, NULL);

  /* flatten data into packet buffer */
  sprintf(pktbuf, "vers=%s stamp=%lu,%lu lave=%.10f,%.10f,%.10f ovld=%u cpu=%u mem=%u,%u disk=%u netmem=%u,%u,%u ",
          CDPROTOVERS,
          now.tv_sec,
          now.tv_usec,
          pkt->loadavg[0],
          pkt->loadavg[1],
          pkt->loadavg[2],
          pkt->overload,
          pkt->olmet[PCTCPU],
          pkt->olmet[PCTMEM],
          pkt->olmet[PGOUT],
          pkt->olmet[PCTDISK],
          pkt->olmet[PCTNETMEM],
          pkt->olmet[NETMEMWAITS],
          pkt->olmet[NETMEMDROPS]);

  pktbufsiz = strlen(pktbuf);
  
  /* get all the interfaces too */
  for (i = 0; i < pkt->ifcnt; ++i) {
    sprintf(minibuf, "iface=%s,%lu,%lu,%llu,%llu ",
            pkt->ifaces[i].addr,
            pkt->ifaces[i].ipkts,
            pkt->ifaces[i].opkts,
            pkt->ifaces[i].ibytes,
            pkt->ifaces[i].obytes);
    minibufsiz = strlen(minibuf);
    if (pktbufsiz + minibufsiz + 1 > sizeof(pktbuf)) {
      lerror("pktbuf not big enough to hold all content!");
      abort();
    }
    strcat(pktbuf, minibuf);
    pktbufsiz += minibufsiz;
  }

  /* add vnode info, if necessary */
  for (i = 0; pkt->vnodes[i].hname; ++i) {
    sprintf(minibuf, "vnode=%s,%.1f,%.1f ",
            pkt->vnodes[i].hname,
            pkt->vnodes[i].cpu,
            pkt->vnodes[i].mem);
    minibufsiz = strlen(minibuf);
    if (pktbufsiz + minibufsiz + 1 > sizeof(pktbuf)) {
      lerror("pktbuf not big enough to hold all content!");
      abort();
    }
    strcat(pktbuf, minibuf);
    pktbufsiz += minibufsiz;
  }


  if (opts->debug) {
    printf("packet: %s\n", pktbuf);
    printf("packet len: %d\n", pktbufsiz);
  }
  
  /* send it */
  else {
    if (opts->log) {
      fputs(pktbuf, parms->log);
      fputs("\n", parms->log);
      fflush(parms->log);
      if (ferror(parms->log)) {
        lwarn("Error on log file stream");
      }
    }
    else {
      if ((numsent = send(parms->sd, pktbuf, pktbufsiz+1, 0)) < 0) {
        lerror("Problem sending canaryd packet");
        retval = 0;
      }
    
      else if (numsent < pktbufsiz+1) {
        lwarn("Warning!  Canaryd packet truncated");
        retval = 0;
      }
    }
  }

  return retval;
}

int process_ps(char *psln, void *data) {

  VPROCENT *proctab = (VPROCENT*)data;
  int i;
  int pid;
  float cpu, mem;
  char *eptr;

  /* skip label line */
  if ((strtod(psln, &eptr) == 0) && (eptr == psln)) {
    return 0;
  }

  sscanf(psln, "%u %f %f", &pid, &cpu, &mem);

  for (i = 0; proctab[i].hname; ++i) {
    if (proctab[i].pid == pid) {
      break;
    }
  }

  if (proctab[i].hname) {
    proctab[i].cpu = cpu;
    proctab[i].mem = mem;
  }

  return 0;
}
  
void get_vnode_stats (CANARYD_PACKET *pkt) {

  int i;
  DIR *pfsdir;
  struct dirent *procent;
  FILE *procfile;
  char sfilename[MAXPATHLEN];
  char statbuf[MAXLINELEN];
  static VPROCENT proctab[MAXPROCNUM];
  unsigned int pid;
  char hname[MAXHOSTNAMELEN];
  char *endlptr;
  char *psprog[] = {"ps", "ax", "-o", "pid,%cpu,%mem", NULL};

  for (i = 0; pkt->vnodes[i].hname; ++i) {
    pkt->vnodes[i].cpu = 0;
    pkt->vnodes[i].mem = 0;
  }

  /* Iterate over the /proc entries, looking for jailed nodes.
     Once found, we grab their usage stats and pack them up to
     be sent back.
  */
  if ((pfsdir = opendir("/proc")) < 0) {
    lerror("Problem opening /proc");
    return;
  }

  i = 0;
  while ((procent = readdir(pfsdir)) != NULL) {
    if (!strcmp(procent->d_name, "curproc") ||
        (procent->d_name[0] == '.')) {
      continue;
    }

    snprintf(sfilename, sizeof(sfilename), "/%s/%s/%s", 
             "proc", 
             procent->d_name,
             "status");

    if ((procfile = fopen(sfilename, "r")) == NULL) {
      lerror("problem opening process file entry");
      continue;
    }

    fgets(statbuf, sizeof(statbuf), procfile);
    fclose(procfile);
    sscanf(statbuf, "%*s %u %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %s",
           &pid, hname);
    endlptr = strchr(hname, '\n');
    if (endlptr) {
      *endlptr = '\0';
    }

    if (!strcmp(hname, "-")) {
      continue;
    }

    proctab[i].pid = pid;
    proctab[i].hname = strdup(hname);
    proctab[i].cpu = 0;
    proctab[i].mem = 0;
    i++;
  }

  closedir(pfsdir);
  procpipe(psprog, process_ps, (void*)proctab);
  
  for (i = 0; proctab[i].hname; ++i) {
    int j;

    for (j = 0; pkt->vnodes[j].hname; ++j) {
      if (!strcmp(pkt->vnodes[j].hname, proctab[i].hname)) {
        break;
      }
    }

    if (!pkt->vnodes[j].hname) {
      pkt->vnodes[j].hname = strdup(proctab[i].hname);
      parms->numvnodes++;
    }
    pkt->vnodes[j].cpu += proctab[i].cpu;
    pkt->vnodes[j].mem += proctab[i].mem;
    free(proctab[i].hname);
    proctab[i].hname = NULL;
  }

  return;
}

void get_vmem_stats(CANARYD_PACKET *pkt) {

  int *pres;

  pkt->olmet[PCTCPU] = getcpubusy();
  pres = getmembusy(getmempages());
  pkt->olmet[PGOUT] = pres[0];
  pkt->olmet[PCTMEM] = pres[1];
  pkt->olmet[PCTDISK] = getdiskbusy();
  pres = getnetmembusy();
  pkt->olmet[PCTNETMEM] = pres[0];
  pkt->olmet[NETMEMWAITS] = pres[1];
  pkt->olmet[NETMEMDROPS] = pres[2];

  return;
}

void check_overload(CANARYD_PACKET *pkt) {

  int i;
  
  pkt->overload = 0;
  for (i = 0; i < NUMRANGES; ++i) {
    if (pkt->olmet[i] > (int)parms->olr[i].max) {
      pkt->overload = 1;
    }
  }
}

void get_load(CANARYD_PACKET *pkt) {

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


void get_packet_counts(CANARYD_PACKET *pkt) {

  int i;
  char *niprog[] = {"netstat", NETSTAT_ARGS, NULL};

  pkt->ifcnt = 0;
  if (procpipe(niprog, &get_counters, (void*)pkt)) {
    lwarn("Netinfo exec failed.");
    pkt->ifcnt = 0;
  }
  else if (opts->debug) {
    for (i = 0; i < pkt->ifcnt; ++i) {
      printf("IFACE: %s  ipkts: %lu  opkts: %lu ibytes: %llu obytes: %llu\n", 
             pkt->ifaces[i].ifname, 
             pkt->ifaces[i].ipkts,
             pkt->ifaces[i].opkts,
             pkt->ifaces[i].ibytes,
             pkt->ifaces[i].obytes);
    }
  }
  return;
}

int get_counters(char *buf, void *data) {

  CANARYD_PACKET *pkt = (CANARYD_PACKET*)data;
#ifdef __linux__
  struct ifreq ifr;
  bzero(&ifr, sizeof(struct ifreq));
#endif

  if (pkt->ifcnt < MAXNUMIFACES
      && !strstr(buf, "lo")
#ifdef __FreeBSD__
      && !strstr(buf, "*")
      && strstr(buf, "<Link"))
#endif
#ifdef __linux__
      && strstr(buf, "eth"))
#endif
  {

    if (sscanf(buf, CNTFMTSTR,
               pkt->ifaces[pkt->ifcnt].ifname,
#ifdef __FreeBSD__
               pkt->ifaces[pkt->ifcnt].addr,
#endif
               &pkt->ifaces[pkt->ifcnt].ipkts,
               &pkt->ifaces[pkt->ifcnt].ibytes,               
               &pkt->ifaces[pkt->ifcnt].opkts,
               &pkt->ifaces[pkt->ifcnt].obytes)  != NUMSCAN) {
      lwarn("scan of netstat line failed!");
      return -1;
    }
#if 0
    { 
      struct in_addr iaddr;
      if (inet_pton(AF_INET, pkt->ifaces[pkt->ifcnt].addr, &iaddr) != 1) {
        if (opts->debug) {
          printf("failed to convert addr to inet addr\n");
        }
        return 0;
      }
    }
#endif
#ifdef __linux__
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
#endif
    pkt->ifcnt++;
  }
  return 0;
}


/* XXX change to combine last return value of procfunc with exec'ed process'
   exit status & write macros for access.
*/
int procpipe(char *const prog[], int (procfunc)(char*,void*), void* data) {

  int fdes[2], retcode, cpid, status;
  char buf[MAXLINELEN];
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
