/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pwd.h>
#include <time.h>
#include "tbdefs.h"
#include "log.h"
#include "event.h"
#include "linktest.h"

static int	debug;
static void	callback(event_handle_t handle,
			 event_notification_t notification, void *data);

static void
start_linktest(char *args, int);

void
usage(char *progname)
{
	fprintf(stderr,
		"Usage: %s [-d] "
		"[-s server] [-p port] [-k keyfile] [-l logfile] -e pid/eid\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv) {
	event_handle_t handle;
	address_tuple_t	tuple;
	char *server = NULL;
	char *port = NULL;
	char *keyfile = NULL;
	char *pideid = NULL;
	char *pidfile = NULL;
	char *logfile = NULL;
	char *progname;
	char c;
	char buf[BUFSIZ];
	
	progname = argv[0];

	while ((c = getopt(argc, argv, "s:p:e:l:dk:i:")) != -1) {
	  switch (c) {
	  case 'd':
	    debug++;
	    break;
	  case 's':
	    server = optarg;
	    break;
	  case 'p':
	    port = optarg;
	    break;
	  case 'e':
	    pideid = optarg;
	    break;
	  case 'i':
	    pidfile = optarg;
	    break;
	  case 'l':
	    logfile = optarg;
	    break;
	  case 'k':
	    keyfile = optarg;
	    break;
	  default:
	    usage(progname);
	  }
	}

	if (!pideid)
	  usage(progname);

	if (debug)
		loginit(0, 0);
	else {
		if (logfile)
			loginit(0, logfile);
		else
			loginit(1, "linktest");
		/* See below for daemonization */
	}

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	if (server) {
		snprintf(buf, sizeof(buf), "elvin://%s%s%s",
			 server,
			 (port ? ":"  : ""),
			 (port ? port : ""));
		server = buf;
	}

	/*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	/*
	 * Ask for just the events we care about. 
	 */
	tuple->expt      = pideid;
	tuple->objtype   = TBDB_OBJECTTYPE_LINKTEST;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	handle = event_register_withkeyfile(server, 0, keyfile);
	if (handle == NULL) {
	  fatal("could not register with event system");
	}
	
	/*
	 * Subscribe to the event we specified above.
	 */
	if (! event_subscribe(handle, callback, tuple, NULL)) {
		fatal("could not subscribe to event");
	}

	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug)
		daemon(0, 1);

	/*
	 * Write out a pidfile if root (after we daemonize).
	 */
	if (!getuid()) {
		FILE *fp;
		
		if (pidfile)
			strcpy(buf, pidfile);
		else
			sprintf(buf, "%s/linktest.pid", _PATH_VARRUN);
		fp = fopen(buf, "w");
		if (fp != NULL) {
			fprintf(fp, "%d\n", getpid());
			(void) fclose(fp);
		}
	}

	/*
	 * Begin the event loop, waiting to receive event notifications:
	 */
	event_main(handle);

	/*
	 * Unregister with the event system:
	 */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}

	return 0;
}
/*
 * Handle the events.
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		objname[TBDB_FLEN_EVOBJTYPE];
	char		event[TBDB_FLEN_EVEVENTTYPE];
	char		args[BUFSIZ];
	struct timeval	now;

	gettimeofday(&now, NULL);
	
	if (! event_notification_get_objname(handle, notification,
					     objname, sizeof(objname))) {
		error("Could not get objname from notification!\n");
		return;
	}

	if (! event_notification_get_eventtype(handle, notification,
					       event, sizeof(event))) {
		error("Could not get event from notification!\n");
		return;
	}

	event_notification_get_arguments(handle,
					 notification, args, sizeof(args));

/*        info("Event: %lu:%d %s %s %s\n", now.tv_sec, now.tv_usec,
	     objname, event, args);*/
	/*
	 * Dispatch the event. 
	 */
	if (strcmp(event, TBDB_EVENTTYPE_START) == 0) {
          start_linktest(args, sizeof(args));
	}

}

/* convert arguments from the event into a form for Linktest.
 */
static void
start_linktest(char *args, int buflen) {
  pid_t lt_pid;
  int status;
  char *word;
  char *argv[MAX_ARGS];
  int i=1;
  
  word = strtok(args," \t");
  do {
    argv[i++] = word;
  } while ((word = strtok(NULL," \t"))
           && (i<MAX_ARGS));
  argv[i] = NULL;
  argv[0] = LINKTEST_SCRIPT;
/*  info("starting linktest.\n");*/
  
  lt_pid = fork();
  if(!lt_pid) {
    execv( LINKTEST_SCRIPT,argv);
  }
  waitpid(lt_pid, &status, 0);
/*  info("linktest completed.\n");*/
}

