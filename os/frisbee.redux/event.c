/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Testbed event system interface
 * Allow starting multiple clients in a synchronized way.
 */

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h> 

#include "lib/libtb/tbdefs.h"
#include "event/lib/event.h"

#include "decls.h"
#include "log.h"
#include "event.h"

//#define EVENTDEBUG

static Event_t		lastevent;
static event_handle_t	ehandle;

static int		gotevent;
static int		clientnum;

/*
 * type==START
 *	STAGGER=N PKTTIMEOUT=N IDLETIMER=N READAHEAD=N INPROGRESS=N REDODELAY=N
 * type==STOP
 *	STAGGER=N
 */
static int
parse_event(Event_t *event, char *etype, char *buf)
{
	char *cp;
	int val;

	memset(event, -1, sizeof *event);

	if (strcmp(etype, TBDB_EVENTTYPE_START) == 0)
		event->type = EV_START;
	else if (strcmp(etype, TBDB_EVENTTYPE_STOP) == 0)
		event->type = EV_STOP;
	else
		return 1;

	cp = buf;
	if (cp != NULL) {
		while ((cp = strsep(&buf, " ")) != NULL) {
			if (sscanf(cp, "STARTDELAY=%d", &val) == 1) {
				event->data.start.startdelay = val;
				continue; 
			}
			if (sscanf(cp, "PKTTIMEOUT=%d", &val) == 1) {
				event->data.start.pkttimeout = val;
				continue; 
			}
			if (sscanf(cp, "IDLETIMER=%d", &val) == 1) {
				event->data.start.idletimer = val;
				continue; 
			}
			if (sscanf(cp, "CHUNKBUFS=%d", &val) == 1) {
				event->data.start.chunkbufs = val;
				continue; 
			}
			if (sscanf(cp, "READAHEAD=%d", &val) == 1) {
				event->data.start.readahead = val;
				continue; 
			}
			if (sscanf(cp, "INPROGRESS=%d", &val) == 1) {
				event->data.start.inprogress = val;
				continue; 
			}
			if (sscanf(cp, "REDODELAY=%d", &val) == 1) {
				event->data.start.redodelay = val;
				continue; 
			}
			if (sscanf(cp, "IDLEDELAY=%d", &val) == 1) {
				event->data.start.idledelay = val;
				continue; 
			}
			if (sscanf(cp, "SLICE=%d", &val) == 1) {
				event->data.start.slice = val;
				continue; 
			}
			if (sscanf(cp, "ZEROFILL=%d", &val) == 1) {
				event->data.start.zerofill = val;
				continue; 
			}
			if (sscanf(cp, "RANDOMIZE=%d", &val) == 1) {
				event->data.start.randomize = val;
				continue; 
			}
			if (sscanf(cp, "NOTHREADS=%d", &val) == 1) {
				event->data.start.nothreads = val;
				continue; 
			}
			if (sscanf(cp, "DEBUG=%d", &val) == 1) {
				event->data.start.debug = val;
				continue; 
			}
			if (sscanf(cp, "TRACE=%d", &val) == 1) {
				event->data.start.trace = val;
				continue; 
			}
			if (sscanf(cp, "EXITSTATUS=%d", &val) == 1) {
				event->data.stop.exitstatus = val;
				continue; 
			}

			/*
			 * Hideous Hack Alert!
			 *
			 * Assume our hostname is of the form 'c-<num>.<domain>'
			 * and use <num> to determine our client number.
			 * We use that number and compare to the maxclients
			 * field to determine if we should process this event.
			 * If not, we ignore this event.
			 */
			if (sscanf(cp, "MAXCLIENTS=%d", &val) == 1) {
				if (clientnum >= val) {
					gotevent = 0;
					return 0;
				}
				continue;
			}
		}
	}
	gotevent = 1;
	return 0;
}

static void
callback(event_handle_t handle, event_notification_t notification, void *data)
{
	char		buf[7][64];
	char		args[256];
	int		len = 64;

	buf[0][0] = buf[1][0] = buf[2][0] = buf[3][0] = 0;
	buf[4][0] = buf[5][0] = buf[6][0] = 0;
	event_notification_get_site(handle, notification, buf[0], len);
	event_notification_get_expt(handle, notification, buf[1], len);
	event_notification_get_group(handle, notification, buf[2], len);
	event_notification_get_host(handle, notification, buf[3], len);
	event_notification_get_objtype(handle, notification, buf[4], len);
	event_notification_get_objname(handle, notification, buf[5], len);
	event_notification_get_eventtype(handle, notification, buf[6], len);
	event_notification_get_arguments(handle, notification,
					 args, sizeof(args));

#ifdef EVENTDEBUG
	{
		struct timeval now;
		static int ecount;

		gettimeofday(&now, NULL);
		fprintf(stderr, "Event %d: %lu.%03lu %s %s %s %s %s %s %s %s\n",
			++ecount, now.tv_sec, now.tv_usec / 1000,
			buf[0], buf[1], buf[2],
			buf[3], buf[4], buf[5], buf[6], args);
	}
#endif
	if (parse_event(&lastevent, buf[6], args))
		log("bogus event '%s %s' ignored", buf[6], args);
}

int
EventInit(char *server)
{
	char buf[BUFSIZ], ipbuf[BUFSIZ];
	struct hostent *he;
	struct in_addr myip;
	char *ipaddr;
	address_tuple_t	tuple;
	    
	if (server == NULL) {
		warning("no event server specified");
		return 1;
	}

	if (gethostname(buf, sizeof(buf)) < 0) {
		pwarning("could not get hostname");
		return 1;
	}

	if ((he = gethostbyname(buf)) == NULL) {
		warning("could not get IP address from hostname");
		return 1;
	}

	memcpy((char *)&myip, he->h_addr, he->h_length);
	strcpy(ipbuf, inet_ntoa(myip));
	ipaddr = ipbuf;

#if 1
	/*
	 * Hideous Hack Alert!
	 *
	 * Assume our hostname is of the form 'c-<num>.<domain>'
	 * and use <num> to determine our client number.
	 * We use that number later to compare to the MAXCLIENTS
	 * field to determine if we should process an event.
	 */
	if (sscanf(buf, "c-%d.", &clientnum) != 1) {
		warning("could not determine client number from hostname %s",
			buf);
		return 1;
	} else if (debug)
		log("client number %d for event handling", clientnum);
#endif

	/*
	 * Convert server/port to elvin thing.
	 */
	snprintf(buf, sizeof(buf), "elvin://%s", server);
	server = buf;

	/*
	 * Construct an address tuple for subscribing to events for
	 * this node.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		warning("could not allocate an address tuple");
		return 1;
	}
	tuple->host	 = ADDRESSTUPLE_ANY; /* ipaddr; */
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = ADDRESSTUPLE_ANY;
	tuple->objtype   = TBDB_OBJECTTYPE_FRISBEE;
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->eventtype = ADDRESSTUPLE_ANY;

	/*
	 * Register with the event system. 
	 */
	ehandle = event_register(server, 0);
	if (ehandle == NULL) {
		warning("could not register with event system");
		address_tuple_free(tuple);
		return 1;
	}
	
	/*
	 * Subscribe to the event we specified above.
	 */
	if (!event_subscribe(ehandle, callback, tuple, NULL)) {
		warning("could not subscribe to FRISBEE events");
		address_tuple_free(tuple);
		return 1;
	}
	address_tuple_free(tuple);
	return 0;
}

int
EventCheck(Event_t *event)
{
	int rv;

	gotevent = 0;
	rv = event_poll(ehandle);
	if (rv)
		fatal("event_poll failed, err=%d\n", rv);
	if (gotevent)
		memcpy(event, &lastevent, sizeof lastevent);
	return gotevent;
}

void
EventWait(int eventtype, Event_t *event)
{
	while (EventCheck(event) == 0 ||
	       (eventtype != EV_ANY && event->type != eventtype))
		/* sleep? */;
}
