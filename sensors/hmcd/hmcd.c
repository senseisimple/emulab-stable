/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/* Collection agent for healthd 'push' mode - Healthd Master Collection
   Daemon.
*/

#include "hmcd.h"
#include <paths.h>

#ifndef LOG_TESTBED
#define LOG_TESTBED	LOG_DAEMON
#endif

void AcceptClient(int, int*);
void CollectData(int, HMONENT*);
int EmitData(int, HMONENT*);
char *Pidname;

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
  if (unlink(SOCKPATH) < 0 && errno != ENOENT) {
    lerror("Couldn't remove UNIX socket");
    exit(1);
  }
  if (Pidname)
    (void) unlink(Pidname);
  lnotice("Daemon exiting.");
  exit(0);
}

/* XXX:  Comment the code better! */

int main(int argc, char **argv) {

  int sd, usd, i, clients[MAXCLIENTS];
  struct sockaddr_in servaddr;
  struct sockaddr_un unixaddr;
  HMONENT monents[MAX_HOSTS];

  /* Foo on SIGPIPE & SIGHUP. */
  signal(SIGPIPE, SIG_IGN);
  signal(SIGHUP, SIG_IGN);

  /* Cleanup on sigint */
  signal(SIGTERM, siginthandler);

  bzero(monents, sizeof(monents));

  bzero(&servaddr, sizeof(struct sockaddr_in));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(PUSH_PORT);

  bzero(&unixaddr, sizeof(struct sockaddr_un));
  unixaddr.sun_family = AF_LOCAL;
  strcpy(unixaddr.sun_path, SOCKPATH);

  if (strchr(argv[0], '/')) {
    openlog(strrchr(argv[0], '/')+1, 0, LOG_TESTBED);
  }
  else {
    openlog(argv[0], 0, LOG_DAEMON);
  }

  for (i = 0; i < MAXCLIENTS; ++i) {
    clients[i] = -1;
  }

  if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    lerror("Can't create socket");
    exit(1);
  }

  if ((usd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
    lerror("Can't create UNIX domain socket");
    exit(1);
  }
  
  if (unlink(SOCKPATH) < 0 && errno != ENOENT) {
    lerror("Couldn't remove stale UNIX socket");
    exit(1);
  }
  
  if (bind(sd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) < 0) {
    lerror("Can't bind socket");
    exit(1);
  }

  if (bind(usd, (struct sockaddr*)&unixaddr, SUN_LEN(&unixaddr)) < 0) {
    lerror("Can't bind UNIX socket");
    exit(1);
  }

  /* Let anyone send/recv from us */
  if (chmod(SOCKPATH, 00777) < 0) {
    lerror("Can't change socket path permissions");
    exit(1);
  }

  if (listen(usd, 1) < 0) {
    lerror("Can't listen on UNIX socket");
    exit(1);
  }

  /* Become daemon */
  if (daemon(0, 0) < 0) {
    lerror("Couldn't become daemon");
    exit(1);
  }
  if (!getuid()) {
    FILE	*fp;
    char    mybuf[BUFSIZ];
	    
    sprintf(mybuf, "%s/hmcd.pid", _PATH_VARRUN);
    fp = fopen(mybuf, "w");
    if (fp != NULL) {
      fprintf(fp, "%d\n", getpid());
      (void) fclose(fp);
      Pidname = strdup(mybuf);
    }
  }

  lnotice("Daemon started successfully");
  
  for ( ; ; ) {
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(sd, &rfds);
    FD_SET(usd, &rfds);
    for (i = 0; i < MAXCLIENTS; ++i) {
      if (clients[i] > -1) {
        FD_SET(clients[i], &rfds);
      }
    }

    /* XXX:  Add check for maxclients; don't add usd to rfds if so. */

    if (select(getdtablesize(), &rfds, NULL, NULL, NULL) < 0) {
      continue;
    }

    if (FD_ISSET(sd, &rfds)) {
      CollectData(sd, monents);
    }

    if (FD_ISSET(usd, &rfds)) {
      AcceptClient(usd, clients);
    }

    for (i = 0; i < MAXCLIENTS; ++i) {
      if (clients[i] > -1 && FD_ISSET(clients[i], &rfds)) {
        if (EmitData(clients[i], monents)) {
          close(clients[i]);
          clients[i] = -1;
        }
      }
    }
  }
}

void AcceptClient(int usd, int clients[]) {
  int clilen, i;
  struct sockaddr_un cliaddr;

  bzero(&cliaddr, sizeof(struct sockaddr_un));
  clilen = SUN_LEN(&cliaddr);
  
  for(i = 0; i < MAXCLIENTS; ++i) {
    if (clients[i] < 0)  break;
  }

  if (i != MAXCLIENTS) {
    if ((clients[i] = accept(usd, (struct sockaddr*)&cliaddr, &clilen)) < 0) {
      lerror("Unable to accept incoming connection");
      return;
    }
  }
  else {
    /* XXX:  Shouldn't get here, but need to add an accept->close sequence */
  }
  return;
}

void CollectData(int sd, HMONENT monents[]) {
  int numbytes, slen, curid = 0;
  char *hnumptr = NULL;
  char buf[1024];
  struct hostent *hent;
  struct sockaddr_in cliaddr;

  bzero(&cliaddr, sizeof(struct sockaddr_in));
  bzero(buf, sizeof(buf));
  slen = sizeof(struct sockaddr_in);

  if((numbytes = recvfrom(sd, buf, sizeof(buf), 0,
                          (struct sockaddr*)&cliaddr, &slen))) {
    if (!(hent = gethostbyaddr((char*)&cliaddr.sin_addr, 
                               sizeof(cliaddr.sin_addr), AF_INET))) {
      lerror("Can't resolve host");
      return;
    }
    else {
      /*      printf("Received data from: %s\n", hent->h_name); */
      hnumptr = hent->h_name;
      while (*hnumptr && !isdigit(*hnumptr)) {
        hnumptr++;
      }
      if (!*hnumptr || (curid = atoi(hnumptr)) < 0 || curid > MAX_HOSTS) {
        return;
      }
      else {
        if (monents[curid].id == NULL) {
          monents[curid].id = strdup(hent->h_name);
        }
        else {
          free(monents[curid].data);
        }
        monents[curid].data = strdup(buf);
      }
    }
    /*    printf("%s\n", buf); */
  }
  return;
}

#define MAXCMDSIZE 100

int EmitData(int csd, HMONENT monents[]) {
  int curid, numbytes;
  char cmd[MAXCMDSIZE], *mname;

  bzero(cmd, sizeof(cmd));
  
  if ((numbytes = recv(csd, cmd, sizeof(cmd), 0)) > 0) {
    if (cmd[numbytes-1] != '\0')  cmd[numbytes-1] = '\0';
    if (!strncmp(cmd, "GET ", 4)) {
      mname = &cmd[4];
      while (*mname && !isdigit(*mname)) {
        mname++;
      }
      if (!*mname || (curid = atoi(mname)) < 0 || curid > MAX_HOSTS) {
        return 1;
      }
      if (monents[curid].data) {
        if (send(csd, monents[curid].data, 
                 strlen(monents[curid].data)+1, 0) < 0) {
          return 1;
        }
      }
      else {
        if (send(csd, "NULL", 5, 0) < 0) {
          return 1;
        }
      }
    }
  }
  else if (numbytes == 0) {
    return 1;
  }
  return 0;
}
