/*
 * event.h --
 *
 *      Definitions for the testbed event library.
 *
 * @COPYRIGHT@
 *
 * $Id: event.h,v 1.1 2001-11-02 04:43:04 imurdock Exp $
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdio.h>
#include <elvin/elvin.h>

/* Handle to the event server: */
struct event_handle {
    elvin_handle_t server;
    elvin_error_t status;
};
typedef struct event_handle * event_handle_t;

/* Event notification: */
typedef elvin_notification_t event_notification_t;

/* Event subscription: */
typedef elvin_subscription_t event_subscription_t;

/* Event notification callback function.  Passed to event_subscribe
   and called whenever the specified event is triggered.  HANDLE
   is the handle to the event server, NOTIFICATION is the event
   notification itself, and DATA is an arbitrary value passed to
   event_subscribe (argument 4). */
typedef void (*event_notify_callback_t)(event_handle_t handle,
                                        event_notification_t notification,
                                        void *data);

/* Supported event types: */
typedef enum {
    EVENT_TEST,
    EVENT_TRAFGEN_START,
    EVENT_TRAFGEN_STOP
} event_type_t;

/*
 * Event notification attribute definitions:
 */

/* Attribute types: */
typedef enum {
    EVENT_ATTR_DOUBLE,
    EVENT_ATTR_INT32,
    EVENT_ATTR_INT64,
    EVENT_ATTR_STRING
} event_attr_type_t;

/* Attribute values: */
typedef elvin_value_t event_attr_value_t;

/* The "any host" string: */
#define EVENT_HOST_ANY "*"

/*
 * Debugging and tracing definitions:
 */

#define ERROR(fmt,...) fprintf(stderr, __FUNCTION__ ": " fmt, ## __VA_ARGS__)
#ifdef DEBUG
#define TRACE(fmt,...) printf(__FUNCTION__ ": " fmt, ## __VA_ARGS__)
#else
#define TRACE(fmt,...)
#endif /* DEBUG */

/*
 * Function prototypes:
 */

/* event.c */
event_handle_t event_register(void);
int event_unregister(event_handle_t handle);
int event_main(event_handle_t handle);
int event_notify(event_handle_t handle, event_notification_t notification);
event_notification_t event_notification_alloc(event_handle_t handle,
                                              char *host, event_type_t type);
int event_notification_free(event_handle_t handle,
                            event_notification_t notification);
int event_notification_attr_get(event_handle_t handle,
                                event_notification_t notification,
                                event_attr_type_t type, char *name,
                                event_attr_value_t *value);
int event_notification_attr_put(event_handle_t handle,
                                event_notification_t notification,
                                event_attr_type_t type, char *name,
                                event_attr_value_t value);
event_subscription_t event_subscribe(event_handle_t handle,
                                     event_notify_callback_t callback,
                                     event_type_t type, void *data);

/* util.c */
void *xmalloc(int size);
void *xrealloc(void *p, int size);

#endif /* __EVENT_H__ */
