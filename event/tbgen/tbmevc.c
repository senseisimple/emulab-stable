/*
 * This is used to send Testbed events (reboot, ready, etc.) to the
 * testbed event client. It is intended to be wrapped up by a perl
 * script that will use the tmcc to figure out where the event server
 * lives (host/port) to construct the server string. 
 */

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "event.h"

static char	*progname;

void
usage()
{
	fprintf(stderr,
		"Usage: %s [-s server] [-p port] [-i ipaddr] <event>\n",
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
	char *ipaddr = NULL;
	char buf[BUFSIZ], ipbuf[BUFSIZ], *bp;
	int c;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "s:p:i:")) != -1) {
		switch (c) {
		case 's':
			server = optarg;
			break;
		case 'p':
			port = optarg;
			break;
		case 'i':
			ipaddr = optarg;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/*
	 * Uppercase event tags for now. Should be wired in list instead.
	 */
	bp = argv[0];
	while (*bp) {
		*bp = toupper(*bp);
		bp++;
	}

	/*
	 * Get our IP address. Thats how we name ourselves to the
	 * Testbed Event System. 
	 */
	if (ipaddr == NULL) {
	    struct hostent	*he;
	    struct in_addr	myip;
	    
	    if (gethostname(buf, sizeof(buf)) < 0) {
		ERROR("could not get hostname\n");
		return 1;
	    }

	    if (! (he = gethostbyname(buf))) {
		ERROR("could not get IP address from hostname\n");
		return 1;
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
	 * Construct an address tuple for generating the event.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		ERROR("could not allocate an address tuple\n");
		return 1;
	}
	tuple->objtype  = OBJECTTYPE_TESTBED;
	tuple->objname  = argv[0];
	tuple->host	= ipaddr;

	/* Register with the event system: */
	handle = event_register(server, 0);
	if (handle == NULL) {
		ERROR("could not register with event system\n");
		return 1;
	}

	/* Generate the event */
	notification = event_notification_alloc(handle, tuple);
	
	if (notification == NULL) {
		ERROR("could not allocate notification\n");
		return 1;
	}

	if (event_notify(handle, notification) == 0) {
		ERROR("could not send test event notification\n");
		return 1;
	}

	event_notification_free(handle, notification);

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		ERROR("could not unregister with event system\n");
		return 1;
	}
	
	return 0;
}
