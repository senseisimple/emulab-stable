/*
 * This is used to send Testbed Control Event Daemon. Its purpose
 * is to handle TBCONTROL events.
 *
 * TODO: Needs to be daemonized.
 */

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "event.h"

static char	*progname;

void
usage()
{
	fprintf(stderr,	"Usage: %s [-s server] [-p port]\n",
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
	char *server = NULL;
	char *port = NULL;
	char buf[BUFSIZ];
	int c;
	
	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:")) != -1) {
		switch (c) {
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
		ERROR("could not allocate an address tuple\n");
		return 1;
	}
	tuple->objtype  = OBJECTTYPE_TESTBED;

	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		ERROR("could not register with event system\n");
		return 1;
	}

	/* Subscribe to the test event: */
	if (! event_subscribe(handle, callback, tuple, "event received")) {
		ERROR("could not subscribe to event\n");
		return 1;
	}
	
	/* Begin the event loop, waiting to receive event notifications: */
	event_main(handle);

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		ERROR("could not unregister with event system\n");
		return 1;
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

	printf("Event: %lu %s %s %s %s %s %s %s\n", now.tv_sec,
	       buf[0], buf[1], buf[2], 
	       buf[3], buf[4], buf[5], buf[6]);
}
