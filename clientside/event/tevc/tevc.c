/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2011 University of Utah and the Flux Group.
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
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "tbdefs.h"
#include "event.h"
#include "log.h"

static int debug;

static int exit_value = 0;
static int completion_token;
static int wait_for_complete;

static event_handle_t handle;

static void     comp_callback(event_handle_t handle,
			      event_notification_t notification,
			      void *data);

void
sigalrm(int sig)
{
	event_stop_main(handle);
	exit_value = ETIMEDOUT;
}

void
usage(char *progname)
{
    fprintf(stderr,
	"Usage: %s [-w] [-s server] [-c] event\n"
	"       %s [-w] [-s server] [-k keyfile] -e pid/eid time objname "
	"event [args ...]\n"
	"       time: 'now' or '+seconds' or [[[[yy]mm]dd]HH]MMss\n"
	"Options:\n"
	"       -w    Wait for the event to complete.\n"
	"       -t N  Timeout after waiting N seconds.\n"
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
	event_notification_t notification;
	address_tuple_t	tuple;
	char *progname;
	char *server = NULL;
	char *port = NULL;
	int control = 0;
	int timeout = 0;
	char *myeid = NULL;
	char *keyfile = NULL;
	char keyfilebuf[BUFSIZ];
	char buf[BUFSIZ], *bp;
	char *evtime = NULL, *objname = NULL, *event;
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "dws:p:ce:k:t:")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 'w':
			wait_for_complete = 1;
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
		case 'k':
			keyfile = optarg;
			break;
		case 't':
			timeout = atoi(optarg);
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
	if (!server) {
		/*
		 * We should kill the control events from tevc; nothing
		 * uses it to send control events. Note that since the
		 * event port on boss is firewalled, you have to be on boss
		 * to use tevc to send a control event.
		 */
		if (control)
			server = "localhost";
		else
			server = "event-server";
	}

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

	if (!control) {
		/*
		 * A dynamic event. 
		 */
		if (! myeid)
			fatal("Must provide pid/eid");

		/*
		 * If no keyfile specified, construct the name and
		 * make sure it can be accessed. Only users with access
		 * to the keyfile can send events to the pid/eid specified.
		 */
		if (! keyfile) {
			char    temp[BUFSIZ], *bp;

			strncpy(temp, myeid, sizeof(temp));
			
			if ((bp = strchr(temp, '/')) == NULL)
				fatal("Malformed pid/eid: %s", myeid);
			*bp++ = '\0';

			sprintf(keyfilebuf, "/proj/%s/exp/%s/tbdata/eventkey",
				temp, bp);
			keyfile = keyfilebuf;
		}

		/*
		 * Make sure the keyfile exists and is readable.
		 */
		if (access(keyfile, R_OK) < 0) {
			pfatal("Could not read %s", keyfile);
		}
	}

	/* Register with the event system: */
	if (control)
		handle = event_register(server, 0);
	else
		handle = event_register_withkeyfile(server, 0, keyfile);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	if (wait_for_complete) {
		tuple->expt      = myeid;
		tuple->eventtype = TBDB_EVENTTYPE_COMPLETE;
		tuple->objname   = objname;
		
		if (! event_subscribe(handle, comp_callback, tuple, NULL)) {
			fatal("could not subscribe to event");
		}
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
		buf[0] = '\0';
		while (argc) {
			if (strlen(*argv) + strlen(buf) >= sizeof(buf)-2)
				fatal("Too many event argument strings!");

			if ( strchr(*argv, '=') == 0 ) {
			    /* Tcl strings are sent in an NSEEVENT. We
			     * will allow arbitrary strings
			     */
			    sprintf(&buf[strlen(buf)], "%s", *argv);
			} else {
			    bp = *argv;
			    while (*bp && *bp != '=') {
				*bp = toupper(*bp);
				bp++;
			    }
			    if (*(bp-1) == '_')
				*(bp-1) = '\0';
			    *bp++ = '\0';

			    sprintf(&buf[strlen(buf)], "%s=%s ", *argv, bp);
			}
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
			localtime_r((time_t *)&when.tv_sec, &tm);
			if (!strptime(evtime, format, &tm))
				usage(progname);

			when.tv_sec  = mktime(&tm);
			when.tv_usec = 0;
		}

		if (wait_for_complete) {
			struct timeval now;
			
			/*
			 * If waiting for a complete event, lets stick our
			 * own token in so we can wait for the proper
			 * completion event.
			 */			
			gettimeofday(&now, NULL);

			completion_token = now.tv_sec;

			event_notification_put_int32(handle,
						     notification,
						     "TOKEN",
						     completion_token);
		}
		
		if (event_schedule(handle, notification, &when) == 0) {
			fatal("could not send test event notification");
		}
	}

	event_notification_free(handle, notification);

	if (wait_for_complete) {
		if (timeout) {
			signal(SIGALRM, sigalrm);
			alarm(timeout);
		}
		event_main(handle);
	}

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}
	
	return exit_value;
}

void comp_callback(event_handle_t handle,
		   event_notification_t notification,
		   void *data)
{
	char *value, argsbuf[BUFSIZ] = "";
	int ctoken;

	event_notification_get_arguments(handle, notification,
					 argsbuf, sizeof(argsbuf));

	if (event_arg_get(argsbuf, "CTOKEN", &value) > 0) {
		if (sscanf(value, "%d", &ctoken) != 1) {
			error("bad ctoken value for complete: %s\n", argsbuf);
		}
		if (ctoken != completion_token)
			return;
	}
	else {
		return;
	}
	
	if (event_arg_get(argsbuf, "ERROR", &value) > 0) {
		if (sscanf(value, "%d", &exit_value) != 1) {
			error("bad error value for complete: %s\n", argsbuf);
		}
	}
	else {
		warning("completion event is missing ERROR argument\n");
	}

	event_stop_main(handle);
}
