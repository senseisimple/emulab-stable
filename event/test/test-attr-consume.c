/* test-attr-consume.c: Test delivery of events, with attributes (consumer). */

#include "event.h"

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
    double attr_double;
    int32_t attr_int32;
    int64_t attr_int64;
    char attr_string[64];
    struct timeval attr_opaque;
    
    TRACE("data: %s\n", (char *) data);

    TRACE("host: %s\n", host);

    TRACE("type: %d\n", type);

    if (event_notification_get_double(handle, notification, "double",
                                      &attr_double)
        == 0)
    {
        ERROR("could not get double attribute\n");
        return;
    }

    TRACE("double: %f\n", attr_double);

    if (event_notification_get_int32(handle, notification, "int32",
                                     &attr_int32)
        == 0)
    {
        ERROR("could not get int32 attribute\n");
        return;
    }

    TRACE("int32: %d\n", attr_int32);

    if (event_notification_get_int64(handle, notification, "int64",
                                     &attr_int64)
        == 0)
    {
        ERROR("could not get int64 attribute\n");
        return;
    }

    TRACE("int64: %lld\n", attr_int64);

    if (event_notification_get_opaque(handle, notification, "opaque",
                                      &attr_opaque, sizeof(attr_opaque))
        == 0)
    {
        ERROR("could not get opaque attribute\n");
        return;
    }

    TRACE("opaque: (tv_sec=%ld, tv_usec=%ld)\n",
          attr_opaque.tv_sec,
          attr_opaque.tv_usec);

    if (event_notification_get_string(handle, notification, "string",
                                      attr_string, 64)
        == 0)
    {
        ERROR("could not get string attribute\n");
        return;
    }

    TRACE("string: \"%s\"\n", attr_string);
}
