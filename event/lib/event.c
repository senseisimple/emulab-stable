/*
 * event.c --
 *
 *      Testbed event library.  Currently uses the Elvin publish/
 *      subscribe system for routing event notifications.
 *
 * @COPYRIGHT@
 */

static char rcsid[] = "$Id: event.c,v 1.8 2002-01-31 12:42:51 imurdock Exp $";

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <event.h>

static char hostname[MAXHOSTNAMELEN];


/*
 * Register with the testbed event system.  NAME specifies the name of
 * the event server.  Returns a pointer to a handle that may be passed
 * to other event system routines if the operation is successful, NULL
 * otherwise.
 *
 * Elvin note: NAME is a URL of the form "elvin:/[protocol
 * stack]/[endpoint]", where a protocol stack names a transport
 * module, a security module, and a marshaling module as a comma
 * separated list (e.g., "http,none,xml"), and the endpoint format
 * is dependent on the transport module used.  If no protocol
 * stack is given, the default stack (tcp, none, xdr) is used.  For the
 * testbed's purposes, "elvin://HOSTNAME" should suffice.  If NAME
 * is NULL, then Elvin's server discovery protocol will be used to find
 * the Elvin server.
 */

event_handle_t
event_register(char *name)
{
    event_handle_t handle;
    elvin_handle_t server;
    elvin_error_t status;

    if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
        ERROR("could not get hostname: %s\n", strerror(errno));
        return 0;
    }

    TRACE("registering with event system (hostname=\"%s\")\n", hostname);

    /* Initialize the elvin synchronous interface: */

    status = elvin_sync_init_default();
    if (status == NULL) {
        ERROR("elvin_sync_init_default failed\n");
        return 0;
    }
    server = elvin_handle_alloc(status);
    if (server == NULL) {
        ERROR("elvin_handle_alloc failed: ");
        elvin_error_fprintf(stderr, status);
        return 0;
    }

    /* Set the discovery scope to "testbed", so that we only interact
       with testbed elvin servers. */
    if (elvin_handle_set_discovery_scope(server, "testbed", status) == 0) {
        ERROR("elvin_handle_set_discovery_scope failed: ");
        elvin_error_fprintf(stderr, status);
        return 0;
    }

    /* Set the server URL, if we were passed one by the user. */
    if (name) {
        if (elvin_handle_append_url(server, name, status) == 0) {
            ERROR("elvin_handle_append_url failed: ");
            elvin_error_fprintf(stderr, status);
            return 0;
        }
    }

    /* Connect to the elvin server: */
    if (elvin_sync_connect(server, status) == 0) {
        ERROR("elvin_sync_connect failed: ");
        elvin_error_fprintf(stderr, status);
        return 0;
    }

    /* Allocate a handle to be returned to the caller: */
    handle = xmalloc(sizeof(*handle));
    handle->server = server;
    handle->status = status;

    return handle;
}


/*
 * Unregister with the testbed event system. Returns non-zero if the
 * operation is successful, 0 otherwise.
 */

int
event_unregister(event_handle_t handle)
{
    if (!handle) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("unregistering with event system (hostname=\"%s\")\n", hostname);

    /* Disconnect from the elvin server: */
    if (elvin_sync_disconnect(handle->server, handle->status) == 0) {
        ERROR("elvin_sync_disconnect failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    /* Clean up: */

    if (elvin_handle_free(handle->server, handle->status) == 0) {
        ERROR("elvin_handle_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }
    if (elvin_sync_cleanup(1, handle->status) == 0) {
        ERROR("elvin_sync_cleanup failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    free(handle);

    return 1;
}


/*
 * Enter the main loop of the event system, waiting to receive event
 * notifications. Returns non-zero if the operation is successful, 0
 * otherwise.
 */

int
event_main(event_handle_t handle)
{
    int loop = 1;

    if (!handle) {
        ERROR("invalid parameter\n"); 
        return 0;
    }

    TRACE("entering event loop...\n");

    if (elvin_sync_default_mainloop(&loop, handle->status) == 0) {
        ERROR("elvin_sync_default_mainloop failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Send the event notification NOTIFICATION.  NOTIFICATION is
 * allocated by event_notification_alloc, and may optionally
 * have attributes added to it by event_notification_put_*.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notify(event_handle_t handle, event_notification_t notification)
{
    if (!handle || !notification) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("sending event notification %p\n", notification);

    /* Send notification to Elvin server for routing: */
    if (elvin_sync_notify(handle->server, notification, 1, NULL,
                          handle->status)
        == 0)
    {
        ERROR("elvin_sync_notify failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Schedule the event notification NOTIFICATION to be sent at time
 * TIME.  NOTIFICATION is allocated by event_notification_alloc,
 * and may optionally have attributes added to it by
 * event_notification_put_*.  Returns non-zero if the operation
 * is successful, 0 otherwise.
 *
 * This function essentially operates as a deferred event_notify.
 * event_notify sends notifications immediately,
 * whereas event_schedule sends notifications at some later time.
 */

int
event_schedule(event_handle_t handle, event_notification_t notification,
               struct timeval *time)
{
    event_type_t type;

    if (!handle || !notification || !time) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("scheduling event notification %p to be sent at time (%ld, %ld)\n",
          notification, time->tv_sec, time->tv_usec);

    /* Change the event type to EVENT_SCHEDULE, saving the old event
       type in the notification structure so the event scheduler can
       change it back when the event is scheduled: */

    if (event_notification_get_int32(handle, notification, "type",
                                     (int *) &type)
        == 0)
    {
        ERROR("could not get old type attribute from notification %p\n",
              notification);
        return 0;
    }

    if (event_notification_remove(handle, notification, "type") == 0) {
        ERROR("could not remove old type attribute from notification %p\n",
              notification);
        return 0;
    }

    if (event_notification_put_int32(handle, notification, "type",
                                     EVENT_SCHEDULE)
        == 0)
    {
        ERROR("could not add new type attribute to notification %p\n",
              notification);
        return 0;
    }
    if (event_notification_put_int32(handle, notification, "old_type",
                                     type)
        == 0)
    {
        ERROR("could not add old type attribute to notification %p\n",
              notification);
        return 0;
    }

    /* Add the time this event should be fired to the notification
       structure: */

    if (event_notification_put_int32(handle, notification, "time_sec",
                                     time->tv_sec)
        == 0)
    {
        ERROR("could not add time.tv_sec attribute to notification %p\n",
              notification);
        return 0;
    }
    if (event_notification_put_int32(handle, notification, "time_usec",
                                     time->tv_usec)
        == 0)

    {
        ERROR("could not add time.tv_usec attribute to notification %p\n",
              notification);
        return 0;
    }

    /* Send the event notification: */
    return event_notify(handle, notification);
}


/*
 * Allocate an event notification.  The HOST parameter specifies
 * the hostname of the node that should receive the notification,
 * or EVENT_HOST_ANY if the notification should go to all hosts.
 * The TYPE parameter specifies the event type.  Returns
 * a pointer to an event notification structure if the operation
 * is successful, 0 otherwise.
 */

event_notification_t
event_notification_alloc(event_handle_t handle, char *host, event_type_t type)
{
    elvin_notification_t notification;

    if (!handle || !host || !type) {
        ERROR("invalid paramater\n");
        return NULL;
    }

    TRACE("allocating notification (host=\"%s\", type=%d)\n", host, type);

    notification = elvin_notification_alloc(handle->status);
    if (notification == NULL) {
        ERROR("elvin_notification_alloc failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    TRACE("allocated notification %p\n", notification);

    /* Add hostname to notification: */
    if (event_notification_put_string(handle, notification, "host", host)
        == 0)
    {
        ERROR("could not add host attribute to notification %p\n",
              notification);
        return NULL;
    }

    /* Add type to notification: */
    if (event_notification_put_int32(handle, notification, "type", type)
        == 0)
    {
        ERROR("could not add type attribute to notification %p\n",
              notification);
        return NULL;
    }

    return notification;
}


/*
 * Free the event notification NOTIFICATION. Returns non-zero if the
 * operation is successful, 0 otherwise.
 */

int
event_notification_free(event_handle_t handle,
                        event_notification_t notification)
{
    if (!handle || !notification) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("freeing notification %p\n", notification);

    if (elvin_notification_free(notification, handle->status) == 0) {
        ERROR("elvin_notification_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


struct attr_traverse_arg {
    char *name;
    elvin_value_t *value;
};

static int attr_traverse(void *rock, char *name, elvin_basetypes_t type,
                         elvin_value_t value, elvin_error_t error);

/*
 * Get the attribute with name NAME from the event notification
 * NOTIFICATION.
 * Writes the value of the attribute to *VALUE and returns
 * non-zero if the named attribute is found, 0 otherwise.
 */

static int
event_notification_get(event_handle_t handle,
                       event_notification_t notification,
                       char *name, elvin_value_t *value)
{
    struct attr_traverse_arg arg;

    if (!handle || !notification || !name || !value) {
        ERROR("invalid parameter\n");
        return 0;
    }

    arg.name = name;
    arg.value = value;

    /* attr_traverse returns 0 to indicate that it has found the
       desired attribute. */
    if (elvin_notification_traverse(notification, attr_traverse, &arg,
                                    handle->status)
        == 0)
    {
        /* Found it. */
        return 1;
    }

    /* Didn't find it. */
    return 0;
}


/*
 * Get the double attribute with name NAME from the event
 * notification NOTIFICATION.
 * Writes the value of the attribute to *VALUE and returns
 * non-zero if the named attribute is found, 0 otherwise.
 */

int
event_notification_get_double(event_handle_t handle,
                              event_notification_t notification,
                              char *name, double *value)
{
    elvin_value_t v;

    if (!handle || !notification || !name || !value) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (event_notification_get(handle, notification, name, &v) == 0) {
        ERROR("could not get double attribute \"%s\" from notification %p\n",
              name, notification);
        return 0;
    }

    *value = v.d;

    return 1;
}


/*
 * Get the int32 attribute with name NAME from the event
 * notification NOTIFICATION.
 * Writes the value of the attribute to *VALUE and returns
 * non-zero if the named attribute is found, 0 otherwise.
 */

int
event_notification_get_int32(event_handle_t handle,
                             event_notification_t notification,
                             char *name, int32_t *value)
{
    elvin_value_t v;

    if (!handle || !notification || !name || !value) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (event_notification_get(handle, notification, name, &v) == 0) {
        ERROR("could not get int32 attribute \"%s\" from notification %p\n",
              name, notification);
        return 0;
    }

    *value = v.i;

    return 1;
}


/*
 * Get the int64 attribute with name NAME from the event
 * notification NOTIFICATION.
 * Writes the value of the attribute to *VALUE and returns
 * non-zero if the named attribute is found, 0 otherwise.
 */

int
event_notification_get_int64(event_handle_t handle,
                             event_notification_t notification,
                             char *name, int64_t *value)
{
    elvin_value_t v;

    if (!handle || !notification || !name || !value) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (event_notification_get(handle, notification, name, &v) == 0) {
        ERROR("could not get int64 attribute \"%s\" from notification %p\n",
              name, notification);
        return 0;
    }

    *value = v.h;

    return 1;
}


/*
 * Get the opaque attribute with name NAME from the event
 * notification NOTIFICATION.
 * Writes LENGTH bytes into *BUFFER and returns non-zero if the named
 * attribute is found, 0 otherwise.
 */

int
event_notification_get_opaque(event_handle_t handle,
                              event_notification_t notification,
                              char *name, void *buffer, int length)
{
    elvin_value_t v;

    if (!handle || !notification || !name || !buffer || !length) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (event_notification_get(handle, notification, name, &v) == 0) {
        ERROR("could not get opaque attribute \"%s\" from notification %p\n",
              name, notification);
        return 0;
    }

    memcpy(buffer, v.o.data, length);

    return 1;
}


/*
 * Get the string attribute with name NAME from the event
 * notification NOTIFICATION.
 * Writes LENGTH bytes into *BUFFER and returns non-zero if the named
 * attribute is found, 0 otherwise.
 */

int
event_notification_get_string(event_handle_t handle,
                              event_notification_t notification,
                              char *name, char *buffer, int length)
{
    elvin_value_t v;

    if (!handle || !notification || !name || !buffer || !length) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (event_notification_get(handle, notification, name, &v) == 0) {
        ERROR("could not get string attribute \"%s\" from notification %p\n",
              name, notification);
        return 0;
    }

    strncpy(buffer, v.s, length);

    return 1;
}


/*
 * Add a double attribute with name NAME and value VALUE to the
 * notification NOTIFICATION.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notification_put_double(event_handle_t handle,
                              event_notification_t notification,
                              char *name, double value)
{
    if (!handle || !notification || !name) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("adding attribute (name=\"%s\", value=%f) to notification %p\n",
          name, value, notification);

    if (elvin_notification_add_real64(notification, name, value,
                                      handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_real64 failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Add an int32 attribute with name NAME and value VALUE to the
 * notification NOTIFICATION.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notification_put_int32(event_handle_t handle,
                             event_notification_t notification,
                             char *name, int32_t value)
{
    if (!handle || !notification || !name) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("adding attribute (name=\"%s\", value=%d) to notification %p\n",
          name, value, notification);

    if (elvin_notification_add_int32(notification, name, value,
                                      handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_int32 failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Add an int64 attribute with name NAME and value VALUE to the
 * notification NOTIFICATION.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notification_put_int64(event_handle_t handle,
                             event_notification_t notification,
                             char *name, int64_t value)
{
    if (!handle || !notification || !name) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("adding attribute (name=\"%s\", value=%lld) to notification %p\n",
          name, value, notification);

    if (elvin_notification_add_int64(notification, name, value,
                                     handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_int64 failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Add an opaque attribute with name NAME to the notification
 * NOTIFICATION. The attribute is stored in the buffer BUFFER
 * with length LENGTH.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notification_put_opaque(event_handle_t handle,
                              event_notification_t notification,
                              char *name, void *buffer, int length)
{
    if (!handle || !notification || !buffer || !length) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("adding attribute (name=\"%s\", value=<opaque>) "
          "to notification %p\n", name, notification);

    if (elvin_notification_add_opaque(notification, name, buffer, length,
                                      handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_opaque failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Add a string attribute with name NAME and value VALUE to the
 * notification NOTIFICATION.
 * Returns non-zero if the operation is successful, 0 otherwise.
 */

int
event_notification_put_string(event_handle_t handle,
                              event_notification_t notification,
                              char *name, char *value)
{
    if (!handle || !notification || !name || !value) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("adding attribute (name=\"%s\", value=\"%s\") to notification %p\n",
          name, value, notification);

    if (elvin_notification_add_string(notification, name, value,
                                      handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_string failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


/*
 * Remove the attribute with name NAME and type TYPE from the event
 * notification NOTIFICATION.  Returns non-zero if the operation is
 * successful, 0 otherwise.
 */

int
event_notification_remove(event_handle_t handle,
                          event_notification_t notification, char *name)
{
    if (!handle || !notification || !name) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("removing attribute \"%s\" from notification %p\n",
          name, notification);

    if (elvin_notification_remove(notification, name, handle->status) == 0) {
        ERROR("elvin_notification_remove failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


struct notify_callback_arg {
    event_notify_callback_t callback;
    void *data;
};

static void notify_callback(elvin_handle_t server,
                            elvin_subscription_t subscription,
                            elvin_notification_t notification, int is_secure,
                            void *rock, elvin_error_t status);

#define EXPRESSION_LENGTH 1024

/*
 * Subscribe to events of type TYPE.  Event notifications that match
 * TYPE will be passed to the callback function CALLBACK; DATA is
 * an arbitrary pointer that will be passed to the callback function.
 * Callback functions are of the form
 *
 *     void callback(event_handle_t handle,
 *                   event_notification_t notification,
 *                   char *host,
 *                   event_type_t type,
 *                   void *data);
 *
 * where HANDLE is the handle to the event server, NOTIFICATION is the
 * event notification, HOST and TYPE are the respective attributes of
 * the event notification, and DATA is the arbitrary pointer passed to
 * event_subscribe.  Returns a pointer to an event
 * subscription structure if the operation is successful, 0 otherwise.
 */

event_subscription_t
event_subscribe(event_handle_t handle, event_notify_callback_t callback,
                event_type_t type, void *data)
{
    elvin_subscription_t subscription;
    struct notify_callback_arg *arg;
    char expression[EXPRESSION_LENGTH];

    /* XXX: The declaration of expression has to go last, or the
       local variables on the stack after it get smashed.  Check
       Elvin for buffer overruns. */

    if (!handle || !callback || !type) {
        ERROR("invalid parameter\n");
        return NULL;
    }

    snprintf(expression, EXPRESSION_LENGTH,
             "(host == \"*\" || host == \"%s\") && type == %d",
             hostname,
             type);

    TRACE("subscribing to event %s\n", expression);

    arg = xmalloc(sizeof(*arg));
    /* XXX: Free this in an event_unsubscribe.. */
    arg->callback = callback;
    arg->data = data;

    subscription = elvin_sync_add_subscription(handle->server,
                                               expression, NULL, 1,
                                               notify_callback,
                                               arg,
                                               handle->status);
    if (subscription == NULL) {
        ERROR("elvin_sync_add_subscription failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    return subscription;
}


/*
 * Callback passed to elvin_notification_traverse in
 * event_notification_attr_get.
 * Returns 0 if the desired attribute is found, 1 otherwise.
 */

static int
attr_traverse(void *rock, char *name, elvin_basetypes_t type,
              elvin_value_t value, elvin_error_t error)
{
    struct attr_traverse_arg *arg = (struct attr_traverse_arg *) rock;

    assert(arg);

    /* If this is the name, then set the result value parameter to
       VALUE. */
    if (strcmp(name, arg->name) == 0) {
        *arg->value = value;
        return 0;
    }

    return 1;
}


/*
 * Callback passed to elvin_sync_add_subscription in
 * event_subscribe. Used to provide our own callback above Elvin's.
 */

static void
notify_callback(elvin_handle_t server,
                elvin_subscription_t subscription,
                elvin_notification_t notification, int is_secure,
                void *rock, elvin_error_t status)
{
    struct event_handle handle;
    struct notify_callback_arg *arg = (struct notify_callback_arg *) rock;
    event_notify_callback_t callback;
    void *data;
    char host[MAXHOSTNAMELEN];
    event_type_t type;

    TRACE("received event notification\n");

    assert(arg);
    callback = arg->callback;
    data = arg->data;
    handle.server = server;
    handle.status = status;

    if (event_notification_get_string(&handle, notification, "host",
                                      host, MAXHOSTNAMELEN)
        == 0)
    {
        ERROR("could not get host attribute from notification %p\n",
              notification);
        return;
    }

    if (event_notification_get_int32(&handle, notification, "type",
                                     (int *) &type)
        == 0)
    {
        ERROR("could not get type attribute from notification %p\n",
              notification);
        return;
    }

    callback(&handle, notification, host, type, data);
}
