/*
 * event.c --
 *
 *      Testbed event library.  Currently uses the Elvin publish/
 *      subscribe system for routing event notifications.
 *
 * @COPYRIGHT@
 */

static char rcsid[] = "$Id: event.c,v 1.1 2001-11-02 04:43:04 imurdock Exp $";

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <elvin/elvin.h>
#include <event.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif /* MAXHOSTNAMELEN */

static char hostname[MAXHOSTNAMELEN];

/* Register with the event system.  Returns pointer to handle if
   successful, NULL otherwise. */
event_handle_t
event_register(void)
{
    event_handle_t handle;
    elvin_handle_t server;
    elvin_error_t status;

    if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
        ERROR("could not get hostname: %s\n", strerror(errno));
        return 0;
    }

    TRACE("registering with event system (hostname=%s)\n", hostname);

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

/* Unregister with the event system.  Returns non-zero if successful,
   0 otherwise. */
int
event_unregister(event_handle_t handle)
{
    if (!handle) {
        ERROR("invalid handle\n");
        return 0;
    }

    TRACE("unregistering with event system (hostname=%s)\n", hostname);

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

/* Main event loop for event system.  Returns non-zero if successful,
   0 otherwise. */
int
event_main(event_handle_t handle)
{
    int loop = 1;

    if (!handle) {
        ERROR("invalid handle\n"); 
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

/* Send the event notification NOTIFICATION.  Returns non-zero if
   successful, 0 otherwise. */
int
event_notify(event_handle_t handle, event_notification_t notification)
{
    if (!handle) {
        ERROR("invalid handle\n");
        return 0;
    }

    TRACE("sending event notification (notification=%p)\n", notification);

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

/* Allocate a notification of type TYPE for the host HOST (or,
   if HOST is EVENT_HOST_ANY, all hosts).  Returns pointer to
   notification if successful, NULL otherwise. */
event_notification_t
event_notification_alloc(event_handle_t handle, char *host, event_type_t type)
{
    elvin_notification_t notification;

    if (!handle) {
        ERROR("invalid handle\n");
        return NULL;
    }

    TRACE("allocating notification (host=%s, type=%d)\n", host, type);

    notification = elvin_notification_alloc(handle->status);
    if (notification == NULL) {
        ERROR("elvin_notification_alloc failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    TRACE("allocated notification (notification=%p)\n", notification);

    /* Add hostname to notification: */
    if (elvin_notification_add_string(notification, "host", host,
                                      handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_string failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    /* Add type to notification: */
    if (elvin_notification_add_int32(notification, "type", type,
                                     handle->status)
        == 0)
    {
        ERROR("elvin_notification_add_int32 failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    return notification;
}

/* Free the notification NOTIFICATION.  Returns non-zero if successful,
   0 otherwise. */
int
event_notification_free(event_handle_t handle,
                        event_notification_t notification)
{
    if (!handle) {
        ERROR("invalid handle\n");
        return 0;
    }

    TRACE("freeing notification (notification=%p)\n", notification);

    if (elvin_notification_free(notification, handle->status) == 0) {
        ERROR("elvin_notification_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}

struct attr_traverse_arg {
    event_attr_type_t type;
    char *name;
    event_attr_value_t *value;
};

static int attr_traverse(void *rock, char *name, elvin_basetypes_t type,
                         elvin_value_t value, elvin_error_t error);

/* Get the attribute with name NAME and type TYPE from the
   notification NOTIFICATION.  Writes the value of the
   attribute to *VALUE and returns non-zero if successful,
   0 otherwise. */
int
event_notification_attr_get(event_handle_t handle,
                            event_notification_t notification,
                            event_attr_type_t type, char *name,
                            event_attr_value_t *value)
{
    struct attr_traverse_arg arg;

    if (!handle) {
        ERROR("invalid handle\n");
        return 0;
    }

    if (!value) {
        ERROR("invalid result parameter (value)\n");

        return 0;
    }

    arg.type = type;
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

/* Put the attribute with name NAME, type TYPE, and value VALUE into
   the notification NOTIFICATION.  Returns non-zero if successful, 0
   otherwise. */
int
event_notification_attr_put(event_handle_t handle,
                            event_notification_t notification,
                            event_attr_type_t type, char *name,
                            event_attr_value_t value)
{
    if (!handle) {
        ERROR("invalid handle\n");
        return 0;
    }

    switch (type) {
      case EVENT_ATTR_DOUBLE:
          TRACE("adding attribute to notification (notification=%p, "
                "name=%s, value=%f)\n", notification, name, value.d);
          if (elvin_notification_add_real64(notification, name, value.d,
                                            handle->status)
              == 0)
          {
              ERROR("elvin_notification_add_real64 failed: ");
              elvin_error_fprintf(stderr, handle->status);
              return 0;
          }
          break;
      case EVENT_ATTR_INT32:
          TRACE("adding attribute to notification (notification=%p, "
                "name=%s, value=%d)\n", notification, name, value.i);
          if (elvin_notification_add_int32(notification, name, value.i,
                                           handle->status)
              == 0)
          {
              ERROR("elvin_notification_add_int32 failed: ");
              elvin_error_fprintf(stderr, handle->status);
              return 0;
          }
          break;
      case EVENT_ATTR_INT64:
          TRACE("adding attribute to notification (notification=%p, "
                "name=%s, value=%lld)\n", notification, name, value.h);
          if (elvin_notification_add_int64(notification, name, value.h,
                                           handle->status)
              == 0)
          {
              ERROR("elvin_notification_add_int64 failed: ");
              elvin_error_fprintf(stderr, handle->status);
              return 0;
          }
          break;
      case EVENT_ATTR_STRING:
          TRACE("adding attribute to notification (notification=%p, "
                "name=%s, value=\"%s\")\n", notification, name, value.s);
          if (elvin_notification_add_string(notification, name, value.s,
                                            handle->status)
              == 0)
          {
              ERROR("elvin_notification_add_string failed: ");
              elvin_error_fprintf(stderr, handle->status);
              return 0;
          }
          break;
      default:
          ERROR("unknown event notification attribute type (type=%d)\n", type);
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

/* Subscribe to events of type TYPE.  The callback CALLBACK will
   be invoked when events of this type fire that are directed at
   this host (or all hosts).  Returns pointer to subscription if
   successful, NULL otherwise. */
event_subscription_t
event_subscribe(event_handle_t handle, event_notify_callback_t callback,
                event_type_t type, void *data)
{
    elvin_subscription_t subscription;
    struct notify_callback_arg arg;
    char expression[EXPRESSION_LENGTH];

    /* XXX: The declaration of expression has to go last, or the
       local variables on the stack after it get smashed.  Check
       Elvin for buffer overruns. */

    if (!handle) {
        ERROR("invalid handle\n");
        return NULL;
    }

    snprintf(expression, EXPRESSION_LENGTH,
             "(host == \"*\" || host == \"%s\") && type == %d",
             hostname,
             type);

    TRACE("subscribing to event %s\n", expression);

    arg.callback = callback; arg.data = data;

    subscription = elvin_sync_add_subscription(handle->server,
                                               expression, NULL, 1,
                                               notify_callback,
                                               &arg,
                                               handle->status);
    if (subscription == NULL) {
        ERROR("elvin_sync_add_subscription failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    return subscription;
}

/* Callback passed to elvin_notification_traverse in
   event_notification_attr_get.
   Returns 0 if the desired attribute is found, 1 otherwise. */
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

/* Callback passed to elvin_sync_add_subscription in
   event_subscribe. Used to provide our own callback above Elvin's. */
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

    TRACE("received event notification\n");

    assert(arg);
    callback = arg->callback;
    data = arg->data;
    handle.server = server;
    handle.status = status;

    callback(&handle, notification, data);
}
