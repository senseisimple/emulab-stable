/* test-consume.c: Test delivery of events (consumer). */

static char rcsid[] = "$Id: test-consume.c,v 1.4 2002-02-19 15:51:01 imurdock Exp $";

#include <event.h>

static void callback(event_handle_t handle, event_notification_t notification,
                     char *host, event_type_t type, void *data);

int
main(int argc, char **argv)
{
    event_handle_t handle;
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
    handle = event_register(server, 0);
    if (handle == NULL) {
        ERROR("could not register with event system\n");
        return 1;
    }

    /* Subscribe to the test event: */
    if (event_subscribe(handle, callback, EVENT_TEST, "event received")
        == NULL)
    {
        ERROR("could not subscribe to test event\n");
        return 1;
    }

    /* Begin the event loop, waiting to receive event notifications: */
    event_main(handle);

    /* Unregister with the event system: */
    if (event_unregister(handle) == 0) {
        ERROR("could not unregister with event system\n");
        return 1;
    }

    return 0;
}

static void
callback(event_handle_t handle, event_notification_t notification, char *host,
         event_type_t type, void *data)
{
    TRACE("data: %s\n", (char *) data);
    TRACE("host: %s\n", host);
    TRACE("type: %d\n", type);
}
