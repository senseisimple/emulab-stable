#include "slothd.h"

SLOTHD_PACKET *pkt;
SLOTHD_OPTS   *opts;

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
  unlink(PIDFILE);
  while (wait(&status) != -1);
  exit(signum);
}

void siginthandler(int signum) {

  int status;

  unlink(PIDFILE);
  while (wait(&status) != -1);
  lnotice("exiting.");
  exit(0);
}

void usage(void) {  

  printf("Usage:\tslothd -h\n");
  printf("\tslothd [-a] [-d] [-i <interval>] [-p <port>] [-s <server>]\n");
  printf("\t -h\t This message.\n");
  printf("\t -a\t Only scan active terminal special files.\n");
  printf("\t -f\t Send first idle data report immediately on startup.\n");
  printf("\t -d\t Debug mode; do not fork into background.\n");
  printf("\t -i <interval>\t Run interval, in seconds. (default is 10 min.)\n");
  printf("\t -p <port>\t Send on port <port> (default is %d).\n", SLOTHD_DEF_PORT);
  printf("\t -s <server>\t Send data to <server> (default is %s).\n", SLOTHD_DEF_SERV);
}


int main(int argc, char **argv) {

  int exitcode = -1;
  int pfd;
  char pidbuf[10];
  static SLOTHD_OPTS mopts;
  static SLOTHD_PACKET mpkt;

  /* pre-init */
  bzero(&mopts, sizeof(SLOTHD_OPTS));
  bzero(&mpkt, sizeof(SLOTHD_PACKET));
  opts = &mopts;
  pkt = &mpkt;

  /* Try to get lock.  If can't, then bail out. */
  if ((pfd = open(PIDFILE, O_EXCL | O_CREAT | O_RDWR)) < 0) {
    exit(1);
  }
  fchmod(pfd, S_IRUSR | S_IRGRP | S_IROTH);
  sprintf(pidbuf, "%d", getpid());
  write(pfd, pidbuf, sizeof(pidbuf));
  close(pfd);

  if (parse_args(argc, argv) < 0) {
    fprintf(stderr, "Error processing arguments.\n");
  }
  else {
    if (init_slothd() < 0) {
      lerror("Problem initializing, bailing out");
    }
    else {
      exitcode = 0;
      lnotice("Slothd started");
      for (;;) {
        if (!opts->first) {
          sleep(mopts.interval);
        }          
        get_load();
        get_min_tty_idle();
        get_packet_counts();
        send_pkt();
        if (opts->first) {
          sleep(mopts.interval);
        }
      }
    }
  }
  return exitcode;
}


int parse_args(int argc, char **argv) {

  char ch;

  /* setup defaults. */
  opts->debug = 0;
  opts->actterms = 0;
  opts->interval = DEF_INTVL;
  opts->port = SLOTHD_DEF_PORT;
  opts->servname = SLOTHD_DEF_SERV;
  opts->first = 0;

  while ((ch = getopt(argc, argv, "ai:dp:s:hf")) != -1) {
    switch (ch) {

    case 'i':
      if ((opts->interval = atoi(optarg)) < MIN_INTVL) {
        lwarn("Warning!  Interval set too low, defaulting.");
        opts->interval = MIN_INTVL;
      }
      break;

    case 'd':
      lnotice("Debug mode requested; staying in foreground.");
      opts->debug = 1;
      break;

    case 'a':
      lnotice("Scanning only active terminals.");
      opts->actterms = 1;
      break;

    case 'f':
      opts->first = 1;
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

  DIR *devs;
  char bufstr[MAXDEVLEN];
  struct dirent *dptr;
  struct hostent *hent;

  /* init internal vars */
  opts->numttys = 0;

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
  openlog("slothd", LOG_NDELAY, LOG_DAEMON);

  /* Setup path */
  if (setenv("PATH", SLOTHD_PATH_ENV, 1) < 0) {
    lerror("Couldn't set path env");
  }

  /* enum tty special files, if necessary. */
  if (!opts->actterms) {
    if ((devs = opendir("/dev")) == 0) {
      lerror("Can't open directory /dev for processing");
      return -1;
    }
    opts->ttys[opts->numttys] = strdup("/dev/console");
    opts->numttys++;
    while (opts->numttys < MAXTTYS && (dptr = readdir(devs))) {
      if (strstr(dptr->d_name, "tty") || strstr(dptr->d_name, "pty")) {
        snprintf(bufstr, MAXDEVLEN, "/dev/%s", dptr->d_name);
        opts->ttys[opts->numttys] = strdup(bufstr);
        opts->numttys++;
        bzero(bufstr, sizeof(bufstr));
      }
    }
  }

  /* prepare UDP connection to server */
  if ((pkt->sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    lerror("Could not alloc socket");
    return -1;
  }
  if (!(hent = gethostbyname(opts->servname))) {
    lerror("Can't resolve server hostname"); /* XXX use herror */
    return -1;
  }
  bzero(&pkt->servaddr, sizeof(struct sockaddr_in));
  pkt->servaddr.sin_family = AF_INET;
  pkt->servaddr.sin_port = htons(opts->port);
  bcopy(hent->h_addr_list[0], &pkt->servaddr.sin_addr.s_addr, 
        sizeof(struct in_addr));
  if (connect(pkt->sd, (struct sockaddr*)&pkt->servaddr, 
              sizeof(struct sockaddr_in)) < 0) {
    lerror("Couldn't connect to server");
    return -1;
  }

  /* Daemonize, unless in debug mode. */
  if (!opts->debug && daemon(0,0) < 0) {
    lerror("Couldn't daemonize");
    return -1;
  }

  return 0;
}


void get_min_tty_idle(void) {
  
  int i;
  time_t mintime = 0;
  struct stat sb;

  if (opts->actterms) {
    utmp_enum_terms();
    if (!opts->numttys) {
      mintime = wtmp_get_last();
    }
  }
#ifdef __linux__
  /* Linux uses dynamically allocated UNIX98 ptys */
  else {
    DIR *ptsdir;
    struct dirent *dptr;
    char *ptspath = "/dev/pts", curpath[15];
    
    if ((ptsdir = opendir(ptspath)) == NULL) {
      lerror("Couldn't open /dev/pts for processing");
    }
    else {
      while ((dptr = readdir(ptsdir))) {
        sprintf(curpath, "%s/%s", ptspath, dptr->d_name);
        if (stat(curpath, &sb) < 0) {
          fprintf(stderr, "Can't stat %s:  %s\n", 
                  curpath, strerror(errno)); /* XXX change. */
        }
        else if (sb.st_atime > mintime) {
          mintime = sb.st_atime;
        }
      }
    }
  }
#endif /* __linux__ */

  for (i = 0; i < opts->numttys; ++i) {
    if (stat(opts->ttys[i], &sb) < 0) {
      fprintf(stderr, "Can't stat %s:  %s\n", 
              opts->ttys[i], strerror(errno)); /* XXX change. */
    }
    else if (sb.st_atime > mintime) {
      mintime = sb.st_atime;
    }
  }

  pkt->minidle = mintime;
  if (opts->debug) {
    printf("Minidle: %s", ctime(&mintime));
  }
  return;
}


void utmp_enum_terms(void) {
  
  int utfd, nbytes, i;
  char bufstr[MAXDEVLEN];
  struct utmp u;

  for (i = 0; i < opts->numttys; ++i) {
    free(opts->ttys[i]);
  }

  opts->numttys = 0;

  if ((utfd = open(UTMP_PATH, O_RDONLY)) < 0) {
    lerror("Couldn't open utmp file");
    return;
  }
  else {
    while ((nbytes = read(utfd, &u, sizeof(struct utmp)))) {
      if (nbytes < sizeof(struct utmp)) {
        lwarn("problem reading utmp structure - truncated.");
      }
      else if (u.ut_name[0] != '\0' && u.ut_host[0] != '\0') {
        snprintf(bufstr, MAXDEVLEN, "/dev/%s", u.ut_line);
        opts->ttys[opts->numttys] = strdup(bufstr);
        opts->numttys++;
      }
    }
  }

  close(utfd);
  return;
}


time_t wtmp_get_last(void) {
  int nbytes;
  time_t retval = 0;
  struct utmp u;
  FILE *wtf;

  if ((wtf = fopen(WTMP_PATH, "r")) < 0) {
    lerror("Couldn't open wtmp file");
  }

  else if (fseek(wtf, -sizeof(struct utmp), SEEK_END) < 0) {
    lerror("Can't seek to end of file");
  }

  else {
    do {
      nbytes = fread(&u, 1, sizeof(struct utmp), wtf);
      if (nbytes < sizeof(struct utmp)) {
        lwarn("problem reading utmp structure - truncated.");
      }
      else if (u.ut_name[0] != '\0' &&
               u.ut_host[0] != '\0' &&
               strlen(u.ut_line) > 1) {
        retval = u.ut_time;
        break;
      } 
    } while (fseek(wtf, -2*sizeof(struct utmp), SEEK_CUR) == 0 &&
             !ferror(wtf));
  }
  fclose(wtf);
  return retval;
}        

void get_load(void) {

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


void get_packet_counts(void) {

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

#ifdef comment
int get_macaddrs(char *buf, void *data) {

  int i=-1, j;
  char *maddr;
  SLOTHD_PACKET *pkt = (SLOTHD_PACKET*)data;
 
  if ((maddr = strstr(buf, "MAC="))) {
    maddr += 4;
    ++i;
    for (j = 0; j < MACADDRLEN; j+=3, maddr+=2) {
      pkt->ifaces[i].macaddr[j] = *maddr;
      pkt->ifaces[i].macaddr[j+1] = *(maddr+1);
      pkt->ifaces[i].macaddr[j+2] = ':';
    }
    pkt->ifaces[i].macaddr[MACADDRLEN-1] = '\0';
  }
  pkt->numaddrs = i+1;
  return 0;
}
#endif /* comment */

int get_counters(char *buf, void *data) {

  SLOTHD_PACKET *pkt = (SLOTHD_PACKET*)data;
#ifdef __linux__
  int sfd = socket(PF_INET, SOCK_DGRAM, 0);
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
               &pkt->ifaces[pkt->ifcnt].opkts) != NUMSCAN) {
      return -1;
    }
#ifdef __linux__
    strcpy(ifr.ifr_name, pkt->ifaces[pkt->ifcnt].ifname);
    if (ioctl(sfd, SIOCGIFHWADDR, &ifr) < 0) {
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
  

void send_pkt(void) {

  int i, numsent;
  static char pktbuf[1500];
  static char minibuf[50];

  /* flatten data into packet buffer */
  sprintf(pktbuf, "mis=%lu lave=%.10f,%.10f,%.10f ",
          pkt->minidle,
          pkt->loadavg[0],
          pkt->loadavg[1],
          pkt->loadavg[2]);

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
    if ((numsent = send(pkt->sd, pktbuf, strlen(pktbuf)+1, 0)) < 0) {
      lerror("Problem sending slothd packet");
    }

    else if (numsent < strlen(pktbuf)+1) {
      lwarn("Warning!  Slothd packet truncated");
    }
  }
  return;
}
