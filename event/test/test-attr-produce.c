/* test-attr-produce.c: Test delivery of events, with attributes (producer). */

static char rcsid[] = "$Id: test-attr-produce.c,v 1.4 2002-01-29 17:23:34 imurdock Exp $";

#include <event.h>

int
main(int argc, char **argv)
{
    event_handle_t handle;
    event_notification_t notification;
    char *server = NULL;
    int c;
    struct timeval time;

    /* Get current time: */
    gettimeofday(&time, NULL);
    TRACE("time.tv_sec = %ld, time.tv_usec = %ld\n",
          time.tv_sec,
          time.tv_usec);

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
    handle = event_register(server);
    if (handle == NULL) {
        ERROR("could not register with event system\n");
        return 1;
    }

    /* Generate a test event, with some attributes: */

    notification = event_notification_alloc(handle, EVENT_HOST_ANY,
                                            EVENT_TEST);
    if (notification == NULL) {
        ERROR("could not allocate notification\n");
        return 1;
    }

    if (event_notification_put_double(handle, notification, "double", 1.23)
        == 0)
    {
        ERROR("could not put double attribute\n");
        return 1;
    }

    if (event_notification_put_int32(handle, notification, "int32", 123)
        == 0)
    {
        ERROR("could not put int32 attribute\n");
        return 1;
    }

    if (event_notification_put_int64(handle, notification, "int64",
                                     100000000000)
        == 0)
    {
        ERROR("could not put int64 attribute\n");
        return 1;
    }

    if (event_notification_put_opaque(handle, notification, "opaque", &time,
                                      sizeof(time))
        == 0)
    {
        ERROR("could not put opaque attribute\n");
        return 1;
    }

    if (event_notification_put_string(handle, notification, "string", "foo")
        == 0)
    {
        ERROR("could not put string attribute\n");
        return 1;
    }

    if (event_notify(handle, notification) == 0) {
        ERROR("could not send test event notification\n");
        return 1;
    }

    event_notification_free(handle, notification);

    /* Unregister with the event system: */
    if (event_unregister(handle) == 0) {
        ERROR("could not unregister with event system\n");
        return 1;
    }

    return 0;
}
