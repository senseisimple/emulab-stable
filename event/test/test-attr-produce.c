/* test-attr-produce.c: Test delivery of events, with attributes (producer). */

static char rcsid[] = "$Id: test-attr-produce.c,v 1.1 2001-11-02 04:47:14 imurdock Exp $";

#include <event.h>

int
main(int argc, char **argv)
{
    event_handle_t handle;
    event_notification_t notification;

    /* Register with the event system: */
    handle = event_register();
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

    if (event_notification_attr_put(handle, notification,
                                    EVENT_ATTR_DOUBLE, "double",
                                    (event_attr_value_t) 1.23)
        == 0)
    {
        ERROR("could not put double attribute\n");
        return 1;
    }
    if (event_notification_attr_put(handle, notification,
                                    EVENT_ATTR_INT32, "int32",
                                    (event_attr_value_t) 123)
        == 0)
    {
        ERROR("could not put int32 attribute\n");
        return 1;
    }
    if (event_notification_attr_put(handle, notification,
                                    EVENT_ATTR_INT64, "int64",
                                    (event_attr_value_t) 100000000000)
        == 0)
    {
        ERROR("could not put int64 attribute\n");
        return 1;
    }
    if (event_notification_attr_put(handle, notification,
                                    EVENT_ATTR_STRING, "string",
                                    (event_attr_value_t) "foo")
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
