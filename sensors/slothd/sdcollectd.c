#include "sdcollectd.h"

void lerror(const char* msgstr) {
  if (msgstr) {
    syslog(LOG_ERR, "%s: %m", msgstr);
    perror(msgstr);
  }
}

void lnotice(const char *msgstr) {
  if (msgstr) {
    syslog(LOG_NOTICE, "%s", msgstr);
    printf("%s", msgstr);
  }
}

void siginthandler(int signum) {

  lnotice("Daemon exiting.\n");
  exit(0);
}


void usage(void) {  

  printf("Usage:\tslothd -h\n");
  printf("\tslothd [-d] [-p <port>]\n");
  printf("\t -h\t This message.\n");
  printf("\t -d\t Debug mode; do not fork into background.\n");
  printf("\t -p <port>\t Send on port <port> (default is %d).\n", SDCOLLECTD_PORT);
}


int parse_args(int argc, char **argv) {

  char ch;

  /* setup defaults. */
  opts->debug = 0;
  opts->port = SDCOLLECTD_PORT;

  while ((ch = getopt(argc, argv, "dp:h")) != -1) {
    switch (ch) {

    case 'd':
      lnotice("Debug mode requested; staying in foreground.\n");
      opts->debug = 1;
      break;

    case 'p':
      opts->port = (u_short)atoi(optarg);
      break;

    case 'h':
    default:
      usage();
      return 0;
      break;
    }
  }
  return 1;
}


/* XXX:  Comment the code better! */

int main(int argc, char **argv) {

  int sd;
  struct sockaddr_in servaddr;
  static IDLE_DATA iddata;
  static SDCOLLECTD_OPTS gopts;

  opts = &gopts;

  /* Foo on SIGPIPE & SIGHUP. */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Cleanup on sigint/term */
  signal(SIGTERM, siginthandler);
  signal(SIGINT, siginthandler);

  if (!parse_args(argc, argv)) {
    lnotice("Error processing arguments, exiting.\n");
    exit(1);
  }

  /* clear, and initialize inet sockaddr */
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(SDCOLLECTD_PORT);

  /* setup logging facilities */
  if (strchr(argv[0], '/')) {
    openlog(strrchr(argv[0], '/')+1, 0, LOG_DAEMON);
  }
  else {
    openlog(argv[0], 0, LOG_DAEMON);
  }

  /* Create and bind udp socket for collecting slothd client-side idle data */
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    lerror("Can't create socket");
    exit(1);
  }

  if (bind(sd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0) {
    lerror("Can't bind socket");
    exit(1);
  }

  /* get DB funcs ready - libtb */
  if (!dbinit()) {
    lerror("Couldn't connect to db. bailing out.");
    exit(1);
  }

  if (!opts->debug) {
    if (daemon(0, 0) < 0) {
      lerror("Couldn't become daemon");
      exit(1);
    }
    lnotice("Daemon started successfully");
  }

  /* do our thing - just loop collecting data from clients, and insert into
     DB. 
  */
  for ( ;; ) {
    bzero(&iddata, sizeof(IDLE_DATA)); /* cleanse out old values, if any */
    if (CollectData(sd, &iddata)) {
      ParseRecord(&iddata);
      PutDBRecord(&iddata);
    }
  }
  /* NOTREACHED */
}

int CollectData(int sd, IDLE_DATA *iddata) {
  int numbytes, slen;
  /* struct hostent *hent; */
  struct sockaddr_in cliaddr;

  bzero(&cliaddr, sizeof(struct sockaddr_in));
  slen = sizeof(struct sockaddr_in);

  if((numbytes = recvfrom(sd, iddata->buf, sizeof(iddata->buf), 0,
                          (struct sockaddr*)&cliaddr, &slen))) {
    /* Using libtb functionality now.
      if (!(hent = gethostbyaddr((char*)&cliaddr.sin_addr, 
      sizeof(cliaddr.sin_addr), AF_INET))) {
      lerror("Can't resolve host");
      return -1;
    }
    */

    if (!mydb_iptonodeid(inet_ntoa(cliaddr.sin_addr), iddata->id)) {
      lerror("Couldn't obtain node id");
      return 0;
    }
    if (opts->debug) {
      printf("Received data from: %s\n", iddata->id);
      printf("buffer: %s\n", iddata->buf);
    }
  }

  return 1;
}

/******************************************************************************
Function:  tbmac

Purpose:  Utility function which takes buffers with ascii strings corresponding
          to ethernet mac addresses, and converts them into the standard 12
          character testbed format.

Args:  char *maddr: NULL-terminated input C-string which should contain a 
                    macaddr of the form "[X]X:[X]X:[X]X:[X]X:[X]X:[X]X.  If 
                    the first char in an octet is missing, it is assumed to 
                    be zero. Delimiter can actually be any non-hex digit (not 
                    just ':')

       char **endptr: mem addr to be filled in with pointer to character 
                     proceeding mac addr conversion in the input string *maddr.
                     Behaves as in strtol() and friends.

Returns: Pointer to static memory containing a testbed-style mac address (this
         being a string of exactly 12 hex digits.  NULL is returned if the
         input buffer does not have a mac address conforming to the form
         specified above for *maddr.

Side Effects:  Return memory area is overwritten on each call.

Notes:  *maddr parameter is unaltered.
******************************************************************************/

char *tbmac(char *maddr, char **endptr) {

  int i = 0;
  static char tbaddr[MACADDRLEN+1];
  char *myptr = maddr, *mylast = maddr;

  bzero(&tbaddr, sizeof(tbaddr));

  while (i < MACADDRLEN) {
    while (*myptr && isxdigit(*myptr)) myptr++;
    
    if (myptr - mylast == 1) {
      tbaddr[i++] = '0';
      tbaddr[i++] = toupper(*(myptr-1));
      mylast = ++myptr;
    }
    else if (myptr - mylast == 2) {
      tbaddr[i++] = toupper(*mylast);
      tbaddr[i++] = toupper(*(myptr-1));
      mylast = ++myptr;
    }
    else if (myptr - maddr == MACADDRLEN) {
      strcpy(tbaddr, maddr);
      ++myptr;
      break;
    }
    else {
      break;
    }
  }

  if (strlen(tbaddr) != MACADDRLEN) {
    if (endptr) *endptr = NULL;
    return NULL;
  }
  else {
    if (endptr) *endptr = myptr-1;
    return tbaddr;
  }
}

void ParseRecord(IDLE_DATA *iddata) {

  int i;
  char *itemptr, *ptr;

  iddata->ifcnt = 0;
  
  /* Parse out fields */
  itemptr = strtok(iddata->buf, " \t");
  do {
    if (strstr(itemptr, "mis")) {
      iddata->mis = atol(itemptr+4);
    }
    else if (strstr(itemptr, "lave")) {
      iddata->l1m = strtod(itemptr+5, &ptr);
      iddata->l5m = strtod(ptr+1, &ptr);
      iddata->l15m = strtod(ptr+1, NULL);
    }
    else if (strstr(itemptr, "iface")) {
      strcpy(iddata->ifaces[iddata->ifcnt].mac, tbmac(itemptr+6, &ptr));
      iddata->ifaces[iddata->ifcnt].ipkts = strtoul(ptr+1, &ptr, 10);
      iddata->ifaces[iddata->ifcnt].opkts = strtoul(ptr+1, NULL, 10);
      iddata->ifcnt++;
    }
    else {
      lnotice("Unrecognized string in packet\n");
    }
  } while ((itemptr = strtok(NULL, " \t")) && iddata->ifcnt < MAXNUMIFACES);

  if (opts->debug) {
    printf("iddata->mis: %lu\n", iddata->mis);
    printf("iddata->l1m: %f\n", iddata->l1m);
    printf("iddata->l5m: %f\n", iddata->l5m);
    printf("iddata->l15m: %f\n", iddata->l15m);
    for (i = 0; i < iddata->ifcnt; ++i) {
      printf("iface%d: %s, ipkts %lu, opkts %lu\n",
             i,
             iddata->ifaces[i].mac,
             iddata->ifaces[i].ipkts,
             iddata->ifaces[i].opkts);
    }
  }

  return;
}


void PutDBRecord(IDLE_DATA *iddata) {

  int i;
  time_t now = time(NULL);

  if (!mydb_update("INSERT INTO node_idlestats VALUES ('%s', FROM_UNIXTIME(%lu), FROM_UNIXTIME(%lu),  %f, %f, %f)", 
                   iddata->id, 
                   now,
                   iddata->mis, 
                   iddata->l1m, 
                   iddata->l5m, 
                   iddata->l15m)) {
    lerror("Error inserting data into node_idlestats table");
  }

  for (i = 0; i < iddata->ifcnt; ++i) {
    if (!mydb_update("INSERT INTO iface_counters VALUES ('%s', FROM_UNIXTIME(%lu), '%s', %lu, %lu)",
                     iddata->id,
                     now,
                     iddata->ifaces[i].mac,
                     iddata->ifaces[i].ipkts,
                     iddata->ifaces[i].opkts)) {
      lerror("Error inserting data into iface_counters table");
    }
  }
  return;
}
