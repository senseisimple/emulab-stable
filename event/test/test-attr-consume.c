/* test-consume.c: Test delivery of events, with attributes (consumer). */

static char rcsid[] = "$Id: test-attr-consume.c,v 1.1 2001-11-02 04:47:14 imurdock Exp $";

#include <event.h>

static void callback(event_handle_t handle, event_notification_t notification,
                     void *data);

int
main(int argc, char **argv)
{
    event_handle_t handle;

    /* Register with the event system: */
    handle = event_register();
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
callback(event_handle_t handle, event_notification_t notification, void *data)
{
    char *message = (char *) data;
    char *host;
    event_type_t type;
    double attr_double;
    int32_t attr_int32;
    int64_t attr_int64;
    char *attr_string;
    
    TRACE("the message is: %s\n", message);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_STRING, "host",
                                    (event_attr_value_t *) &host)
        == 0)
    {
        ERROR("could not get host attribute\n");
        return;
    }

    TRACE("host: %s\n", host);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_STRING, "type",
                                    (event_attr_value_t *) &type)
        == 0)
    {
        ERROR("could not get type attribute\n");
        return;
    }

    TRACE("type: %d\n", type);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_DOUBLE, "double",
                                    (event_attr_value_t *) &attr_double)
        == 0)
    {
        ERROR("could not get double attribute\n");
        return;
    }

    TRACE("double: %f\n", attr_double);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_DOUBLE, "int32",
                                    (event_attr_value_t *) &attr_int32)
        == 0)
    {
        ERROR("could not get int32 attribute\n");
        return;
    }

    TRACE("int32: %d\n", attr_int32);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_DOUBLE, "int64",
                                    (event_attr_value_t *) &attr_int64)
        == 0)
    {
        ERROR("could not get int64 attribute\n");
        return;
    }

    TRACE("int64: %lld\n", attr_int64);

    if (event_notification_attr_get(handle, notification,
                                    EVENT_ATTR_DOUBLE, "string",
                                    (event_attr_value_t *) &attr_string)
        == 0)
    {
        ERROR("could not get string attribute\n");
        return;
    }

    TRACE("string: \"%s\"\n", attr_string);
}
