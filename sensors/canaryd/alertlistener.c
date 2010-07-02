/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002, 2008 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This is used to send Testbed Control Event Daemon. Its purpose
 * is to handle TBCONTROL events.
 *
 * TODO: Needs to be daemonized.
 */

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event.h"
#include "log.h"
#include "tbdb.h"

static char     *outfilepath;
static char	*progname;
static int	debug = 0;

void
usage()
{
	fprintf(stderr,	"Usage: %s [-s server] [-p port] [-f <outfile>] -e pid/eid\n",
		progname);
	exit(-1);
}

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

int
main(int argc, char **argv)
{
	event_handle_t handle;
	address_tuple_t	tuple;
	char *server = "boss.emulab.net";
	char *port = NULL;
        char *pideid = NULL;
	char buf[BUFSIZ];
	int c;
	
	progname = argv[0];
        outfilepath = "./sdalerts";
	
	while ((c = getopt(argc, argv, "s:p:de:f:")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
                case 'e':
                        pideid = optarg;
                        break;
                case 'f':
                        outfilepath = optarg;
                        break;
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

        if (!pideid) {
          fatal("a pid/eid must be specified!");
        }

	if (debug)
		loginit(0, 0);
	else {
		loginit(1, "tevd");
		/* See below for daemonization */
	}

	/*
	 * Set up DB state.
	 */
#ifdef COMMENT
	if (!dbinit())
		return 1;
#endif
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
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
        tuple->host	 = ADDRESSTUPLE_ANY;
        tuple->site      = ADDRESSTUPLE_ANY;
        tuple->group     = ADDRESSTUPLE_ANY;
        tuple->expt      = pideid;
        tuple->objtype   = TBDB_OBJECTTYPE_CANARYD;
        tuple->objname   = ADDRESSTUPLE_ANY;
        tuple->eventtype = TBDB_EVENTTYPE_ALERT;

	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	/* Subscribe to the test event: */
	if (! event_subscribe(handle, callback, tuple, "event received")) {
		fatal("could not subscribe to event");
	}

	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug)
		daemon(0, 0);
	
	/* Begin the event loop, waiting to receive event notifications: */
	event_main(handle);

	/* Unregister with the event system: */
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
	char		eventtype[TBDB_FLEN_EVEVENTTYPE];
	char		ipaddr[32];
        char            args[2000];
        struct timeval  now;
        static FILE     *ofile;

        gettimeofday(&now, 0);

	event_notification_get_sender(handle, notification,
				      ipaddr, sizeof(ipaddr));
	
	event_notification_get_eventtype(handle, notification,
					 eventtype, sizeof(eventtype));

        event_notification_get_arguments(handle, notification, 
                                         args, sizeof(args));

	/* info("%s -> %s -> %s\n", ipaddr, eventtype, args); */

        if (!ofile) {
          if ((ofile = fopen(outfilepath, "w")) == NULL) {
            fatal("Error opening alertlistener output file!");
          }
        }
        
        if (ferror(ofile)) {
          fatal("Output file error");
        }

        fprintf(ofile, "time=%lu,%lu %s\n", now.tv_sec, now.tv_usec, args);
        fflush(ofile);
}
