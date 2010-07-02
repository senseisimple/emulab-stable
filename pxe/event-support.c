/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */
#ifdef EVENTSYS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "log.h" 
#include "tbdb.h"
#include "event.h"
#include "bootinfo.h"

static event_handle_t	event_handle = NULL;
static address_tuple_t  tuple = NULL;

/*
 * Connect to the event system. It's not an error to call this function if
 * already connected. Returns 1 on failure, 0 on sucess.
 */
int
bievent_init(void)
{
	if (!event_handle) {
		event_handle =
		  event_register("elvin://localhost:" BOSSEVENTPORT, 0);

		if (!event_handle) {
			error("Unable to register with event system!\n");
			return 1;
		}
		tuple = address_tuple_alloc();
		if (! tuple) {
			error("Unable to allocate event tuple!\n");
			event_unregister(event_handle);
			event_handle = NULL;
			return 1;
		}
	}
	return 0;
}

int
bievent_shutdown(void)
{
	if (event_handle) {
		if (tuple) {
			address_tuple_free(tuple);
			tuple = NULL;
		}
		event_unregister(event_handle);
		event_handle = NULL;
	}
	return 0;
}

/*
 * Send an event to the event system. Automatically connects (registers)
 * if not already done. Returns 0 on sucess, 1 on failure.
 */
int
bievent_send(struct in_addr ipaddr, void *opaque, char *event)
{
	event_notification_t	notification;
	char			nodeid[TBDB_FLEN_NODEID];
	char			ip[TBDB_FLEN_IP], *bp;

	if (bievent_init())
		return 1;

	/* Convert IP to nodeid */
	bp = inet_ntoa(ipaddr);
	strcpy(ip, bp);
	if (! mydb_iptonodeid(ip, nodeid)) {
		error("Unable to convert IP to nodeid for %s!\n", ip);
		return 1;
	}
	   
	tuple->host      = BOSSNODE;
	tuple->objtype   = "TBNODESTATE";
	tuple->objname   = nodeid;
	tuple->eventtype = event;

	notification = event_notification_alloc(event_handle,tuple);
	if (notification == NULL) {
		error("Unable to allocate notification!\n");
		return 1;
	}

	if (debug >= 2)
	    info("Sending event %s for node %s\n", event, nodeid);

	if (event_notify(event_handle, notification) == 0) {
		error("Unable to send notification!");
		event_notification_free(event_handle, notification);

		/*
		 * Let's try to disconnect from the event system, so that
		 * we'll reconnect next time around.
		 */
		bievent_shutdown();
		return 1;
	}

	event_notification_free(event_handle,notification);
	return 0;
	
}
#endif
