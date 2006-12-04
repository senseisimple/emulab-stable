/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This is a sample client to run on a testbed node to capture TBEXAMPLE
 * events for the node. Modify as needed of course.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "log.h"
#include "event.h"

static char	*progname;

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-k keyfile]\n", progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	address_tuple_t	tuple;
	char *server = NULL;
	char *keyfile = NULL;
	char *port = NULL;
	char *ipaddr = NULL;
	char buf[BUFSIZ], ipbuf[BUFSIZ];
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:k:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'k':
			keyfile = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	loginit(0, 0);

	/*
	 * Get our IP address. Thats how we name ourselves to the
	 * Testbed Event System. 
	 */
	if (ipaddr == NULL) {
	    struct hostent	*he;
	    struct in_addr	myip;
	    
	    if (gethostname(buf, sizeof(buf)) < 0) {
		fatal("could not get hostname");
	    }

	    if (! (he = gethostbyname(buf))) {
		fatal("could not get IP address from hostname: %s", buf);
	    }
	    memcpy((char *)&myip, he->h_addr, he->h_length);
	    strcpy(ipbuf, inet_ntoa(myip));
	    ipaddr = ipbuf;
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
	 * Change this stuff as needed. 
	 */
	tuple->host	 = ADDRESSTUPLE_ALL;
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = ADDRESSTUPLE_ANY;	/* pid/eid */
	tuple->objtype   = "TBEXAMPLE";
	tuple->objname   = ADDRESSTUPLE_ANY;
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
	 * Begin the event loop, waiting to receive event notifications:
	 */
	event_main(handle);

	/*
	 * Or, we can use a blocking poll like so:
	 */
	
	/*
	while (1) {
		event_poll_blocking(handle,0);
	}
	*/

	/*
	 * Unregister with the event system:
	 */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}

	return 0;
}

/*
 * Handle Testbed Control events.
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		buf[7][64];
	int		len = 64;
	struct timeval	now;

	gettimeofday(&now, NULL);

	event_notification_get_site(handle, notification, buf[0], len);
	event_notification_get_expt(handle, notification, buf[1], len);
	event_notification_get_group(handle, notification, buf[2], len);
	event_notification_get_host(handle, notification, buf[3], len);
	event_notification_get_objtype(handle, notification, buf[4], len);
	event_notification_get_objname(handle, notification, buf[5], len);
	event_notification_get_eventtype(handle, notification, buf[6], len);

	info("Event: %lu:%d %s %s %s %s %s %s %s\n", now.tv_sec, now.tv_usec,
	     buf[0], buf[1], buf[2], 
	     buf[3], buf[4], buf[5], buf[6]);

	exit(0);
}
