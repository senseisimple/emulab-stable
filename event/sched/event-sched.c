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
#include "event-sched.h"

static void enqueue(event_handle_t handle, event_notification_t notification,
                    char *host, event_type_t type, void *data);
static void dequeue(event_handle_t handle);

int
main(int argc, char **argv)
{
    event_handle_t handle;
    char *server = NULL;
    int c;

    /* Initialize event queue semaphores: */
    sched_event_init();

    while ((c = getopt(argc, argv, "s:")) != -1) {
        switch (c) {
          case 's':
              server = optarg;
              break;
          default:
              fprintf(stderr, "Usage: %s [-s SERVER]\n", argv[0]);
              return 1;
        }
    }

    /* Register with the event system: */
    handle = event_register(server, 1);
    if (handle == NULL) {
        ERROR("could not register with event system\n");
        return 1;
    }

    /* Subscribe to the EVENT_SCHEDULE event, and enqueue events as
       they arrive: */
    if (event_subscribe(handle, enqueue, EVENT_SCHEDULE, NULL) == NULL) {
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
enqueue(event_handle_t handle, event_notification_t notification, char *host,
        event_type_t type, void *data)
{
    sched_event_t event;
    event_type_t old_type;

    /* Clone the event notification, since we want the notification to
       live beyond the callback function: */
    event.notification = elvin_notification_clone(notification,
                                                  handle->status);
    if (!event.notification) {
        ERROR("elvin_notification_clone failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return;
    }

    /* Restore the original event type: */

    if (event_notification_get_int32(handle, event.notification, "old_type",
                                    (int *) &old_type)
        == 0)
    {
        ERROR("could not restore type attribute of notification %p\n",
              event.notification);
        return;
    }

    if (event_notification_remove(handle, event.notification, "type") == 0) {
        ERROR("could not restore type attribute of notification %p\n",
              event.notification);
        return;
    }

    if (event_notification_put_int32(handle, event.notification, "type",
                                     old_type)
        == 0)
    {
        ERROR("could not restore type attribute of notification %p\n",
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

    while (sched_event_dequeue(&next_event, 1) != 0) {
        /* Determine how long to wait before firing the next event. */
        next_event_wait = sched_time_until_event_fires(next_event);

        /* If the event's firing time is in the future, then use
           select to wait until the event should fire. */
        if (next_event_wait.tv_sec >= 0 && next_event_wait.tv_usec > 0) {
            if (select(0, NULL, NULL, NULL, &next_event_wait) != 0) {
                ERROR("select did not timeout\n");
                return;
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
