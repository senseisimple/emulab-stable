/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2005, 2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

/**
 * @file canarydEvents.c
 *
 * Implementation file for the event related stuff for canaryd.
 */

#include <ctype.h>
#include <syslog.h>
#include <string.h>

#include "auxfuncs.h"

#include "canarydEvents.h"

struct ceCanarydEventsData canaryd_events_data;

/**
 * The list of events we care about: start, stop, reset, report.
 */
static const char *CANARYD_EVENTS = (TBDB_EVENTTYPE_START ","
				     TBDB_EVENTTYPE_STOP ","
				     TBDB_EVENTTYPE_RESET ","
				     TBDB_EVENTTYPE_REPORT);

/**
 * Grab a the PID/EID from a "tmcc status" invocation.
 *
 * <p>Expected input: ALLOCATED=pid/eid ...
 *
 * @param buf The output of tmcc.
 * @param data ...
 * @return Zero on success, -1 otherwise.
 */
static int ceGrabPidEid(char *buf, void *data)
{
    char *bp, *tmpbp, *value = "";
    int retval = -1;

    if( strstr(buf, "ALLOCATED=") )
    {
	bp = tmpbp = buf+10;
	while (!isspace(*tmpbp)) tmpbp++;
	*tmpbp = '\0';
	value = strdup(bp);
	retval = 0;
    }
    canaryd_events_data.ced_PidEid = value;
    
    return retval;
}

int ceInitCanarydEvents(const char *event_server)
{
    char *stprog[] = { "tmcc", "status", NULL };
    address_tuple_t tuple = NULL;
    int retval = 1;

    /* Allocate the tuple that describes what we want from the event system, */
    if( (tuple = address_tuple_alloc()) == NULL )
    {
	retval = 0;
    }
    /* ... determine the pid/eid for this node, and */
    else if( procpipe(stprog, &ceGrabPidEid, NULL) )
    {
	syslog(LOG_ERR, "procpipe failed");
	retval = 0;
    }
    /* ... attempt the subscription. */
    else
    {
	tuple->host	 = ADDRESSTUPLE_ANY;
	tuple->site      = ADDRESSTUPLE_ANY;
	tuple->group     = ADDRESSTUPLE_ANY;
	tuple->expt      = (char *)canaryd_events_data.ced_PidEid;
	tuple->objtype   = "SLOTHD"; // XXX This needs to be updated in the DB
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->eventtype = (char *)CANARYD_EVENTS;
	/* Get a handle to the event system and */
	if( (canaryd_events_data.ced_Handle =
	     event_register_withkeyfile((char *)event_server,
					0,
					EVENTKEYFILE)) == NULL )
	{
	    syslog(LOG_ERR, "event_register_failed");
	    retval = 0;
	}
	/* ... subscribe to the canaryd events. */
	else if( !event_subscribe(canaryd_events_data.ced_Handle,
				  ceEventCallback,
				  tuple,
				  NULL) )
	{
	    syslog(LOG_ERR, "event_subscribe");
	    retval = 0;
	}
    }

    address_tuple_free(tuple);
    
    return( retval );
}
