/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * event.c --
 *
 *      Testbed event library.  Currently uses the Elvin publish/
 *      subscribe system for routing event notifications.
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "event.h"
#include "log.h"

#define ERROR(fmt,...) error(__FUNCTION__ ": " fmt, ## __VA_ARGS__)
#ifdef  DEBUG
#define TRACE(fmt,...) info(__FUNCTION__ ": " fmt, ## __VA_ARGS__)
#else
#define TRACE(fmt,...)
#endif

static char hostname[MAXHOSTNAMELEN];
static char ipaddr[32];

/*
 * Register with the testbed event system.  NAME specifies the name of
 * the event server.  Returns a pointer to a handle that may be passed
 * to other event system routines if the operation is successful, NULL
 * otherwise.
 *
 * The THREADED parameter should be set to 1 if the registering
 * client is multi-threaded. If THREADED is 1, the event
 * library will call routines that are thread-safe, and event
 * notifications will be dispatched using background threads (i.e.,
 * the client will supply its own event loop). If THREADED is 0, event
 * notifications will be dispatched using an event system-provided
 * event loop, and the client must call event_main after connecting in
 * order to receive notifications.
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
event_register(char *name, int threaded)
{
    event_handle_t	handle;
    elvin_handle_t	server;
    elvin_error_t	status;
    struct hostent     *he;
    struct in_addr	myip;

    if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
        ERROR("could not get hostname: %s\n", strerror(errno));
        return 0;
    }

    /*
     * Get our IP address. Thats how we name ourselves to the
     * Testbed Event System. 
     */
    if (! (he = gethostbyname(hostname))) {
	ERROR("could not get IP address from hostname: %s", hostname);
    }
    memcpy((char *)&myip, he->h_addr, he->h_length);
    strcpy(ipaddr, inet_ntoa(myip));

    TRACE("registering with event system (hostname=\"%s\")\n", hostname);

    /* Allocate a handle to be returned to the caller: */
    handle = xmalloc(sizeof(*handle));

    /* Set up the Elvin interface pointers: */
    if (threaded) {
#ifdef THREADED
        handle->init = elvin_threaded_init_default;
        handle->connect = elvin_threaded_connect;
        handle->disconnect = elvin_threaded_disconnect;
        handle->cleanup = elvin_threaded_cleanup;
        handle->mainloop = NULL; /* no mainloop for mt programs */
        handle->notify = elvin_threaded_notify;
        handle->subscribe = elvin_threaded_add_subscription;
#else
	ERROR("Threaded API not linked in with the program!\n");
	free(handle);
	return 0;
#endif
    } else {
        handle->init = elvin_sync_init_default;
        handle->connect = elvin_sync_connect;
        handle->disconnect = elvin_sync_disconnect;
        handle->cleanup = elvin_sync_cleanup;
        handle->mainloop = elvin_sync_default_mainloop;
        handle->notify = elvin_sync_notify;
        handle->subscribe = elvin_sync_add_subscription;
    }

    /* Initialize the elvin interface: */

    status = handle->init();
    if (status == NULL) {
        ERROR("could not initialize Elvin\n");
        free(handle);
        return 0;
    }
    server = elvin_handle_alloc(status);
    if (server == NULL) {
        ERROR("elvin_handle_alloc failed: ");
        elvin_error_fprintf(stderr, status);
        free(handle);
        return 0;
    }

    /* Set the discovery scope to "testbed", so that we only interact
       with testbed elvin servers. */
    if (elvin_handle_set_discovery_scope(server, "testbed", status) == 0) {
        ERROR("elvin_handle_set_discovery_scope failed: ");
        elvin_error_fprintf(stderr, status);
        free(handle);
        return 0;
    }

    /* Set the server URL, if we were passed one by the user. */
    if (name) {
        if (elvin_handle_append_url(server, name, status) == 0) {
            ERROR("elvin_handle_append_url failed: ");
            elvin_error_fprintf(stderr, status);
            free(handle);
            return 0;
        }
    }

    /* Connect to the elvin server: */
    if (handle->connect(server, status) == 0) {
        ERROR("could not connect to Elvin server: ");
        elvin_error_fprintf(stderr, status);
        free(handle);
        return 0;
    }

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
    if (handle->disconnect(handle->server, handle->status) == 0) {
        ERROR("could not disconnect from Elvin server: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    /* Clean up: */

    if (elvin_handle_free(handle->server, handle->status) == 0) {
        ERROR("elvin_handle_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }
    if (handle->cleanup(1, handle->status) == 0) {
        ERROR("could not clean up Elvin state: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    free(handle);

    return 1;
}


/*
 * An internal function to handle the two different event_poll calls, without
 * making the library user mess around with arguments they don't care about.
 */

int
internal_event_poll(event_handle_t handle, int blocking, unsigned int timeout)
{
	extern int depth;
	int rv;
	elvin_timeout_t elvin_timeout = NULL;

	if (!handle->mainloop) {
		ERROR("multithreaded programs cannot use event_poll\n");
		return 0;
	}

	/*
	 * If the user wants a timeout, set up an elvin timeout now. We just
	 * use a NULL callback, so that it simply causes a timeout, and doesn't
	 * actually do anything.
	 */
	if (timeout) {
		elvin_timeout = elvin_sync_add_timeout(NULL, timeout, NULL,
				NULL, handle->status);
		if (!elvin_timeout) {
			ERROR("Elvin elvin_sync_add_timeout failed\n");
			elvin_error_fprintf(stderr, handle->status);
			return elvin_error_get_code(handle->status);
		}
	}

	depth++;
	rv = elvin_sync_default_select_and_dispatch(blocking, handle->status);
	depth--;
	if (rv == 0) {
		ERROR("Elvin select_and_dispatch failed\n");
		elvin_error_fprintf(stderr, handle->status);
	}

	/*
	 * Try to remove the timeout - if it didn't go off, we don't want to
	 * hit it later. We don't check the return value, since, if it did go
	 * off (and we don't really have a good way of knowing that), it's not
	 * there any more, so it looks like an error.
	 */
	if (timeout && elvin_timeout) {
		elvin_error_t error;
		elvin_sync_remove_timeout(elvin_timeout, error);
	}

	return elvin_error_get_code(handle->status);
}

/*
 * A non-blocking poll of the event system
 */

int
event_poll(event_handle_t handle)
{
	return internal_event_poll(handle,0,0);
}

/*
 * A blocking poll of the event system, with an optional timeout
 */

int event_poll_blocking(event_handle_t handle, unsigned int timeout)
{
	return internal_event_poll(handle,1,timeout);
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

    if (!handle->mainloop) {
        ERROR("multithreaded programs don't need to call event_main\n");
        return 0;
    }

    TRACE("entering event loop...\n");

    if (handle->mainloop(&loop, handle->status) == 0) {
        ERROR("Elvin mainloop failed: ");
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
 *
 * Note that NOTIFICATION is not deallocated by event_notify.  The
 * caller is responsible for deallocating the notification when it
 * is finished with it.
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
    if (handle->notify(handle->server, notification, 1, NULL, handle->status)
        == 0)
    {
        ERROR("could not send event notification: ");
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
 *
 * Note that NOTIFICATION is not deallocated by event_schedule.
 * The caller is responsible for deallocating the notification
 * when it is finished with it.
 */

int
event_schedule(event_handle_t handle, event_notification_t notification,
               struct timeval *time)
{
    if (!handle || !notification || !time) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("scheduling event notification %p to be sent at time (%ld, %ld)\n",
          notification, time->tv_sec, time->tv_usec);

    /*
     * Add an attribute that signifies its a scheduler operation.
     */
    if (! event_notification_remove(handle, notification, "SCHEDULER") ||
	! event_notification_put_int32(handle, notification, "SCHEDULER", 1)) {
	ERROR("could not add scheduler attribute to notification %p\n",
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
 * Allocate an event notification.  The address TUPLE defines who
 * should receive the notification. Returns a pointer to an event
 * notification structure if the operation is successful, 0 otherwise.
 */

event_notification_t
event_notification_alloc(event_handle_t handle, address_tuple_t tuple)
{
    elvin_notification_t notification;

    if (!handle || !tuple) {
        ERROR("invalid paramater\n");
        return NULL;
    }

    TRACE("allocating notification (tuple=%p)\n", tuple);

    notification = elvin_notification_alloc(handle->status);
    if (notification == NULL) {
        ERROR("elvin_notification_alloc failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }

    TRACE("allocated notification %p\n", notification);
#define EVPUT(name, field) \
({ \
	char *foo = (tuple->field ? tuple->field : ADDRESSTUPLE_ALL); \
	\
	event_notification_put_string(handle, notification, name, foo); \
})
    
    /* Add the target address stuff to the notification */
    if (!EVPUT("SITE", site) ||
	!EVPUT("EXPT", expt) ||
	!EVPUT("GROUP", group) ||
	!EVPUT("HOST", host) ||
	!EVPUT("OBJTYPE", objtype) ||
	!EVPUT("OBJNAME", objname) ||
	!EVPUT("EVENTTYPE", eventtype) ||
	! event_notification_put_int32(handle, notification, "SCHEDULER", 0)) {
	ERROR("could not add attributes to notification %p\n", notification);
        return NULL;
    }

    /* Add our address */
    event_notification_set_sender(handle, notification, ipaddr);

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

/*
 * Clones (copies) the event notificaion. Returns the copy if successful,
 * or NULL if it is not.
 */
event_notification_t
event_notification_clone(event_handle_t handle,
			 event_notification_t notification)
{
    event_notification_t clone;

    if (!handle || !notification) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("cloning notification %p\n", notification);

    if (! (clone = elvin_notification_clone(notification, handle->status)) ) {
        ERROR("elvin_notification_clone failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return clone;

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
	buffer[0] = '\0';
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
 *                   void *data);
 *
 * where HANDLE is the handle to the event server, NOTIFICATION is the
 * event notification, and DATA is the arbitrary pointer passed to
 * event_subscribe.  Returns a pointer to an event
 * subscription structure if the operation is successful, 0 otherwise.
 */

/*
 * This routine takes a "FOO,BAR" string and breaks it up into
 * separate (TAG==FOO || TAG==BAR) clauses.
 */
static int
addclause(char *tag, char *clause, char *exp, int size, int *index)
{
	int	count = 0;
	char	*bp;
	char    clausecopy[BUFSIZ], *strp = clausecopy;
	char	buf[BUFSIZ];
	int     needglob = 1;

	/* Must copy clause since we use strsep! */
	if (strlen(clause) >= sizeof(clausecopy)-1)
		goto bad;
	strcpy(clausecopy, clause);

	/*
	 * Build expression of "or" statements.
	 */
	while (strp && count < sizeof(buf)) {
		bp = strsep(&strp, " ,");

		/* Empty token (two delimiters next to each other) */
		if (! *bp)
			continue;

		if (! strcmp("*", bp))
			needglob = 0;
		
		count += snprintf(&buf[count], sizeof(buf) - count,
				  "%s %s == \"%s\" ",
				  (count ? "||" : ""), tag, bp);
	}
	if (strp || count >= sizeof(buf))
		goto bad;
#if 0
	if (needglob) {
		count += snprintf(&buf[count], sizeof(buf) - count,
				  "%s %s == \"*\" ",
				  (count ? "||" : ""), tag);

		if (count >= size)
			goto bad;
	}
#endif
	/*
	 * And wrap in parens (add an "and" if not the first clause).
	 */
	count = snprintf(exp, size, "%s (%s) ", (*index ? "&&" : ""), buf);
	if (count >= size)
		goto bad;
	
	*index += count;
	return 1;
 bad:
	ERROR("Ran out of room for subscription clause: %s %s\n", tag, clause);
	return 0;	
}

event_subscription_t
event_subscribe(event_handle_t handle, event_notify_callback_t callback,
		address_tuple_t tuple, void *data)
{
    elvin_subscription_t subscription;
    struct notify_callback_arg *arg;
    char expression[EXPRESSION_LENGTH];
    int index = 0;

    /* XXX: The declaration of expression has to go last, or the
       local variables on the stack after it get smashed.  Check
       Elvin for buffer overruns. */

    if (!handle || !callback || !tuple) {
        ERROR("invalid parameter\n");
        return NULL;
    }

    if (tuple->site &&
	! addclause("SITE", tuple->site,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;

    if (tuple->expt &&
	! addclause("EXPT", tuple->expt,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;

    if (tuple->group &&
	! addclause("GROUP", tuple->group,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;

    if (tuple->host &&
	! addclause("HOST", tuple->host,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;
	
    if (tuple->objtype &&
	! addclause("OBJTYPE", tuple->objtype,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;

    if (tuple->objname &&
	! addclause("OBJNAME", tuple->objname,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;

    if (tuple->eventtype &&
	! addclause("EVENTTYPE", tuple->eventtype,
		    &expression[index], sizeof(expression) - index, &index))
	    return NULL;
	    
    index += snprintf(&expression[index], sizeof(expression) - index,
		     "%s SCHEDULER == %d ",
		     (index ? "&&" : ""),
		     tuple->scheduler);

    TRACE("subscribing to event %s\n", expression);

    arg = xmalloc(sizeof(*arg));
    /* XXX: Free this in an event_unsubscribe.. */
    arg->callback = callback;
    arg->data = data;

    subscription = handle->subscribe(handle->server, expression, NULL, 1,
                                     notify_callback, arg, handle->status);
    if (subscription == NULL) {
        ERROR("could not subscribe to event %s: ", expression);
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
 * Callback passed to handle->subscribe in event_subscribe. Used to
 * provide our own callback above Elvin's.
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

    TRACE("received event notification\n");

    assert(arg);
    callback = arg->callback;
    data = arg->data;
    handle.server = server;
    handle.status = status;

    callback(&handle, notification, data);
}

/*
 * address tuple alloc and free.
 */
address_tuple_t
address_tuple_alloc(void)
{
	address_tuple_t	tuple = xmalloc(sizeof(address_tuple));

	bzero(tuple, sizeof(address_tuple));

	return tuple;
}

int
address_tuple_free(address_tuple_t tuple)
{
	free(tuple);
	return 1;
}
