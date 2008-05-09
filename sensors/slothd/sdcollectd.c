/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "sdcollectd.h"

void siginthandler(int signum) {

  info("Daemon exiting.\n");
  exit(0);
}


void usage(void) {  

  printf("Usage:\tslothd -h\n"
         "\tslothd [-d] [-p <port>]\n"
         "\t -h\t This message.\n"
         "\t -o\t DO populate old idlestats tables.\n"
         "\t -d\t Debug mode; do not fork into background.\n"
         "\t -p <port>\t Listen on port <port> (default is %d).\n", 
         SDCOLLECTD_PORT);
}


int parse_args(int argc, char **argv) {

  char ch;

  /* setup defaults. */
  opts->popold = 0;
  opts->debug = 0;
  opts->port = SDCOLLECTD_PORT;

  while ((ch = getopt(argc, argv, "odp:h")) != -1) {
    switch (ch) {

    case 'o':
      info("Populating old node_idlestats, and iface_counters tables");
      opts->popold = 1;
      break;

    case 'd':
      info("Debug mode requested; staying in foreground.\n");
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

  extern char build_info[];

  opts = &gopts;
  
  /* Foo on SIGPIPE & SIGHUP. */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  
  /* Cleanup on sigint/term */
  signal(SIGTERM, siginthandler);
  signal(SIGINT, siginthandler);

  /* setup logging facilities */
  loginit(1, "sdcollectd");
  
  if (!parse_args(argc, argv)) {
    error("Error processing arguments, exiting.\n");
    exit(1);
  }
  
  /* clear, and initialize inet sockaddr */
  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(SDCOLLECTD_PORT);

  /* Create and bind udp socket for collecting slothd client-side idle data */
  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    errorc("Can't create socket");
    exit(1);
  }

  if (bind(sd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0) {
    errorc("Can't bind socket");
    exit(1);
  }

  /* get DB funcs ready - libtb */
  if (!dbinit()) {
    error("Couldn't connect to db. bailing out.");
    exit(1);
  }

  if (!opts->debug) {
    if (daemon(0, 0) < 0) {
      errorc("Couldn't become daemon");
      exit(1);
    }
    info("sdcollectd started successfully");
    info(build_info);
  }

  /*
   * Change to non-root user!
   */
  if (geteuid() == 0) {
    struct passwd	*pw;
    uid_t		uid;
    gid_t		gid;

    /*
     * Must be a valid user of course.
     */
    if ((pw = getpwnam(RUNASUSER)) == NULL) {
      error("invalid user: %s", RUNASUSER);
      exit(1);
    }
    uid = pw->pw_uid;
    gid = pw->pw_gid;

    if (setgroups(1, &gid)) {
      errorc("setgroups");
      exit(1);
    }
    if (setgid(gid)) {
      errorc("setgid");
      exit(1);
    }
    if (setuid(uid)) {
      errorc("setuid");
      exit(1);
    }
    info("Flipped to user/group %d/%d\n", uid, gid);
  }

  /* do our thing - just loop collecting data from clients, and insert into
     DB. 
  */
  for ( ;; ) {
    bzero(&iddata, sizeof(IDLE_DATA)); /* cleanse out old values, if any */
    if (CollectData(sd, &iddata)) {
      if (ParseRecord(&iddata)) {
        PutDBRecord(&iddata);
      }
    }
  }
  /* NOTREACHED */
}

int CollectData(int sd, IDLE_DATA *iddata) {
  int numbytes, slen;
  time_t curtime;
  /* struct hostent *hent; */
  struct sockaddr_in cliaddr;

  bzero(&cliaddr, sizeof(struct sockaddr_in));
  slen = sizeof(struct sockaddr_in);

  if((numbytes = recvfrom(sd, iddata->buf, sizeof(iddata->buf), 0,
                          (struct sockaddr*)&cliaddr, &slen))) {

    if (!mydb_iptonodeid(inet_ntoa(cliaddr.sin_addr), iddata->id)) {
      error("Couldn't obtain node id");
      return 0;
    }
    if (opts->debug) {
      curtime = time(NULL);
      printf("Received data from: %s at %s\n", iddata->id, ctime(&curtime));
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


int ParseRecord(IDLE_DATA *iddata) {

  int tmpvers;
  char *itemptr, *ptr, *tmpstr;

  iddata->ifcnt = 0;
  
  /* Parse out fields */
  itemptr = strtok(iddata->buf, " \t");
  if (itemptr == NULL) {
    error("No valid data; rejecting.");
    return 0;
  }

  do {

    if (strstr(itemptr, "vers")) {
      if ((tmpvers = strtoul(itemptr+5, NULL, 10)) > SDPROTOVERS) {
        error("Unsupported protocol version; rejecting.");
        return 0;
      }
      iddata->version = tmpvers;
    }

    else if (strstr(itemptr, "mis")) {
      iddata->mis = atol(itemptr+4);
    }

    else if (strstr(itemptr, "lave")) {
      iddata->l1m = strtod(itemptr+5, &ptr);
      iddata->l5m = strtod(ptr+1, &ptr);
      iddata->l15m = strtod(ptr+1, NULL);
    }

    else if (strstr(itemptr, "iface")) {
      if (!(tmpstr = tbmac(itemptr+6, &ptr))) {
        error("Malformed interface record for node %s encountered: %s",
              iddata->id,
              itemptr);
        continue;
      }
      strcpy(iddata->ifaces[iddata->ifcnt].mac, tmpstr);
      iddata->ifaces[iddata->ifcnt].ipkts = strtoul(ptr+1, &ptr, 10);
      iddata->ifaces[iddata->ifcnt].opkts = strtoul(ptr+1, NULL, 10);
      iddata->ifcnt++;
    }

    else if (strstr(itemptr, "abits")) {
      iddata->actbits = strtoul(itemptr+6, NULL, 16);
    }

    else {
      info("Unrecognized string in packet: %s", itemptr);
    }
  } while ((itemptr = strtok(NULL, " \t")) && iddata->ifcnt < MAXNUMIFACES);

  return 1;
}


void PutDBRecord(IDLE_DATA *iddata) {

  int i;
  time_t now = time(NULL);
  char curstamp[100];
  char tmpstr[(NUMACTTYPES+1)*sizeof(curstamp)];
  char *actstr[] = ACTSTRARRAY;

  sprintf(curstamp, "FROM_UNIXTIME(%lu)", now);

  printf("now: %s\n", ctime(&now));

  if (opts->debug) {
    printf("\nReceived and parsed packet. Contents:\n"
           "Version: %u\n"
           "Node: %s\n"
           "Last TTY: %s"
           "Load averages (1, 5, 15): %f, %f, %f\n"
           "Active bits: 0x%04x\n",
           iddata->version,
           iddata->id,
           ctime(&iddata->mis),
           iddata->l1m,
           iddata->l5m,
           iddata->l15m,
           iddata->actbits);
    for (i = 0; i < iddata->ifcnt; ++i) {
      printf("Interface %s: ipkts: %lu  opkts: %lu\n",
             iddata->ifaces[i].mac,
             iddata->ifaces[i].ipkts,
             iddata->ifaces[i].opkts);
    }
    printf("\n\n");
  }
  
  if (opts->popold) {
    if (!mydb_update("INSERT INTO node_idlestats VALUES ('%s', FROM_UNIXTIME(%lu), FROM_UNIXTIME(%lu), %f, %f, %f)", 
                     iddata->id, 
                     now,
                     iddata->mis, 
                     iddata->l1m, 
                     iddata->l5m, 
                     iddata->l15m)) {
      error("Error inserting data into node_idlestats table");
    }
    
    for (i = 0; i < iddata->ifcnt; ++i) {
      if (!mydb_update("INSERT INTO iface_counters VALUES ('%s', FROM_UNIXTIME(%lu), '%s', %lu, %lu)",
                       iddata->id,
                       now,
                       iddata->ifaces[i].mac,
                       iddata->ifaces[i].ipkts,
                       iddata->ifaces[i].opkts)) {
        error("Error inserting data into iface_counters table");
      }
    }
  }
  
  if (iddata->version >= 2) {
    sprintf(tmpstr, "last_report=%s", curstamp);
    for (i = 0; i < NUMACTTYPES; ++i) {
      if (iddata->actbits & (1<<i)) {
          sprintf(tmpstr, "%s, %s=%s",
                  tmpstr,
                  actstr[i], 
                  curstamp);
      }
    }

    if(!mydb_update("UPDATE node_activity SET %s WHERE node_id = '%s'",
                    tmpstr,
                    iddata->id)) {
      error("Error updating stamps in node_activity table");
    }
  }

  return;
}
