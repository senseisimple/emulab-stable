/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * event-sched.c --
 *
 *      Testbed event scheduler.
 *
 *      The event scheduler is an event system client; it operates by
 *      subscribing to the EVENT_SCHEDULE event, enqueuing the event
 *      notifications it receives, and resending the notifications at
 *      the indicated times.
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include "event-sched.h"
#include "log.h"
#include "tbdb.h"
#include "config.h"

static void enqueue(event_handle_t handle,
		    event_notification_t notification, void *data);
static void dequeue(event_handle_t handle);

static char	*progname;
static char	*pid, *eid;
static int	get_static_events(event_handle_t handle);
static int	debug;
static void	cleanup(void);
static void	quit(int);

#define MAXAGENTS	200
static struct {
	char    nodeid[TBDB_FLEN_NODEID];
	char    vnode[TBDB_FLEN_VNAME];
	char	objname[TBDB_FLEN_EVOBJNAME];
	char	objtype[TBDB_FLEN_EVOBJTYPE];
	char	ipaddr[32];
} agents[MAXAGENTS];
static int	numagents;

void
usage()
{
	fprintf(stderr,	"Usage: %s [-s server] [-p port] <pid> <eid>\n",
		progname);
	exit(-1);
}

int
main(int argc, char **argv)
{
	address_tuple_t tuple;
	event_handle_t handle;
	char *server = NULL;
	char *port = NULL;
	char *log = NULL;
	char pideid[BUFSIZ], buf[BUFSIZ];
	int c, count;

	progname = argv[0];

	/* Initialize event queue semaphores: */
	sched_event_init();

	while ((c = getopt(argc, argv, "s:p:dl:")) != -1) {
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
		case 'l':
			log = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s [-s SERVER]\n", argv[0]);
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();
	pid = argv[0];
	eid = argv[1];
	sprintf(pideid, "%s/%s", pid, eid);

	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	signal(SIGHUP, quit);

	if (debug)
		loginit(0, log);
	else
		loginit(1, "event-sched");

	/*
	 * Set up DB state.
	 */
	if (!dbinit())
		return 1;

	/*
	 * Set our pid in the DB. This will fail if there is already
	 * a non-zero value in the DB (although its okay to set it to
	 * zero no matter what). 
	 */
	if (! mydb_seteventschedulerpid(pid, eid, getpid()))
		fatal("Could not update DB with process id!");
	atexit(cleanup);

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

	/* Register with the event system: */
	handle = event_register(server, 1);
	if (handle == NULL) {
		fatal("could not register with event system");
	}

	/*
	 * Construct an address tuple for event subscription. We set the 
	 * scheduler flag to indicate we want to capture those notifications.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		fatal("could not allocate an address tuple");
	}
	tuple->scheduler = 1;
	tuple->expt      = pideid;

	if (event_subscribe(handle, enqueue, tuple, NULL) == NULL) {
		fatal("could not subscribe to EVENT_SCHEDULE event");
	}

	/*
	 * Hacky. Need to wait until all nodes in the experiment are
	 * in the ISUP state before we can start the event list rolling.
	 */
	count = 0;
	c     = 1;
	while (c) {
		if (! mydb_checkexptnodeeventstate(pid, eid,
						   TBDB_EVENTTYPE_ISUP, &c)) {
			fatal("Could not get node event state");
		}
		count++;
		if ((count % 10) == 0)
			info("Waiting for nodes in %s/%s to come up ...\n",
			     pid, eid);

		/*
		 * Don't want to pound the DB too much.
		 */
		sleep(3);
	}

	/*
	 * Read the static events list and schedule.
	 */
	if (!get_static_events(handle)) {
		fatal("could not get static event list");
	}

	/* Dequeue events and process them at the appropriate times: */
	dequeue(handle);

	/* Unregister with the event system: */
	if (event_unregister(handle) == 0) {
		fatal("could not unregister with event system");
	}

	info("Scheduler for %s/%s exiting\n", pid, eid);
	return 0;
}

/* Enqueue event notifications as they arrive. */
static void
enqueue(event_handle_t handle, event_notification_t notification, void *data)
{
    sched_event_t	event;
    char		objname[TBDB_FLEN_EVOBJNAME];
    int			x;

    /* Clone the event notification, since we want the notification to
       live beyond the callback function: */
    event.notification = elvin_notification_clone(notification,
                                                  handle->status);
    if (!event.notification) {
	    error("elvin_notification_clone failed!\n");
	    return;
    }

    /* Clear the scheduler flag */
    if (! event_notification_remove(handle, event.notification, "SCHEDULER") ||
	! event_notification_put_int32(handle,
				       event.notification, "SCHEDULER", 0)) {
	    error("could not clear scheduler attribute of notification %p\n",
		  event.notification);
	    goto bad;
    }

    /* Get the event's firing time: */
    if (! event_notification_get_int32(handle, event.notification, "time_usec",
				       (int *) &event.time.tv_usec) ||
	! event_notification_get_int32(handle, event.notification, "time_sec",
				       (int *) &event.time.tv_sec)) {
	    error("could not get time from notification %p\n",
		  event.notification);
	    goto bad;
    }

    /*
     * Must map the event to the proper agent running on a particular
     * node. 
     */
    if (! event_notification_get_objname(handle, event.notification,
					 objname, sizeof(objname))) {
	    error("could not get object name from notification %p\n",
		  event.notification);
	    goto bad;
    }
    for (x = 0; x < numagents; x++) {
	    if (!strcmp(agents[x].objname, objname))
		    break;
    }
    if (x == numagents) {
	    error("Could not map object to an agent: %s\n", objname);
	    goto bad;
    }
    event_notification_clear_host(handle, event.notification);
    event_notification_set_host(handle,
				event.notification, agents[x].ipaddr);
    event_notification_clear_objtype(handle, event.notification);
    event_notification_set_objtype(handle,
				   event.notification, agents[x].objtype);

    if (debug > 1) {
	    struct timeval now;
	    
	    gettimeofday(&now, NULL);
	    
	    info("Sched: note:%p at:%ld:%d now:%ld:%d agent:%d\n",
                 event.notification,
		 event.time.tv_sec, event.time.tv_usec,
		 now.tv_sec, now.tv_usec,
		 x);
    }

    /* Enqueue the event notification for resending at the indicated
       time: */
    sched_event_enqueue(event);
    return;
 bad:
    event_notification_free(handle, event.notification);
}

/* Dequeue events from the event queue and fire them at the
   appropriate time.  Runs in a separate thread. */
static void
dequeue(event_handle_t handle)
{
    sched_event_t next_event;
    struct timeval now;

    while (1) {
	if (sched_event_dequeue(&next_event, 1) < 0)
	    break;

        /* Fire event. */
	if (debug > 1)
	    gettimeofday(&now, NULL);

        if (event_notify(handle, next_event.notification) == 0) {
            ERROR("could not fire event\n");
            return;
        }

	if (debug > 1) {
	    info("Fire:  note:%p at:%ld:%d now:%ld:%d\n",
                 next_event.notification,
		 next_event.time.tv_sec, next_event.time.tv_usec,
		 now.tv_sec,
		 now.tv_usec);
	}
        event_notification_free(handle, next_event.notification);
    }
}

/*
 * Get the static event list from the DB and schedule according to
 * the relative time stamps.
 */
static int
get_static_events(event_handle_t handle)
{
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;
	struct timeval	now, time;
	address_tuple_t tuple;
	char		pideid[BUFSIZ];
	event_notification_t notification;
	int		adx = 0;
	sched_event_t	event;

	/*
	 * Build up a table of agents that can receive dynamic events.
	 * These are stored in the virt_trafgens table, which we join
	 * with the reserved table to get the physical node name where
	 * the agent is running.
	 *
	 * That is, we want to be able to quickly map from "cbr0" to
	 * the node on which it lives (for dynamic events).
	 */
	res = mydb_query("select vi.vname,vi.vnode,r.node_id,o.type "
			 " from virt_agents as vi "
			 "left join reserved as r on "
			 " r.vname=vi.vnode and r.pid=vi.pid and r.eid=vi.eid "
			 "left join event_objecttypes as o on "
			 " o.idx=vi.objecttype "
			 "where vi.pid='%s' and vi.eid='%s'",
			 4, pid, eid);

	if (!res) {
		error("getting virt_agents list for %s/%s", pid, eid);
		return 0;
	}
	nrows = mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (!row[0] || !row[1] || !row[2] || !row[3])
			continue;

		strcpy(agents[numagents].objname, row[0]);
		strcpy(agents[numagents].vnode,   row[1]);
		strcpy(agents[numagents].nodeid,  row[2]);
		strcpy(agents[numagents].objtype, row[3]);

		if (! mydb_nodeidtoip(row[2], agents[numagents].ipaddr))
			continue;
		numagents++;
		if (numagents >= MAXAGENTS) {
			fatal("Too many agents!");
		}
	}
	mysql_free_result(res);
	
	if (debug) {
		for (adx = 0; adx < numagents; adx++) {
			info("Agent %d: %10s %10s %10s %8s %16s\n", adx,
			     agents[adx].objname,
			     agents[adx].objtype,
			     agents[adx].vnode,
			     agents[adx].nodeid,
			     agents[adx].ipaddr);
		}
	}

	/*
	 * Now get the eventlist. There should be entries in the
	 * agents table for anything we find in the list.
	 */
	res = mydb_query("select ex.idx,ex.time,ex.vnode,ex.vname,"
			 " ex.arguments,ot.type,et.type from eventlist as ex "
			 "left join event_eventtypes as et on "
			 " ex.eventtype=et.idx "
			 "left join event_objecttypes as ot on "
			 " ex.objecttype=ot.idx "
			 "where ex.pid='%s' and ex.eid='%s' "
			 "order by ex.time ASC",
			 7, pid, eid);
#define EXIDX	 row[0]
#define EXTIME	 row[1]
#define EXVNODE	 row[2]
#define OBJNAME  row[3]
#define EXARGS	 row[4]
#define OBJTYPE	 row[5]
#define EVTTYPE	 row[6]

	if (!res) {
		error("getting static event list for %s/%s", pid, eid);
		return 0;
	}

	if ((nrows = (int)mysql_num_rows(res)) == 0) {
		mysql_free_result(res);
		return 1;
	}

	/*
	 * Construct an address tuple for the notifications. We can reuse
	 * the same one over and over since the data is copied out.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("could not allocate an address tuple");
		return 0;
	}

	sprintf(pideid, "%s/%s", pid, eid);
	gettimeofday(&now, NULL);

	info("Getting event stream at: %lu:%d\n",  now.tv_sec, now.tv_usec);

	/*
	 * Pad the start time out a bit to give this code a chance to run.
	 */
	now.tv_sec += 30;
	
	while (nrows) {
		double		firetime;

		row = mysql_fetch_row(res);
		firetime = atof(EXTIME);

		for (adx = 0; adx < numagents; adx++) {
			if (!strcmp(agents[adx].objname, OBJNAME))
				break;
		}
		if (adx == numagents) {
			error("Could not map event index %s", EXIDX);
			return 0;
		}

		tuple->expt      = pideid;
		tuple->host      = agents[adx].ipaddr;
		tuple->objname   = OBJNAME;
		tuple->objtype   = OBJTYPE;
		tuple->eventtype = EVTTYPE;

		if (debug) 
			info("%8s %10s %10s %10s %10s %10s %10s\n",
			     EXTIME, EXVNODE, OBJNAME, OBJTYPE,
			     EVTTYPE, agents[adx].ipaddr, 
			     EXARGS ? EXARGS : "");

		event.notification = event_notification_alloc(handle, tuple);
		if (! event.notification) {
			error("could not allocate notification");
			mysql_free_result(res);
			return 0;
		}
		event_notification_set_arguments(handle,
						 event.notification, EXARGS);

		time.tv_sec  = now.tv_sec  + (int)firetime;
		time.tv_usec = now.tv_usec +
			(int)((firetime - (floor(firetime))) * 1000000);

		if (time.tv_usec >= 1000000) {
			time.tv_sec  += 1;
			time.tv_usec -= 1000000;
		}
		event.time.tv_sec  = time.tv_sec;
		event.time.tv_usec = time.tv_usec;
		sched_event_enqueue(event);
		nrows--;
	}
	mysql_free_result(res);

	/*
	 * Generate a TIME starts message.
	 */
	tuple->expt      = pideid;
	tuple->host      = ADDRESSTUPLE_ALL;
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->objtype   = TBDB_OBJECTTYPE_TIME;
	tuple->eventtype = TBDB_EVENTTYPE_START;
	
	notification = event_notification_alloc(handle, tuple);
	if (! notification) {
		error("could not allocate notification");
		return 0;
	}
	event.notification = notification;
	event.time.tv_sec  = now.tv_sec;
	event.time.tv_usec = now.tv_usec;
	sched_event_enqueue(event);

	info("TIME STARTS will be sent at: %lu:%d\n", now.tv_sec, now.tv_usec);

	gettimeofday(&now, NULL);
	info("The time is now: %lu:%d\n", now.tv_sec, now.tv_usec);

	return 1;
}
	
static void
cleanup(void)
{
	if (pid) 
		mydb_seteventschedulerpid(pid, eid, 0);
	pid = NULL;
	eid = NULL;
}

static void
quit(int sig)
{
	info("Got sig %d, exiting\n", sig);

	/* cleanup() will be called from atexit() */
	exit(0);
}
