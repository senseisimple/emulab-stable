/* test-produce.c: Test delivery of events (producer). */

static char rcsid[] = "$Id: test-produce.c,v 1.2 2001-11-06 17:24:16 imurdock Exp $";

#include <event.h>

int
main(int argc, char **argv)
{
    event_handle_t handle;
    event_notification_t notification;
    char *server = NULL;
    int c;

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

    /* Generate a test event: */

    notification = event_notification_alloc(handle, EVENT_HOST_ANY,
                                            EVENT_TEST);
    if (notification == NULL) {
        ERROR("could not allocate notification\n");
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
