/*
 * This is used to send Testbed Control Event Daemon. Its purpose
 * is to handle TBCONTROL events.
 *
 * TODO: Needs to be daemonized.
 */

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

static char	*progname;
static int	debug = 0;

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
	
	while ((c = getopt(argc, argv, "s:p:d:")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
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

	loginit("tbmevd", !debug);

	/*
	 * Set up DB state.
	 */
	if (!dbinit())
		return 1;

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
	tuple->objtype  = OBJECTTYPE_TESTBED;

	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	/* Subscribe to the test event: */
	if (! event_subscribe(handle, callback, tuple, "event received")) {
		fatal("could not subscribe to event");
	}
	
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
	char		nodeid[TBDB_FLEN_NODEID];
	char		eventtype[TBDB_FLEN_EVEVENTTYPE];
	char		ipaddr[32];

	event_notification_get_host(handle, notification,
				    ipaddr, sizeof(ipaddr));
	
	event_notification_get_eventtype(handle, notification,
					 eventtype, sizeof(eventtype));

	/*
	 * Convert to nodeid.
	 */
	if (! mydb_iptonodeid(ipaddr, nodeid)) {
		error("Could not map ipaddr %s to nodeid!", ipaddr);
		return;
	}

	/*
	 * Set the event status for this node.
	 */
	if (! mydb_setnodeeventstate(nodeid, eventtype)) {
		error("Error setting node event state: %s/%s!",
		      nodeid, eventtype);
		return;
	}
	info("%s(%s) -> %s", ipaddr, nodeid, eventtype);
}
