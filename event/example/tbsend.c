/*
 * This is a sample event generator to send TBEXAMPLE events to all nodes
 * in an experiment. Modify as needed of course.
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

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] <event>\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	event_handle_t handle;
	event_notification_t notification;
	address_tuple_t	tuple;
	char *server = NULL;
	char *port = NULL;
	char buf[BUFSIZ], *bp;
	struct timeval	now;
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

	if (argc != 1)
		usage();
	
	loginit(0, 0);

	/*
	 * Uppercase event tags for now. Should be wired in list instead.
	 */
	bp = argv[0];
	while (*bp) {
		*bp = toupper(*bp);
		bp++;
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
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	tuple->objtype  = "TBEXAMPLE";
	tuple->eventtype= argv[0];
	tuple->host	= ADDRESSTUPLE_ALL;

	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	/* Generate the event */
	notification = event_notification_alloc(handle, tuple);
	
	if (notification == NULL) {
		fatal("could not allocate notification");
	}

	gettimeofday(&now, NULL);

	if (event_notify(handle, notification) == 0) {
		fatal("could not send test event notification");
	}
	info("Sent at time: %lu:%d\n", now.tv_sec, now.tv_usec);

	event_notification_free(handle, notification);

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}
	
	return 0;
}
