/*
 * sched.c --
 *
 *      Testbed event scheduler.
 *
 * @COPYRIGHT@
 */

static char rcsid[] = "$Id: sched.c,v 1.1 2001-11-02 05:40:54 imurdock Exp $";

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>

static void sched_seed_event_queue(void);
#ifdef TEST_SCHED
static void sched_seed_event_queue_debug(void);
#endif /* TEST_SCHED */

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

int
main(int argc, char **argv)
{
    sched_event_t next_event;
    struct timeval next_event_wait, now;
    event_handle_t handle;
    event_notification_t notification;

    /* Register with the event system: */
    handle = event_register();
    if (handle == NULL) {
        ERROR("could not register with event system\n");
        return 1;
    }

#ifdef TEST_SCHED
    /* Seed event queue with scripted events, for testing. */
    sched_seed_event_queue_debug();
#endif /* TEST_SCHED */

    /* Get static events from testbed database. */
    sched_seed_event_queue();

    while (sched_event_dequeue(&next_event) != 0) {
        /* Determine how long to wait before firing the next event. */
        next_event_wait = sched_time_until_event_fires(next_event);

        /* If the event's firing time is in the future, then use
           select to wait until the event should fire. */
        if (next_event_wait.tv_sec >= 0 && next_event_wait.tv_usec > 0) {
            if (select(0, NULL, NULL, NULL, &next_event_wait) != 0) {
                ERROR("select did not timeout\n");
                return 1;
            }
        }

        /* Fire event. */

        gettimeofday(&now, NULL);

        TRACE("firing event (event=(time=(tv_sec=%ld, tv_usec=%ld)), "
              "host=%s, type=%d) "
              "at time (time=(tv_sec=%ld, tv_usec=%ld))\n",
              next_event.time.tv_sec,
              next_event.time.tv_usec,
              next_event.host,
              next_event.type,
              now.tv_sec,
              now.tv_usec);

        notification = event_notification_alloc(handle, next_event.host,
                                                next_event.type);
        if (notification == NULL) {
            ERROR("could not allocate notification\n");
            return 1;
        }

        if (event_notify(handle, notification) == 0) {
            ERROR("could not fire event\n");
            return 1;
        }

        event_notification_free(handle, notification);
    }

    /* Unregister with the event system: */
    if (event_unregister(handle) == 0) {
        ERROR("could not unregister with event system\n");
        return 1;
    }

    return 0;
}

static void
sched_seed_event_queue(void)
{
    /* XXX: Load events from database here. */
}

#ifdef TEST_SCHED
/* Load scripted events into the event queue. */
static void
sched_seed_event_queue_debug(void)
{
    sched_event_t event;
    struct timeval now;

    strncpy(event.host, EVENT_HOST_ANY, MAXHOSTNAMELEN);
    event.type = EVENT_TEST;

    gettimeofday(&now, NULL);

    event.time = now;
    event.time.tv_sec += 5;
    if (sched_event_enqueue(event) == 0) {
        ERROR("could not enqueue event\n");
        return;
    }

    /* Do two events at once, to make sure we can deal with that. */
    event.time = now;
    event.time.tv_sec += 60;
    if (sched_event_enqueue(event) == 0) {
        ERROR("could not enqueue event\n");
        return;
    }
    event.time = now;
    event.time.tv_sec += 60;
    if (sched_event_enqueue(event) == 0) {
        ERROR("could not enqueue event\n");
        return;
    }

    event.time = now;
    event.time.tv_sec += 120;
    if (sched_event_enqueue(event) == 0) {
        ERROR("could not enqueue event\n");
        return;
    }

    event.time = now;
    event.time.tv_sec += 300;
    if (sched_event_enqueue(event) == 0) {
        ERROR("could not enqueue event\n");
        return;
    }
}
#endif /* TEST_SCHED */
