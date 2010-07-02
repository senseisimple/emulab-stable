/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003, 2004, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * TODO: Check for root start. Switch to user nobody.
 *       Require remote client to supply key to confirm its a pid/eid member.
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
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "event.h"
#include "tbdefs.h"
#include "log.h"

#define SOCKBUFSIZE	(128 * 1024)

static int		debug = 1;
static event_handle_t	bosshandle;
static int		clientsock;

/*
 * This record for each client that checks in
 */
typedef struct client_record {
	struct sockaddr_in	address;
	struct client_record	*next;
} client_record_t;
client_record_t		*client_records;

/*
 * This record is for each pending event that needs to be sent.
 */
typedef struct event_record {
	struct in_addr		host;
	struct event_record	*next;
	int			dlen;
	unsigned char		bindata[0];
} event_record_t;
event_record_t		*events_tosend;
event_record_t		*events_qend;	/* Last event, for queuing */

/* Locks for pending events queue */
static pthread_mutex_t event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t event_queue_cond   = PTHREAD_COND_INITIALIZER;

/* protos */
static int		RemoteSocket(int *sockp, int *ourport);
static int		ServerLoop(void);
static int		QueueEvent(unsigned char *data, int len, char *host);
static event_record_t *	DeQueueEvent(int waittime);
static void		FreeEvent(event_record_t *);
static void		SendEvent(event_record_t *record);

void
usage(char *progname)
{
    fprintf(stderr, "Usage: %s [-s server] -e pid/eid\n", progname);
    exit(-1);
}

static void
callback(event_handle_t handle,
	 event_notification_t notification, void *data);

int
main(int argc, char **argv)
{
	address_tuple_t		tuple;
	char			*progname;
	char			*server = NULL;
	char			*port = NULL;
	char			*myeid = NULL;
	char			pid[TBDB_FLEN_PID], eid[TBDB_FLEN_EID];
	char			buf[BUFSIZ], *bp;
	int			c, ourport;
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
			port = optarg;
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
		usage(progname);

	if ((bp = strchr(myeid, '/')) == NULL)
		usage(progname);
	
	*bp++ = (char) NULL;
	strcpy(pid, buf);
	strcpy(eid, bp);

	if (debug)
		loginit(0, 0);
	else {
		sprintf(buf, "/proj/%s/exp/%s/logs/evproxy.log", pid, eid);
		loginit(0, buf);
	}

	/*
	 * If server is not specified, then it defaults to localhost.
	 * This allows the client to work on either users.emulab.net
	 * or on a client node. 
	 */
	if (!server)
		server = "localhost";

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
	 * Construct an address tuple for subscribing to events.
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

	/*
	 * Create a subscription to pass to the remote server. We want
	 * all events for this experiment.
	 */
#if 1
	tuple->objtype  = "TBEXAMPLE";
#else	
	tuple->host = ADDRESSTUPLE_ALL;
	tuple->expt = myeid;
#endif

	/* Subscribe to the test event: */
	if (! event_subscribe(bosshandle, callback, tuple, "event received")) {
		fatal("could not subscribe to events on remote server");
	}

	/*
	 * Stash the pid away.
	 */
	sprintf(buf, "%s/evproxy-%s,%s.pid", _PATH_VARRUN, pid, eid);
	fp = fopen(buf, "w");
	if (fp != NULL) {
		fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	RemoteSocket(&clientsock, &ourport);
	
	/*
	 * Do this now, once we have had a chance to fail on the above
	 * event system calls.
	 */
	if (!debug)
		daemon(0, 0);

	ServerLoop();

	/* Unregister with the remote event system: */
	if (event_unregister(bosshandle) == 0) {
		fatal("could not unregister with remote event system");
	}
	return 0;
}

/*
 * Handle incoming events from the remote server. 
 */
static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		hostname[128];
	char		bindata[2*BUFSIZ];
	int		len;

	event_notification_get_objname(handle,
			       notification, hostname, sizeof(hostname));

	if (debug) {
		char		eventtype[TBDB_FLEN_EVEVENTTYPE];
		char		objecttype[TBDB_FLEN_EVOBJTYPE];
		char		objectname[TBDB_FLEN_EVOBJNAME];
		
		event_notification_get_eventtype(handle,
				notification, eventtype, sizeof(eventtype));
		event_notification_get_objtype(handle,
				notification, objecttype, sizeof(objecttype));
		event_notification_get_objname(handle,
				notification, objectname, sizeof(objectname));

		info("%s %s %s %s\n", hostname,
		     eventtype, objecttype, objectname);
	}

	len = sizeof(bindata);
	if (event_notification_pack(handle, notification, bindata, &len) != 0){
		error("Failed to pack notification!\n");
		return;
	}
	QueueEvent(bindata, len, hostname);
}

/*
 * Create the local socket to talk to the client side. This is a UDP socket.
 * The local port is dynamic.
 */
static int
RemoteSocket(int *sockp, int *ourport)
{
	int			sock, length, i;
	struct sockaddr_in	name;

	/* Create socket from which to read. */
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		pfatal("opening stream socket");
	}

	i = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		       (char *)&i, sizeof(i)) < 0)
		pwarning("setsockopt(SO_REUSEADDR)");

	i = SOCKBUFSIZE;
	if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase send socket buffer size to %d",
			 SOCKBUFSIZE);
    
	i = SOCKBUFSIZE;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
		pwarning("Could not increase recv socket buffer size to %d",
			 SOCKBUFSIZE);

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

#if 0
	/*
	 * Make the socket non-blocking. This does not work with -pthreads.
	 */
	timeout.tv_sec  = 0;
	timeout.tv_usec = 100000;
	
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
		       &timeout, sizeof(timeout)) < 0)
		pfatal("setsockopt(SOL_SOCKET, SO_RCVTIMEO)");
#else
	if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0)
		pfatal("fcntl(sock, F_SETFL, O_NONBLOCK) failed");
#endif

	*ourport = (int) ntohs(name.sin_port);
	*sockp = sock;
	return 0;
}

/*
 * Wait for heartbeat messages from clients, and for events from
 * the server to appear in the queue. We want to wake up regularly
 * to check for the hearbeat messages, but otherwise we sit and wait
 * for the event queue to go non-null with a pthread_cond_wait.
 */
static int
ServerLoop(void)
{
	char		buf[BUFSIZ];
	event_record_t  *erecord;
	
	while (1) {
		/*
		 * Wait for clients to check in.
		 */
		while (1) {
			struct sockaddr_in	client;
			int			len, cc;
			client_record_t		*crecord;

			len = sizeof(client);
			cc = recvfrom(clientsock, buf, sizeof(buf), 0,
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

			/*
			 * See if this client checked in already. If not skip.
			 * Eventually use this as heartbeat to age out dead
			 * clients.
			 */
			crecord = client_records;
			while (crecord) {
				if (crecord->address.sin_addr.s_addr ==
				    client.sin_addr.s_addr) {
					if (debug) {
						info("Heartbeat from %s\n",
						  inet_ntoa(client.sin_addr));
					}
					crecord->address = client;
					goto next;
				}
				crecord++;
			}
			/*
			 * Must be new client.
			 */
			crecord = (client_record_t *)
				malloc(sizeof(client_record_t));
			if (crecord == NULL) {
				fatal("Out of memory!\n");
			}
			crecord->address = client;
			crecord->next    = client_records;
			client_records   = crecord;
			info("Connected: %s\n", inet_ntoa(client.sin_addr));
		next:
			;
		}

		/*
		 * Okay, try to dequeue events. If there are none, we will
		 * block for a short time before trying to run the loop again.
		 */
		while ((erecord = DeQueueEvent(5)) != NULL) {
			info("Got event %p\n", erecord);

			/*
			 * Send the data to all of the appropriate clients
			 * and then free it. 
			 */
			SendEvent(erecord);
			FreeEvent(erecord);
		}
		info("No events, looping\n");
	}
	return 0;
}

/*
 * Add an event to the Queue. We dequeue off the front, and add new events
 * to the end using a last pointer.
 */
static int
QueueEvent(unsigned char *data, int len, char *host)
{
	event_record_t	*record;

	/* Use lock to protect free/malloc calls too. */
	pthread_mutex_lock(&event_queue_mutex);
	if ((record = malloc(sizeof(*record) + len)) == NULL) {
		error("Out of memory for new event\n");
		return -1;
	}
	record->next = 0;
	record->dlen = len;
	memcpy(record->bindata, data, len);
	if (!strcmp(ADDRESSTUPLE_ALL, host))
		record->host.s_addr = 0;
	else
		inet_aton(host, &record->host);

	if (events_qend) {
		/* Maintain pointer to last event for quick insertion */
		events_qend->next = record;
		events_qend       = record;
	}
	else {
		events_tosend = events_qend = record;
	}

	pthread_mutex_unlock(&event_queue_mutex);

	/* Signal that there is a new event */
	pthread_cond_signal(&event_queue_cond);
	
	return 0;
}

/*
 * Dequeue first event to send. If no waittime parameter given, return
 * immediately if nothing to dequeue. Otherwise the wait the amount of
 * time specified; there is no full blocking wait needed.
 *
 * waittime is in seconds. 
 */
static event_record_t *
DeQueueEvent(int waittime)
{
	event_record_t		*record;
	struct timeval		now;
	struct timespec		wakeme;
	int			err;

	pthread_mutex_lock(&event_queue_mutex);

	if (!events_tosend && !waittime) {
		goto nada;
	}
	
	gettimeofday(&now, NULL);
	now.tv_sec += waittime;
	TIMEVAL_TO_TIMESPEC(&now, &wakeme);

	if ((err = pthread_cond_timedwait(&event_queue_cond,
					  &event_queue_mutex, &wakeme)) != 0) {
		if (err != ETIMEDOUT) {
			error("pthread_cond_timedwait failed: %d", err);
			goto nada;
		}
	}
	if (! events_tosend)
		goto nada;
	
	record = events_tosend;
	events_tosend = record->next;
	if (!events_tosend)
		events_qend = NULL;
	pthread_mutex_unlock(&event_queue_mutex);
	return record;
 nada:
	pthread_mutex_unlock(&event_queue_mutex);
	return (event_record_t *) NULL;
}

/*
 * Send an event to all of the clients that want it. Actually, it is
 * a specific client or all of them!
 */
static void
SendEvent(event_record_t *erecord)
{
	client_record_t		*crecord;

	crecord = client_records;
	while (crecord) {
		int	cc;

		/*
		 * If the event is to be sent to a specific host, then
		 * it must match on the address.
		 */
		if (erecord->host.s_addr &&
		    erecord->host.s_addr != crecord->address.sin_addr.s_addr)
			goto bad;
		
		cc = sendto(clientsock, erecord->bindata, erecord->dlen, 0,
			    (struct sockaddr *)&crecord->address,
			    sizeof(crecord->address));

		if (debug)
			info("Sending event to %d\n",
			     inet_ntoa(crecord->address.sin_addr));

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
	bad:
		crecord = crecord->next;
	}
}

/*
 * Free an event.
 */
static void
FreeEvent(event_record_t *record)
{
	pthread_mutex_lock(&event_queue_mutex);
	free(record);
	pthread_mutex_unlock(&event_queue_mutex);
}

