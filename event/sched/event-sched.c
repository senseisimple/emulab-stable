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
 * @COPYRIGHT@
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include "event-sched.h"
#include "libdb.h"

static void enqueue(event_handle_t handle,
		    event_notification_t notification, void *data);
static void dequeue(event_handle_t handle);

static char	*progname;
static char	*pid, *eid;
static int	get_static_events(event_handle_t handle);

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
    char buf[BUFSIZ];
    int c;

    /* Initialize event queue semaphores: */
    sched_event_init();

    while ((c = getopt(argc, argv, "s:p:")) != -1) {
        switch (c) {
          case 's':
              server = optarg;
              break;
          case 'p':
              port = optarg;
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

    /* Register with the event system: */
    handle = event_register(server, 1);
    if (handle == NULL) {
        ERROR("could not register with event system\n");
        return 1;
    }

    /*
     * Read the static events list and schedule.
     */
    if (!get_static_events(handle)) {
        ERROR("could not get static event list\n");
        return 1;
    }

    /*
     * Construct an address tuple for event subscription. We set the scheduler
     * flag to indicate we want to capture those notifications.
     */
    tuple = address_tuple_alloc();
    if (tuple == NULL) {
	    ERROR("could not allocate an address tuple\n");
	    return 1;
    }
    tuple->scheduler = 1;

    if (event_subscribe(handle, enqueue, tuple, NULL) == NULL) {
        ERROR("could not subscribe to EVENT_SCHEDULE event\n");
        return 1;
    }

    /* Dequeue events and process them at the appropriate times: */
    dequeue(handle);

    /* Unregister with the event system: */
    if (event_unregister(handle) == 0) {
        ERROR("could not unregister with event system\n");
        return 1;
    }

    return 0;
}

/* Enqueue event notifications as they arrive. */
static void
enqueue(event_handle_t handle, event_notification_t notification, void *data)
{
    sched_event_t event;

    /* Clone the event notification, since we want the notification to
       live beyond the callback function: */
    event.notification = elvin_notification_clone(notification,
                                                  handle->status);
    if (!event.notification) {
        ERROR("elvin_notification_clone failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return;
    }

    /* Clear the scheduler flag */
    if (! event_notification_remove(handle, event.notification, "SCHEDULER") ||
	! event_notification_put_int32(handle,
				       event.notification, "SCHEDULER", 0)) {
        ERROR("could not clear scheduler attribute of notification %p\n",
              event.notification);
        return;
    }

    /* Get the event's firing time: */

    if (event_notification_get_int32(handle, event.notification, "time_sec",
                                     (int *) &event.time.tv_sec)
        == 0)
    {
        ERROR("could not get time.tv_sec attribute from notification %p\n",
              event.notification);
        return;
    }

    if (event_notification_get_int32(handle, event.notification, "time_usec",
                                     (int *) &event.time.tv_usec)
        == 0)
    {
        ERROR("could not get time.tv_usec attribute from notification %p\n",
              event.notification);
        return;
    }

    /* Enqueue the event notification for resending at the indicated
       time: */
    sched_event_enqueue(event);
}

/* Returns the amount of time until EVENT fires. */
static struct timeval
sched_time_until_event_fires(sched_event_t event)
{
    struct timeval now, time;

    gettimeofday(&now, NULL);

    time.tv_sec = event.time.tv_sec - now.tv_sec;
    time.tv_usec = event.time.tv_usec - now.tv_usec;
    if (time.tv_usec < 0) {
        time.tv_sec -= 1;
        time.tv_usec += 1000000;
    }

    return time;
}

/* Dequeue events from the event queue and fire them at the
   appropriate time.  Runs in a separate thread. */
static void
dequeue(event_handle_t handle)
{
    sched_event_t next_event;
    struct timeval next_event_wait, now;
    int foo;

    while (sched_event_dequeue(&next_event, 1) != 0) {
        /* Determine how long to wait before firing the next event. */
    again:
        next_event_wait = sched_time_until_event_fires(next_event);

        /* If the event's firing time is in the future, then use
           select to wait until the event should fire. */
        if (next_event_wait.tv_sec >= 0 && next_event_wait.tv_usec > 0) {
            if ((foo = select(0, NULL, NULL, NULL, &next_event_wait)) != 0) {
		/*
		 * I'll assume that this fails cause of a pthread
		 * related signal issue.
		 */
		ERROR("select did not timeout %d %d\n", foo, errno);
		goto again;
            }
        }

        /* Fire event. */

        gettimeofday(&now, NULL);

        TRACE("firing event (event=(notification=%p, "
              "time=(tv_sec=%ld, tv_usec=%ld)) "
              "at time (time=(tv_sec=%ld, tv_usec=%ld))\n",
              next_event.notification,
              next_event.time.tv_sec,
              next_event.time.tv_usec,
              now.tv_sec,
              now.tv_usec);

        if (event_notify(handle, next_event.notification) == 0) {
            ERROR("could not fire event\n");
            return;
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

	res = mydb_query("select ex.time,ex.vnode,ex.vname,ex.arguments,"
			 " ot.type,et.type,i.IP from %s_%s_events as ex "
			 "left join event_eventtypes as et on "
			 " ex.eventtype=et.idx "
			 "left join event_objecttypes as ot on "
			 " ex.objecttype=ot.idx "
			 "left join reserved as r on "
			 " ex.vnode=r.vname and r.pid='%s' and r.eid='%s' "
			 "left join nodes as n on r.node_id=n.node_id "
			 "left join node_types as nt on nt.type=n.type "
			 "left join interfaces as i on "
			 " i.node_id=r.node_id and i.iface=nt.control_iface",
			 7, pid, eid, pid, eid);
#define EXTIME	row[0]
#define EXVNODE	row[1]
#define EXVNAME	row[2]
#define EXARGS	row[3]
#define OBJTYPE	row[4]
#define EVTTYPE	row[5]
#define IPADDR	row[6]

	if (!res) {
		ERROR("getting static event list for %s/%s", pid, eid);
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
		ERROR("could not allocate an address tuple\n");
		return 1;
	}

	sprintf(pideid, "%s/%s", pid, eid);
	gettimeofday(&now, NULL);
	
	while (nrows) {
		sched_event_t	event;
		double		firetime;

		row = mysql_fetch_row(res);
		firetime = atof(EXTIME);
			
		DBG("EV: %8s %10s %10s %10s %10s %10s %10s\n",
		    row[0], row[1], row[2],
		    row[3] ? row[3] : "",
		    row[4], row[5],
		    row[6] ? row[6] : "");
		

		tuple->expt      = pideid;
		tuple->host      = IPADDR;
		tuple->objname   = EXVNAME;
		tuple->objtype   = OBJTYPE;
		tuple->eventtype = EVTTYPE;

		event.notification = event_notification_alloc(handle, tuple);
		if (! event.notification) {
			ERROR("could not allocate notification\n");
			mysql_free_result(res);
			return 1;
		}

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
	return 1;
}
	
