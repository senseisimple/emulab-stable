/*
 * event.h --
 *
 *      Definitions for the testbed event library.
 *
 * @COPYRIGHT@
 *
 * $Id: event.h,v 1.6 2002-02-19 15:45:24 imurdock Exp $
 */

#ifndef __EVENT_H__
#define __EVENT_H__

#include <stdio.h>
#include <elvin/elvin.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif /* MAXHOSTNAMELEN */

/* Handle to the event server: */
struct event_handle {
    elvin_handle_t server;
    elvin_error_t status;
    /* API function pointers: */
    elvin_error_t (*init)(void);
    int (*connect)(elvin_handle_t handle, elvin_error_t error);
    int (*disconnect)(elvin_handle_t handle, elvin_error_t error);
    int (*cleanup)(int force, elvin_error_t error);
    int (*mainloop)(int *do_loop, elvin_error_t error);
    int (*notify)(elvin_handle_t handle, elvin_notification_t notification,
                  int deliver_insecure, elvin_keys_t keys,
                  elvin_error_t error);
    elvin_subscription_t (*subscribe)(elvin_handle_t handle, char *sub_exp,
                                      elvin_keys_t keys, int accept_insecure,
                                      elvin_notify_cb_t callback, void *rock,
                                      elvin_error_t error);
};
typedef struct event_handle * event_handle_t;

/* Event notification: */
typedef elvin_notification_t event_notification_t;

/* Event subscription: */
typedef elvin_subscription_t event_subscription_t;

/* The "any host" string: */
#define EVENT_HOST_ANY "*"

/* Supported event types: */
typedef enum {
    EVENT_NULL,
    EVENT_TEST,
    EVENT_SCHEDULE,
    EVENT_TRAFGEN_START,
    EVENT_TRAFGEN_STOP
} event_type_t;

/* Event notification callback function.  Passed to event_subscribe
   and called whenever the specified event is triggered.
   HANDLE is the handle to the event server, NOTIFICATION is the event
   notification itself, and DATA is an arbitrary value passed to
   event_subscribe (argument 4). HOST and TYPE are attributes from the
   event notification. */
typedef void (*event_notify_callback_t)(event_handle_t handle,
                                        event_notification_t notification,
                                        char *host, event_type_t type,
                                        void *data);

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
event_handle_t event_register(char *name, int threaded);
int event_unregister(event_handle_t handle);
int event_main(event_handle_t handle);
int event_notify(event_handle_t handle, event_notification_t notification);
int event_schedule(event_handle_t handle, event_notification_t notification,
                   struct timeval *time);
event_notification_t event_notification_alloc(event_handle_t handle,
                                              char *host, event_type_t type);
int event_notification_free(event_handle_t handle,
                            event_notification_t notification);
int event_notification_get_double(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, double *value);
int event_notification_get_int32(event_handle_t handle,
                                 event_notification_t notification,
                                 char *name, int32_t *value);
int event_notification_get_int64(event_handle_t handle,
                                 event_notification_t notification,
                                 char *name, int64_t *value);
int event_notification_get_opaque(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, void *buffer, int length);
int event_notification_get_string(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, char *buffer, int length);
int event_notification_put_double(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, double value);
int event_notification_put_int32(event_handle_t handle,
                                 event_notification_t notification,
                                 char *name, int32_t value);
int event_notification_put_int64(event_handle_t handle,
                                 event_notification_t notification,
                                 char *name, int64_t value);
int event_notification_put_opaque(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, void *buffer, int length);
int event_notification_put_string(event_handle_t handle,
                                  event_notification_t notification,
                                  char *name, char *value);
int event_notification_remove(event_handle_t handle,
                              event_notification_t notification, char *name);
event_subscription_t event_subscribe(event_handle_t handle,
                                     event_notify_callback_t callback,
                                     event_type_t type, void *data);

/* util.c */
void *xmalloc(int size);
void *xrealloc(void *p, int size);

#endif /* __EVENT_H__ */
