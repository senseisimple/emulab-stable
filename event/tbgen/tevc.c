/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * This is used to send Testbed events (reboot, ready, etc.) to the
 * testbed event client. It is intended to be wrapped up by a perl
 * script that will use the tmcc to figure out where the event server
 * lives (host/port) to construct the server string.
 *
 * Issues: key and pid/eid.
 *         valid events, names, types.
 *         valid arguments.
 *         vname to ipaddr mapping.
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "tbdefs.h"
#include "event.h"

static int debug;

void
usage(char *progname)
{
    fprintf(stderr,
	"Usage: %s [-s server] [-c] event\n"
	"       %s [-s server] -e pid/eid time objname event [args ...]\n"
	"       time: 'now' or '+seconds' or [[[[yy]mm]dd]HH]MMss\n"
	"Examples:\n"
	"       %s -e pid/eid now cbr0 set interval_=0.2\n"
	"       %s -e pid/eid +10 cbr0 start\n"
	"       %s -e pid/eid +20 cbr0 stop\n",
	progname, progname, progname, progname, progname);
    exit(-1);
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	event_notification_t notification;
	address_tuple_t	tuple;
	char *progname;
	char *server = NULL;
	char *port = NULL;
	int control = 0;
	char *myeid = NULL;
	char buf[BUFSIZ], *bp;
	char *evtime = NULL, *objname = NULL, *event;
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "ds:p:ce:")) != -1) {
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
		case 'c':
			control = 1;
			break;
		case 'e':
			myeid = optarg;
			break;
		default:
			usage(progname);
		}
	}
	argc -= optind;
	argv += optind;

	if (control) {
		if (geteuid())
			fatal("Only root can send TBCONTROL events");
		
		if (argc != 1)
			usage(progname);
		
		event = argv[0];
		argc -= 1;
		argv += 1;
	}
	else {
		if (argc < 3)
			usage(progname);

		evtime  = argv[0];
		objname = argv[1];
		event   = argv[2];

		argc -= 3;
		argv += 3;
	}
	loginit(0, 0);

	/*
	 * If server is not specified, then it defaults to BOSSNODE.
	 * This allows the client to work on either users.emulab.net
	 * or on a client node. 
	 */
	if (!server)
		server = BOSSNODE;

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
	
	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	if (control) {
		/*
		 * Send a control event.
		 */
		bp = event;
		while (*bp) {
			*bp = toupper(*bp);
			bp++;
		}
		if (! tbdb_valideventtype(event))
			fatal("Unknown %s event: %s",
			      OBJECTTYPE_TESTBED, event);
		
		tuple->objtype  = OBJECTTYPE_TESTBED;
		tuple->eventtype= event;
	}
	else {
		/*
		 * A dynamic event. 
		 */
		if (! myeid)
			fatal("Must provide pid/eid");

		bp = event;
		while (*bp) {
			*bp = toupper(*bp);
			bp++;
		}
		if (!strcmp(event, "SET"))
			event = TBDB_EVENTTYPE_MODIFY;
		else if (! tbdb_valideventtype(event))
			fatal("Unknown event: %s", event);
		
		tuple->objname   = objname;
		tuple->eventtype = event;
		tuple->expt      = myeid;
		tuple->site      = ADDRESSTUPLE_ANY;
		tuple->host      = ADDRESSTUPLE_ANY;
		tuple->group     = ADDRESSTUPLE_ANY;
	}

	/* Generate the event */
	notification = event_notification_alloc(handle, tuple);
	
	if (notification == NULL) {
		fatal("could not allocate notification");
	}

	/*
	 * If there are any extra arguments, insert them into
	 * the notification as an arg string.
	 *
	 * XXX For now, uppercase the strings, and remove trailing _.
	 */
	if (argc) {
		buf[0] = NULL;
		while (argc) {
			if (strlen(*argv) + strlen(buf) >= sizeof(buf)-2)
				fatal("Too many event argument strings!");

			bp = *argv;
			while (*bp && *bp != '=') {
				*bp = toupper(*bp);
				bp++;
			}
			if (*bp != '=')
				fatal("Malformed argument: %s!", *argv);
			if (*(bp-1) == '_')
				*(bp-1) = NULL;
			*bp++ = NULL;

			sprintf(&buf[strlen(buf)], "%s=%s ", *argv, bp);
			argc--;
			argv++;
		}
		event_notification_set_arguments(handle, notification, buf);
	}

	if (control) {
		if (event_notify(handle, notification) == 0) {
			fatal("could not send test event notification");
		}
	}
	else {
		struct timeval when;
		
		/*
		 * Parse the time. Now is special; set the time to 0.
		 */
		if (!strcmp(evtime, "now")) {
			gettimeofday(&when, NULL);
		}
		else if (evtime[0] == '+') {
			gettimeofday(&when, NULL);

			if (strchr(evtime, '.')) {
				double	val = atof(evtime);

				when.tv_sec  += (int) rint(val);
				when.tv_usec +=
					(int) (1000000 * (val - rint(val)));

				if (when.tv_usec > 1000000) {
					when.tv_sec  += 1;
					when.tv_usec -= 1000000;
				}
			}
			else
				when.tv_sec += atoi(evtime);
		}
		else {
			char	 *format = "%y%m%d%H%M%S";
			int	  len = strlen(evtime);
			struct tm tm;
			
			if ((len & 1) || (len > strlen(format)) || (len < 4))
				usage(progname);
			format += strlen(format) - len;

			gettimeofday(&when, NULL);
			localtime_r(&when.tv_sec, &tm);
			if (!strptime(evtime, format, &tm))
				usage(progname);

			when.tv_sec  = mktime(&tm);
			when.tv_usec = 0;
		}

		if (debug) {
			struct timeval now;
			
			gettimeofday(&now, NULL);
			
		}
		
		if (event_schedule(handle, notification, &when) == 0) {
			fatal("could not send test event notification");
		}
	}

	event_notification_free(handle, notification);

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}
	
	return 0;
}
