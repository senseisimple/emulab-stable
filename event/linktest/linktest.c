/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003 University of Utah and the Flux Group.
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

static void	callback(event_handle_t handle,
			 event_notification_t notification, void *data);

static void
start_linktest(char *args);

void
usage(char *progname)
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-l logfile] -e pid/eid\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv) {
	event_handle_t handle;
	address_tuple_t	tuple;
	char *server = NULL;
	char *port = NULL;
	char *pideid = NULL;
	char *logfile = NULL;
	char *progname;
	char c;
	char buf[BUFSIZ];
	
	progname = argv[0];

	while ((c = getopt(argc, argv, "s:p:e:")) != -1) {
	  switch (c) {
	  case 's':
	    server = optarg;
	    break;
	  case 'p':
	    port = optarg;
	    break;
	  case 'e':
	    pideid = optarg;
	    break;
	  case 'l':
	    logfile = optarg;
	    break;
	  
	  default:
	    usage(progname);
	  }
	}

	if (!pideid)
	  usage(progname);

        info("starting up\n");

	loginit(0, logfile);

	
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
	 * Ask for just the program agents we care about. 
	 */
	tuple->expt      = pideid;
	tuple->objtype   = TBDB_OBJECTTYPE_LINKTEST;
	tuple->objname   = "linktest";
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	info("event_register...\n");

	handle = event_register(server, 0);
	if (handle == NULL) {
		fatal("could not register with event system");
	}
	info("registered with event system\n");

	
	/*
	 * Subscribe to the event we specified above.
	 */
	if (! event_subscribe(handle, callback, tuple, NULL)) {
		fatal("could not subscribe to event");
	}
	info("subscribed to event\n");
	
	
	/*
	 * Begin the event loop, waiting to receive event notifications:
	 */
	info("starting main event loop\n");
	event_main(handle);
	info("end main event loop\n");	

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

	info("got a callback\n");

	
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

        info("Event: %lu:%d %s %s %s\n", now.tv_sec, now.tv_usec,
	     objname, event, args);
	/*
	 * Dispatch the event. 
	 */
	if (strcmp(event, TBDB_EVENTTYPE_START) == 0)
		start_linktest(args);
	else {
		error("Invalid event: %s\n", event);
		return;
	}

}

static void
start_linktest(char *args) {
  info("starting linktest.\n");
  info(LINKTEST_SCRIPT);
}
