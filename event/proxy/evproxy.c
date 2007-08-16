/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 *
 */
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include "config.h"
#include "event.h"
#include "tbdefs.h"
#include "log.h"

static int debug = 0;
static int stop  = 0;
static event_handle_t localhandle;
static event_handle_t bosshandle;

void
usage(char *progname)
{
    fprintf(stderr,
	    "Usage: %s [-s server] [-i pidfile] -e pid/eid\n", progname);
    exit(-1);
}

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

static void
sched_callback(event_handle_t handle,
	       event_notification_t notification, void *data);

static void
sigterm(int sig)
{
	stop = 1;
}

int
main(int argc, char **argv)
{
	address_tuple_t		tuple;
	char			*progname;
	char			*server = NULL;
	char			*port = NULL;
	char			*myeid = NULL;
	char			*pidfile = NULL;
	char			*vnodeid = NULL;
	char			buf[BUFSIZ], ipaddr[32];
	char			hostname[MAXHOSTNAMELEN];
	struct hostent		*he;
	int			c;
	struct in_addr		myip;
	FILE			*fp;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "ds:p:e:i:v:")) != -1) {
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
		case 'i':
			pidfile = optarg;
			break;
		case 'e':
			myeid = optarg;
			break;
		case 'v':
			vnodeid = optarg;
			break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc)
		usage(progname);

	if (! myeid)
		fatal("Must provide pid/eid");

	if (debug)
		loginit(0, 0);
	else {
		loginit(1, "evproxy");
		/* See below for daemonization */
	}

	/*
	 * Get our IP address. Thats how we name this host to the
	 * event System. 
	 */
	if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
		fatal("could not get hostname: %s\n", strerror(errno));
	}
	if (! (he = gethostbyname(hostname))) {
		fatal("could not get IP address from hostname: %s", hostname);
	}
	memcpy((char *)&myip, he->h_addr, he->h_length);
	strcpy(ipaddr, inet_ntoa(myip));

	/*
	 * If server is not specified, then it defaults to EVENTSERVER.
	 * This allows the client to work on either users.emulab.net
	 * or on a client node. 
	 */
	if (!server)
		server = EVENTSERVER;

	/*
	 * XXX Need to daemonize earlier or the threads go away.
	 */
	if (!debug)
		daemon(0, 0);
	
	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	snprintf(buf, sizeof(buf), "elvin://%s%s%s",
		 server,
		 (port ? ":"  : ""),
		 (port ? port : ""));
	server = buf;

	/*
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	
	/* Register with the event system on boss */
	bosshandle = event_register(server, 1);
	if (bosshandle == NULL) {
		fatal("could not register with remote event system");
	}

	/* Register with the event system on the local node */
	localhandle = event_register("elvin://localhost", 1);
	if (localhandle == NULL) {
		fatal("could not register with local event system");
	}
	
	/*
	 * Create a subscription to pass to the remote server. We want
	 * all events for this node, or all events for the experiment
	 * if the node is unspecified (we want to avoid getting events
	 * that are directed at specific nodes that are not us!). 
	 */
	sprintf(buf, "%s,%s", ADDRESSTUPLE_ALL, ipaddr);
	tuple->host = buf;
	tuple->expt = myeid;

	/* Subscribe to the test event: */
	if (! event_subscribe(bosshandle, callback, tuple, "event received")) {
		fatal("could not subscribe to events on remote server");
	}

	tuple->host = ADDRESSTUPLE_ALL;
	tuple->scheduler = 1;
	
	if (! event_subscribe(localhandle, sched_callback, tuple, NULL)) {
		fatal("could not subscribe to events on remote server");
	}

	signal(SIGTERM, sigterm);

	/*
	 * Stash the pid away.
	 */
	if (pidfile)
		strcpy(buf, pidfile);
	else
		sprintf(buf, "%s/evproxy.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	/* Begin the event loop, waiting to receive event notifications */
	while (! stop) {
		struct timeval  tv = { 5, 0 };

		select(0, NULL, NULL, NULL, &tv);
	}

	/* Unregister with the remote event system: */
	if (event_unregister(bosshandle) == 0) {
		fatal("could not unregister with remote event system");
	}
	/* Unregister with the local event system: */
	if (event_unregister(localhandle) == 0) {
		fatal("could not unregister with local event system");
	}

	return 0;
}

/*
 * Handle incoming events from the remote server. 
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		eventtype[TBDB_FLEN_EVEVENTTYPE];
	char		objecttype[TBDB_FLEN_EVOBJTYPE];
	char		objectname[TBDB_FLEN_EVOBJNAME];

	if (debug) {
		event_notification_get_eventtype(handle,
				notification, eventtype, sizeof(eventtype));
		event_notification_get_objtype(handle,
				notification, objecttype, sizeof(objecttype));
		event_notification_get_objname(handle,
				notification, objectname, sizeof(objectname));

		info("%s %s %s\n", eventtype, objecttype, objectname);
	}
	/*
	 * Resend the notification to the local server.
	 */
	if (! event_notify(localhandle, notification))
		error("Failed to deliver notification!");
}

static void
sched_callback(event_handle_t handle,
	       event_notification_t notification,
	       void *data)
{
	if (! event_notify(bosshandle, notification))
		error("Failed to deliver scheduled notification!");
}
