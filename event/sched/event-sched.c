/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2004 University of Utah and the Flux Group.
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
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include "event-sched.h"
#include "log.h"
#include "config.h"
#ifdef	RPC
#include "tbdefs.h"
#include "rpc.h"
#define main realmain
#else
#include "tbdb.h"
#endif

static void enqueue(event_handle_t handle,
		    event_notification_t notification, void *data);
static void dequeue(event_handle_t handle);
static int  handle_simevent(event_handle_t handle, sched_event_t *eventp);
static void waitforactive(char *, char *);

static char	*progname;
static char	*pid, *eid;
static int	get_static_events(event_handle_t handle);
static int	debug;

struct agent {
	char    name[TBDB_FLEN_EVOBJNAME];  /* Agent *or* group name */
	char    nodeid[TBDB_FLEN_NODEID];
	char    vnode[TBDB_FLEN_VNAME];
	char	objname[TBDB_FLEN_EVOBJNAME];
	char	objtype[TBDB_FLEN_EVOBJTYPE];
	char	ipaddr[32];
	struct  agent *next;
};
static struct agent	*agents;

void
usage()
{
	fprintf(stderr,
		"Usage: %s <options> -k keyfile <pid> <eid>\n"
		"options:\n"
		"-d         - Turn on debugging\n"
		"-s server  - Specify location of elvind server\n"
		"-p port    - Specify port number of elvind server\n"
		"-l logfile - Specify logfile to direct output\n"
		"-k keyfile - Specify keyfile name\n",
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
	char *keyfile = NULL;
	char pideid[BUFSIZ], buf[BUFSIZ];
	int c;

	progname = argv[0];

	/* Initialize event queue semaphores: */
	sched_event_init();

	while ((c = getopt(argc, argv, "s:p:dl:k:")) != -1) {
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
		case 'k':
			keyfile = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s [-s SERVER]\n", argv[0]);
			return 1;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2 || !keyfile)
		usage();

	pid = argv[0];
	eid = argv[1];
	sprintf(pideid, "%s/%s", pid, eid);

	/*
	 * Okay this is more complicated than it probably needs to be. We do
	 * not want to run anything on ops that is linked against the event
	 * system as root, even if its just to write out a pid file. This
	 * may be overly paranoid, but so be it. As a result, we do not
	 * want to use the usual daemon function, but rather depend on the
	 * caller (a perl script) to set the pid file then exec this
	 * program. The caller will also handle setting up stdout/stderr to
	 * write to the log file, and will close our stdin. The reason for
	 * writing the pid file in the caller is in order to kill the event
	 * scheduler, it must be done as root since the person swapping the
	 * experiment out might be different then the person who swapped it in
	 * (the scheduler runs as the person who swapped it in). Since the
	 * signal is sent as root, the pid file must not be in a place that
	 * is writable by mere user. 
	 */
	if (log)
		loginit(0, log);

#ifdef  DBIFACE
	/*
	 * Set up DB state.
	 */
	if (!dbinit())
		return 1;
#endif

	/*
	 * Convert server/port to elvin thing.
	 *
	 * XXX This elvin string stuff should be moved down a layer. 
	 */
	if (!server)
		server = "localhost";
#ifdef  RPC
	RPC_init(NULL, BOSSNODE, 0);
#endif
	
	snprintf(buf, sizeof(buf), "elvin://%s%s%s",
		 server,
		 (port ? ":"  : ""),
		 (port ? port : ""));
	server = buf;

	/* Register with the event system: */
	handle = event_register_withkeyfile(server, 1, keyfile);
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

	waitforactive(pid, eid);

	/*
	 * Read the static events list and schedule.
	 */
	if (get_static_events(handle) < 0) {
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
    int			sent = 0;
    struct agent       *agentp;

    /* Get the event's firing time: */
    if (! event_notification_get_int32(handle, notification, "time_usec",
				       (int *) &event.time.tv_usec) ||
	! event_notification_get_int32(handle, notification, "time_sec",
				       (int *) &event.time.tv_sec)) {
	    error("could not get time from notification %p\n",
		  notification);
	    return;
    }

    /*
     * Must map the event to the proper agent running on a particular
     * node. Might be multiple agents if its a group event; send a cloned
     * event for each one. 
     */
    if (! event_notification_get_objname(handle, notification,
					 objname, sizeof(objname))) {
	    error("could not get object name from notification %p\n",
		  notification);
	    return;
    }

    agentp = agents;
    while (agentp) {
	    if (!strcmp(agentp->name, objname)) {
		    /*
		     * Clone the event notification, since we want the
		     * notification to live beyond the callback function:
		     */
		    event.notification =
			    event_notification_clone(handle, notification);
		    
		    if (!event.notification) {
			    error("event_notification_clone failed!\n");
			    return;
		    }

		    /*
		     * Clear the scheduler flag. Not allowed to do this above
		     * the loop cause the notification thats comes in is
		     * "read-only".
		     */
		    if (! event_notification_remove(handle,
					event.notification, "SCHEDULER") ||
			! event_notification_put_int32(handle,
					event.notification, "SCHEDULER", 0)) {
			    error("could not clear scheduler attribute of "
				  "notification %p\n", event.notification);
			    return;
		    }

		    event_notification_clear_host(handle, event.notification);
		    event_notification_set_host(handle,
						event.notification,
						agentp->ipaddr);
		    event_notification_clear_objtype(handle,
						     event.notification);
		    event_notification_set_objtype(handle,
						   event.notification,
						   agentp->objtype);
		    /*
		     * Indicates a group event; must change the name of the
		     * event to the underlying agent.
		     */
		    if (strcmp(agentp->objname, agentp->name)) {
			    event_notification_clear_objname(handle,
					event.notification);
			    event_notification_set_objname(handle,
					event.notification, agentp->objname);
		    }

		    event_notification_insert_hmac(handle, event.notification);
		    event.simevent = !strcmp(agentp->objtype,
					     TBDB_OBJECTTYPE_SIMULATOR);

		    if (debug) {
			    struct timeval now;
	    
			    gettimeofday(&now, NULL);
	    
			    info("Sched: "
				 "note:%p at:%ld:%d now:%ld:%d agent:%s\n",
				 event.notification,
				 event.time.tv_sec, event.time.tv_usec,
				 now.tv_sec, now.tv_usec, agentp->objname);
		    }

		    /*
		     * Enqueue the event notification for resending at the
		     * indicated time:
		     */
		    sched_event_enqueue(event);
		    sent++;
	    }
	    agentp = agentp->next;
    }
    if (!sent) {
	    error("Could not map object to an agent: %s\n", objname);
    }
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
	if (debug)
	    gettimeofday(&now, NULL);

	/*
	 * Sim events are special right now since we handle them here.
	 */
	if (next_event.simevent) {
		if (! handle_simevent(handle, &next_event))
			goto bad;
	}
	else {
	    if (event_notify(handle, next_event.notification) == 0) {
		ERROR("could not fire event\n");
		return;
	    }
	}

	if (debug) {
	    info("Fire:  note:%p at:%ld:%d now:%ld:%d\n",
                 next_event.notification,
		 next_event.time.tv_sec, next_event.time.tv_usec,
		 now.tv_sec,
		 now.tv_usec);
	}
    bad:
        event_notification_free(handle, next_event.notification);
    }
}
/*
 * Stuff to get the event list.
 */
/*
 * Add an agent to the list.
 */
int
AddAgent(char *vname, char *vnode, char *nodeid, char *ipaddr, char *type)
{
	struct agent *agentp;

	if ((agentp = calloc(sizeof(struct agent), 1)) == NULL) {
		error("AddAgent: Out of memory\n");
		return -1;
	}

	strcpy(agentp->name,    vname);
	strcpy(agentp->objname, vname);
	strcpy(agentp->vnode,   vnode);
	strcpy(agentp->objtype, type);

	/*
	 * Look for a wildcard in the vnode slot. As a result
	 * the node_id will come back null from the reserved
	 * table.
	 */
	if (strcmp("*", vnode)) {
		/*
		 * Event goes to specific node.
		 */
		strcpy(agentp->nodeid, nodeid);
		strcpy(agentp->ipaddr, ipaddr);
	}
	else {
		/*
		 * Force events to all nodes. The agents will
		 * need to discriminate on stuff inside the event.
		 */
		strcpy(agentp->nodeid, ADDRESSTUPLE_ALL);
		strcpy(agentp->ipaddr, ADDRESSTUPLE_ALL);
	}
	agentp->next = agents;
	agents       = agentp;
	return 0;
}

/*
 * Add an agent group to the list.
 */
int
AddGroup(char *groupname, char *agentname)
{
	struct agent *agentp, *tmp;

	if ((agentp = calloc(sizeof(struct agent), 1)) == NULL) {
		error("AddGroup: Out of memory\n");
		return -1;
	}

	/*
	 * Find the agent entry.
	 */
	tmp = agents;
	while (tmp) {
		if (!strcmp(tmp->name, agentname))
			break;

		tmp = tmp->next;
	}
	if (! tmp) {
		error("AddGroup: Could not find group event %s/%s\n",
		      groupname, agentname);
		return -1;
	}

	/*
	 * Copy the entry, but rename it to the group name.
	 */
	memcpy((void *)agentp, (void *)tmp, sizeof(struct agent));
	strcpy(agentp->name, groupname);

	agentp->next = agents;
	agents       = agentp;
	return 0;
}

/*
 * Get the agent list and add each agent.
 */
static int
AddAgents()
{
#ifdef	DBIFACE
	MYSQL_RES	*res;
	MYSQL_ROW	row;
	int		nrows;

	/*
	 * Build up a table of agents that can receive dynamic events.
	 * These are stored in the virt_agents table, which we join
	 * with the reserved table to get the physical node name where
	 * the agent is running.
	 *
	 * That is, we want to be able to quickly map from "cbr0" to
	 * the node on which it lives (for dynamic events).
	 */
	res = mydb_query("select vi.vname,vi.vnode,r.node_id,o.type "
			 " from virt_agents as vi "
			 "left join reserved as r on "
			 " r.vname=vi.vnode and r.pid=vi.pid and "
			 " r.eid=vi.eid "
			 "left join event_objecttypes as o on "
			 " o.idx=vi.objecttype "
			 "where vi.pid='%s' and vi.eid='%s'",
			 4, pid, eid);

	if (!res) {
		error("AddAgents: getting virt_agents list for %s/%s\n",
		      pid, eid);
		return -1;
	}

	nrows = mysql_num_rows(res);
	while (nrows--) {
		char	ipaddr[32];
		
		row = mysql_fetch_row(res);

		if (!row[0] || !row[1] || !row[3])
			continue;

		if (strcmp("*", row[1])) {
			if (! row[2]) {
				error("No node_id for vnode %s\n", row[1]);
				continue;
			}
			
			if (! mydb_nodeidtoip(row[2], ipaddr)) {
				error("No ipaddr for node_id %s\n", row[2]);
				continue;
			}
		}
		if (AddAgent(row[0], row[1], row[2], ipaddr, row[3]) < 0) {
		    mysql_free_result(res);
		    return -1;
		}
	}
	mysql_free_result(res);

	/*
	 * Now get the group list. To make life simple, I just create
	 * new entries in the agents table, named by the group name, but
	 * with the info from the underlying agent duplicated, for each
	 * member of the group. 
	 */
	res = mydb_query("select group_name,agent_name from event_groups "
			 "where pid='%s' and eid='%s'",
			 2, pid, eid);

	if (!res) {
		error("AddAgents: getting virt_groups list for %s/%s\n",
		      pid, eid);
		return -1;
	}
	
	nrows = mysql_num_rows(res);
	while (nrows--) {
		row = mysql_fetch_row(res);

		if (AddGroup(row[0], row[1]) < 0) {
		    mysql_free_result(res);
		    return -1;
		}
	}
	mysql_free_result(res);
	return 0;
#else
	if (RPC_agentlist(pid, eid)) {
		error("Could not get agentlist from RPC server\n");
		return -1;
	}

	if (RPC_grouplist(pid, eid)) {
		error("Could not get grouplist from RPC server\n");
		return -1;
	}
	return 0;
#endif
}

int
AddEvent(event_handle_t handle, address_tuple_t tuple, long basetime,
	 char *exidx, char *ftime, char *objname, char *exargs,
	 char *objtype, char *evttype)
{
	sched_event_t	     event;
	double		     firetime = atof(ftime);
	struct agent	    *agentp;
	struct timeval	     time;

	agentp = agents;
	while (agentp) {
		if (!strcmp(agentp->name, objname))
			break;

		agentp = agentp->next;
	}
	if (! agentp) {
		error("AddEvent: Could not map event index %s\n", exidx);
		return -1;
	}

	tuple->host      = agentp->ipaddr;
	tuple->objname   = objname;
	tuple->objtype   = objtype;
	tuple->eventtype = evttype;

	if (debug) 
		info("%8s %10s %10s %10s %10s %10s\n",
		     ftime, objname, objtype,
		     evttype, agentp->ipaddr, 
		     exargs ? exargs : "");

	event.notification = event_notification_alloc(handle, tuple);
	if (! event.notification) {
		error("AddEvent: could not allocate notification\n");
		return -1;
	}
	event_notification_set_arguments(handle, event.notification, exargs);

	event_notification_insert_hmac(handle, event.notification);
	time.tv_sec  = basetime  + (int)firetime;
	time.tv_usec = (int)((firetime - (floor(firetime))) * 1000000);

	if (time.tv_usec >= 1000000) {
		time.tv_sec  += 1;
		time.tv_usec -= 1000000;
	}
	event.time.tv_sec  = time.tv_sec;
	event.time.tv_usec = time.tv_usec;
	event.simevent     = !strcmp(objtype, TBDB_OBJECTTYPE_SIMULATOR);
	sched_event_enqueue(event);
	return 0;
}

/*
 * Get the static event list from the DB and schedule according to
 * the relative time stamps.
 */
static int
get_static_events(event_handle_t handle)
{
	struct timeval	now;
	long            basetime;
	address_tuple_t tuple;
	char		pideid[BUFSIZ];
	event_notification_t notification;
	sched_event_t	event;
#ifdef	DBIFACE
	MYSQL_RES	*res;
	int		nrows;
#endif
	if (AddAgents() < 0)
		return -1;

	if (debug) {
		struct agent *agentp = agents;

		while (agentp) {
			if (!strcmp(agentp->objname, agentp->name)) {
				info("Agent: %15s  %10s %10s %8s %16s\n",
				     agentp->objname, agentp->objtype,
				     agentp->vnode, agentp->nodeid,
				     agentp->ipaddr);
			}
			else {
				info("Group: %15s  %10s\n",
				     agentp->name, agentp->objname);
			}
			agentp = agentp->next;
		}
	}

	/*
	 * Construct an address tuple for the notifications. We can reuse
	 * the same one over and over since the data is copied out.
	 */
	tuple = address_tuple_alloc();
	if (tuple == NULL) {
		error("could not allocate an address tuple\n");
		return -1;
	}
	tuple->expt = pideid;

	sprintf(pideid, "%s/%s", pid, eid);
	gettimeofday(&now, NULL);
	info("Getting event stream at: %lu:%d\n",  now.tv_sec, now.tv_usec);
	
#ifdef	DBIFACE
	/*
	 * Now get the eventlist. There should be entries in the
	 * agents table for anything we find in the list.
	 */
	res = mydb_query("select ex.idx,ex.time,ex.vname,"
			 " ex.arguments,ot.type,et.type from eventlist as ex "
			 "left join event_eventtypes as et on "
			 " ex.eventtype=et.idx "
			 "left join event_objecttypes as ot on "
			 " ex.objecttype=ot.idx "
			 "where ex.pid='%s' and ex.eid='%s' "
			 "order by ex.time ASC",
			 6, pid, eid);
#define EXIDX	 row[0]
#define EXTIME	 row[1]
#define OBJNAME  row[2]
#define EXARGS	 row[3]
#define OBJTYPE	 row[4]
#define EVTTYPE	 row[5]

	/*
	 * XXX Pad the start time out a bit to give this code a chance to run.
	 */
	basetime = now.tv_sec + 30;

	if (!res) {
		error("getting static event list for %s/%s\n", pid, eid);
		return -1;
	}
	nrows = (int) mysql_num_rows(res);

	while (nrows--) {
		MYSQL_ROW	row = mysql_fetch_row(res);

		if (AddEvent(handle, tuple, basetime, EXIDX,
			     EXTIME, OBJNAME, EXARGS, OBJTYPE, EVTTYPE) < 0) {
			mysql_free_result(res);
			return -1;
		}
	}
	mysql_free_result(res);
#else
	/*
	 * XXX Pad the start time out a bit to give this code a chance to run.
	 */
	basetime = now.tv_sec + 30;
	
	if (RPC_eventlist(pid, eid, handle, tuple, basetime)) {
		error("Could not get eventlist from RPC server\n");
		return -1;
	}
#endif
	/*
	 * Generate a TIME starts message.
	 */
	tuple->host      = ADDRESSTUPLE_ALL;
	tuple->objname   = ADDRESSTUPLE_ANY;
	tuple->objtype   = TBDB_OBJECTTYPE_TIME;
	tuple->eventtype = TBDB_EVENTTYPE_START;
	
	notification = event_notification_alloc(handle, tuple);
	if (! notification) {
		error("could not allocate notification\n");
		return -1;
	}
	event_notification_insert_hmac(handle, notification);
	event.simevent     = 0;
	event.notification = notification;
	event.time.tv_sec  = basetime;
	event.time.tv_usec = 0;
	sched_event_enqueue(event);

	info("TIME STARTS will be sent at: %lu:%d\n", now.tv_sec, now.tv_usec);

	gettimeofday(&now, NULL);
	info("The time is now: %lu:%d\n", now.tv_sec, now.tv_usec);

	return 0;
}
	
static int
handle_simevent(event_handle_t handle, sched_event_t *eventp)
{
	char		evtype[TBDB_FLEN_EVEVENTTYPE];
	int		rcode;
	char		cmd[BUFSIZ];
	char		argsbuf[BUFSIZ];

	if (! event_notification_get_eventtype(handle,
					       eventp->notification,
					       evtype, sizeof(evtype))) {
		error("could not get event type from notification %p\n",
		      eventp->notification);
		return 0;
	}

	/*
	 * All we know about is the "SWAPOUT" and "HALT" event!
	 * Also NSESWAP event
	 */
	if (strcmp(evtype, TBDB_EVENTTYPE_HALT) &&
	    strcmp(evtype, TBDB_EVENTTYPE_SWAPOUT) &&
	    strcmp(evtype, TBDB_EVENTTYPE_NSESWAP)) {
		error("cannot handle SIMULATOR event %s.\n", evtype);
		return 0;
	}

	/*
	 * We are lucky! The event scheduler runs as the user! But just in
	 * case, check our uid to make sure we are not root.
	 */
	if (!getuid() || !geteuid()) {
		error("Cannot run SIMULATOR %s as root.\n", evtype);
		return 0;
	}
	/*
	 * Run the command. Output goes ...
	 */
	if (!strcmp(evtype, TBDB_EVENTTYPE_SWAPOUT)) {
	    sprintf(cmd, "swapexp -s out %s %s", pid, eid);
	}
	else if (!strcmp(evtype, TBDB_EVENTTYPE_HALT)) {
	    sprintf(cmd, "endexp %s %s", pid, eid);
	}
	else if (!strcmp(evtype, TBDB_EVENTTYPE_NSESWAP)) {
	    event_notification_get_arguments(handle, eventp->notification,
					     argsbuf, sizeof(argsbuf));
	    /* Need to run nseswap as a background process coz it
	     * sleeps a while before getting done. Also one instance
	     * waits for a swapmod to complete
	     */
	    sprintf(cmd, "nseswap %s %s %s &", pid, eid, argsbuf);
	}
	rcode = system(cmd);
	
	/* Should not return, but ... */
	return (rcode == 0);
}

/*
 * Wait for the experiment to signal active before firing off the events.
 */
static void
waitforactive(char *pid, char *eid)
{
#ifdef  DBIFACE
	int	count = 0;
	
	/*
	 * XXX: Looking at experiment state directly; bad bad bad.
	 */
	while (1) {
		MYSQL_RES	*res;

		res = mydb_query("select state from experiments "
				 "where pid='%s' and eid='%s' and "
				 "      state='active'",
				 1, pid, eid);

		if (!res) {
			fatal("getting current state for %s/%s", pid, eid);
		}
		if (mysql_num_rows(res)) {
			mysql_free_result(res);
			return;
		}
		mysql_free_result(res);

		count++;
		if ((count % 10) == 0)
			info("Waiting for nodes in %s/%s to come up ...\n",
			     pid, eid);

		/*
		 * Don't want to pound the DB too much.
		 */
		sleep(3);
	}
#else
	/*
	 * We use the RPC server, via the generic client.
	 */
	int	status = RPC_waitforactive(pid, eid);

	if (status) 
		fatal("waitforactive: RPC failed!");
#endif
}

