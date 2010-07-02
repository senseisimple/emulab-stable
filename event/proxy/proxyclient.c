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
#include <time.h>
#include <math.h>
#include <paths.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "event.h"
#include "tbdefs.h"
#include "log.h"

static int		debug = 1;
static event_handle_t	localhandle;

/* protos */
static int	RemoteSocket(int *sockp, int *ourport);
static int	ServerConnect(int, int, struct in_addr, int);
static int	GetEvents(event_handle_t handle, int sock);

void
usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-s server] -e pid/eid\n", progname);
    exit(-1);
}

int
main(int argc, char **argv)
{
	char			*progname;
	char			*server = NULL;
	int			serverport = 9843;
	char			*myeid = NULL;
	char			buf[BUFSIZ];
	char			hostname[MAXHOSTNAMELEN];
	struct hostent		*he;
	int			sock, c, ourport;
	struct in_addr		myip, serverip;
	FILE			*fp;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "ds:p:e:")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		case 's':
			server = optarg;
			break;
		case 'p':
			serverport = atoi(optarg);
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

	if (argc)
		usage(progname);

	if (! myeid)
		fatal("Must provide pid/eid");

	if (debug)
		loginit(0, 0);
	else {
		loginit(1, "proxyclient");
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

	/*
	 * If server is not specified, then it defaults to EVENTSERVER.
	 * This allows the client to work on either users.emulab.net
	 * or on a client node. 
	 */
	if (!server)
		server = EVENTSERVER;

	if (! (he = gethostbyname(server))) {
		fatal("could not get IP address for server: %s", server);
	}
	memcpy((char *)&serverip, he->h_addr, he->h_length);

	/* Register with the event system on the local node */
	localhandle = event_register("elvin://localhost", 0);
	if (localhandle == NULL) {
		fatal("could not register with local event system");
	}
	
	/*
	 * Stash the pid away.
	 */
	sprintf(buf, "%s/evproxy.pid", _PATH_VARRUN);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	RemoteSocket(&sock, &ourport);
	ServerConnect(sock, ourport, serverip, serverport);

	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug)
		daemon(0, 0);

	while (1) {
		ServerConnect(sock, ourport, serverip, serverport);
		GetEvents(localhandle, sock);
	}

	/* Unregister with the local event system: */
	if (event_unregister(localhandle) == 0) {
		fatal("could not unregister with local event system");
	}

	return 0;
}


/*
 * Create the remote socket to talk to the server side. This is a UDP socket.
 * The local port is dynamic.
 */
static int
RemoteSocket(int *sockp, int *ourport)
{
	int			sock, length, i;
	struct sockaddr_in	name;
	struct timeval		timeout;

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		pfatal("opening stream socket");
	}

	i = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");
	
	/* Create name. */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = 0;
	if (bind(sock, (struct sockaddr *) &name, sizeof(name))) {
		pfatal("binding dgram socket");
	}

	/* Find assigned port value and print it out. */
	length = sizeof(name);
	if (getsockname(sock, (struct sockaddr *) &name, &length)) {
		pfatal("getsockname");
	}
	info("listening on UDP port %d\n", ntohs(name.sin_port));

	/*
	 * We use a socket level timeout instead of polling for data.
	 */
	timeout.tv_sec  = 10;
	timeout.tv_usec = 0;
	
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
		       &timeout, sizeof(timeout)) < 0)
		pfatal("setsockopt(SOL_SOCKET, SO_RCVTIMEO)");

	*ourport = (int) ntohs(name.sin_port);
	*sockp = sock;
	return 0;
}

/*
 * Tell the server about us. Since the server might crash or whatever,
 * we do this every minute as a heartbeat. There is no need to wait
 * for a reply; if the packet does not get through then we probably do not
 * need to worry about getting events from the server since those packets
 * are also UDP.
 */
static int
ServerConnect(int sock, int ourport, struct in_addr serverip, int serverport)
{
	struct sockaddr_in	request;
	char			buf[BUFSIZ];
	int			cc;
	
	/* Server name. */
	request.sin_family = AF_INET;
	request.sin_addr   = serverip;
	request.sin_port   = htons(serverport);
	strcpy(buf, "Hi!");

	cc = sendto(sock, buf, strlen(buf) + 1, 0,
		    (struct sockaddr *)&request, sizeof(request));

	if (cc <= 0) {
		if (cc < 0) {
			if (errno != EWOULDBLOCK)
				pfatal("Writing to server socket");
				
			errorc("Writing to server socket");
			goto bad;
		}
		error("short write to server\n");
		goto bad;
	}
	return 0;
 bad:
	return -1;
}

static int
GetEvents(event_handle_t handle, int sock)
{
	unsigned char	buf[2*BUFSIZ];

	while (1) {
		struct sockaddr_in	client;
		int			len, cc;
		event_notification_t	newnote;

		len = sizeof(client);
		cc = recvfrom(sock, buf, sizeof(buf), 0,
			      (struct sockaddr *)&client, &len);

		if (cc <= 0) {
			if (cc < 0) {
				if (errno != EWOULDBLOCK)
					pfatal("Reading from socket");
				break;
			}
			error("short read from server\n");
			break;
		}
		event_notification_unpack(handle, &newnote, buf, cc);
		event_notify(handle, newnote);
		event_notification_free(handle, newnote);
	}
	return 0;
}
