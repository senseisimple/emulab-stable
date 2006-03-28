/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
#include <sys/param.h>
#include <time.h>
#include "event.h"

#define ERROR(fmt,...) \
 { fputs(__FUNCTION__,stderr); fprintf(stderr,": " fmt, ## __VA_ARGS__); }
#define INFO(fmt,...) \
 { fputs(__FUNCTION__,stderr); fprintf(stderr,": " fmt, ## __VA_ARGS__); }
#ifdef  DEBUG
#define TRACE(fmt,...) \
 { fputs(__FUNCTION__,stderr); fprintf(stderr,": " fmt, ## __VA_ARGS__); }
#else
#define TRACE(fmt,...)
#endif

static int event_notification_check_hmac(event_handle_t handle,
					  event_notification_t notification);

static char hostname[MAXHOSTNAMELEN];
static char ipaddr[32];

/*
 * Count of how many handles are in use, so that we can avoid cleaning up until
 * the last one is unregistered
 */
static int handles_in_use = 0;

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
	return event_register_withkeydata(name, threaded, NULL, 0);
}

event_handle_t
event_register_withkeyfile(char *name, int threaded, char *keyfile) {
  return event_register_withkeyfile_withretry(name, threaded, keyfile, -1);
}

event_handle_t
event_register_withkeyfile_withretry(char *name, int threaded, 
				     char *keyfile, int retrycount)
{
    /* Grab the key data and stick it into the handle. */
    if (keyfile) {
	FILE		*fp;
	unsigned char   buf[2*BUFSIZ];
	int		cc;

	if ((fp = fopen(keyfile, "r")) == NULL) {
		ERROR("could not open keyfile: %s", keyfile);
		return 0;
	}
	if ((cc = fread(buf, sizeof(char), sizeof(buf), fp)) == 0) {
		ERROR("could not read keyfile: %s", keyfile);
		fclose(fp);
		return 0;
	}
	if (cc == sizeof(buf)) {
		ERROR("keyfile is too big: %s", keyfile);
		fclose(fp);
		return 0;
	}
	fclose(fp);
	return event_register_withkeydata_withretry(name, threaded, 
					  buf, cc, retrycount);
    }
    return event_register_withkeydata_withretry(name, threaded, NULL, 
						0, retrycount);
}

event_handle_t
event_register_withkeydata(char *name, int threaded,
			   unsigned char *keydata, int keylen){
    return event_register_withkeydata_withretry(name, threaded, keydata,
						keylen, -1);

}

event_handle_t
event_register_withkeydata_withretry(char *name, int threaded,
			   unsigned char *keydata, int keylen,
			   int retrycount)
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
    bzero(handle, sizeof(*handle));

    /* Grab the key data and stick it into the handle. */
    if (keydata) {
	handle->keydata = xmalloc(keylen + 1);
	handle->keylen  = keylen;
	memcpy(handle->keydata, keydata, keylen);
	handle->keydata[keylen] = (unsigned char)0;
    }

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
        handle->unsubscribe = elvin_threaded_delete_subscription;
#else
	ERROR("Threaded API not linked in with the program!\n");
	goto bad;
#endif
    } else {
        handle->init = elvin_sync_init_default;
        handle->connect = elvin_sync_connect;
        handle->disconnect = elvin_sync_disconnect;
        handle->cleanup = elvin_sync_cleanup;
        handle->mainloop = elvin_sync_default_mainloop;
        handle->notify = elvin_sync_notify;
        handle->subscribe = elvin_sync_add_subscription;
        handle->unsubscribe = elvin_sync_delete_subscription;
    }

    /* Initialize the elvin interface: */

    status = handle->init();
    if (status == NULL) {
        ERROR("could not initialize Elvin\n");
	goto bad;
    }
    server = elvin_handle_alloc(status);
    if (server == NULL) {
        ERROR("elvin_handle_alloc failed: ");
        elvin_error_fprintf(stderr, status);
	goto bad;
    }

    /* Set the discovery scope to "testbed", so that we only interact
       with testbed elvin servers. */
    if (elvin_handle_set_discovery_scope(server, "testbed", status) == 0) {
        ERROR("elvin_handle_set_discovery_scope failed: ");
        elvin_error_fprintf(stderr, status);
	goto bad;
    }

    /* Set the server URL, if we were passed one by the user. */
    if (name) {
        if (elvin_handle_append_url(server, name, status) == 0) {
            ERROR("elvin_handle_append_url failed: ");
            elvin_error_fprintf(stderr, status);
	    goto bad;
        }
    }

    /* set connection retries */
    if (retrycount >= 0) {
      if (elvin_handle_set_connection_retries(server, retrycount, 
					      status) == 0) {
	ERROR("elvin_handle_set_connection_retries failed: ");
	elvin_error_fprintf(stderr, status);
	goto bad;
      }
    }

    /* Connect to the elvin server: */
    if (handle->connect(server, status) == 0) {
        ERROR("could not connect to Elvin server: ");
        elvin_error_fprintf(stderr, status);
	goto bad;
    }

    handle->server = server;
    handle->status = status;

    /*
     * Keep track of how many handles we have outstanding
     */
    handles_in_use++;
    return handle;

 bad:
    if (handle->keydata)
        free(handle->keydata);
    free(handle);
    return 0;
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

    TRACE("disconnect completed\n");

    /* Clean up: */

    if (elvin_handle_free(handle->server, handle->status) == 0) {
        ERROR("elvin_handle_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    TRACE("free completed\n");

    if (handles_in_use == 1) {
	if (handle->cleanup(1, handle->status) == 0) {
	    ERROR("could not clean up Elvin state: ");
	    elvin_error_fprintf(stderr, handle->status);
	    return 0;
	}

	TRACE("cleanup completed\n");
    }

    handles_in_use--;

    if (handle->keydata)
        free(handle->keydata);
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
    if (!handle) {
        ERROR("invalid parameter\n"); 
        return 0;
    }

    if (!handle->mainloop) {
        ERROR("multithreaded programs don't need to call event_main\n");
        return 0;
    }

    if (handle->do_loop) {
	ERROR("loop is already running\n");
	return 0;
    }

    TRACE("entering event loop...\n");

    handle->do_loop = 1;
    if (handle->mainloop(&handle->do_loop, handle->status) == 0) {
        ERROR("Elvin mainloop failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


int event_stop_main(event_handle_t handle)
{
    if (!handle) {
	ERROR("invalid parameter\n");
	return 0;
    }

    if (!handle->mainloop) {
	ERROR("multithreaded programs do not have a mainloop\n");
	return 0;
    }

    if (!handle->do_loop) {
	ERROR("mainloop is not running\n");
	return 0;
    }

    handle->do_loop = 0;
    
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
    if (handle->keydata && !notification->has_hmac &&
	event_notification_insert_hmac(handle, notification)) {
        return 0;
    }

    TRACE("sending event notification %p\n", notification);

    /* Send notification to Elvin server for routing: */
    if (handle->notify(handle->server, notification->elvin_notification,
		       1, NULL, handle->status)
        == 0) {
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
    event_notification_t notification;
    elvin_notification_t elvin_notification;

    if (!handle) {
        ERROR("invalid paramater\n");
        return NULL;
    }

    TRACE("allocating notification (tuple=%p)\n", tuple);

    notification = xmalloc(sizeof(struct event_notification));
    elvin_notification = elvin_notification_alloc(handle->status);
    if (elvin_notification == NULL) {
        ERROR("elvin_notification_alloc failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return NULL;
    }
    notification->elvin_notification = elvin_notification;
    notification->has_hmac = 0;

    if (tuple == NULL)
	    return notification;

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
	!EVPUT("TIMELINE", timeline) ||
	! event_notification_put_int32(handle, notification, "SCHEDULER", tuple->scheduler)) {
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
    if (!notification) {
	return 1;
    }
    
    if (!handle || !notification->elvin_notification) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("freeing notification %p\n", notification);

    if (elvin_notification_free(notification->elvin_notification,
				handle->status) == 0) {
        ERROR("elvin_notification_free failed: ");
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }
    free(notification);

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

    if (!handle || !notification || !notification->elvin_notification) {
        ERROR("invalid parameter\n");
        return 0;
    }

    TRACE("cloning notification %p\n", notification);

    clone = xmalloc(sizeof(struct event_notification));
    if (! (clone->elvin_notification =
	   elvin_notification_clone(notification->elvin_notification,
				    handle->status))) {
        ERROR("elvin_notification_clone failed: ");
        elvin_error_fprintf(stderr, handle->status);
	free(clone);
        return 0;
    }
    clone->has_hmac = notification->has_hmac;

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
    if (elvin_notification_traverse(notification->elvin_notification,
				    attr_traverse, &arg, handle->status)
        == 0) {
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

    if (elvin_notification_add_real64(notification->elvin_notification,
				      name, value, handle->status)
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

    if (elvin_notification_add_int32(notification->elvin_notification,
				     name, value, handle->status)
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

    if (elvin_notification_add_int64(notification->elvin_notification,
				     name, value, handle->status)
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

    if (elvin_notification_add_opaque(notification->elvin_notification,
				      name, buffer, length, handle->status)
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

    if (elvin_notification_add_string(notification->elvin_notification,
				      name, value, handle->status)
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

    if (elvin_notification_remove(notification->elvin_notification,
				  name, handle->status) == 0) {
        ERROR("elvin_notification_remove of %s failed: ", name);
        elvin_error_fprintf(stderr, handle->status);
        return 0;
    }

    return 1;
}


struct notify_callback_arg {
    event_notify_callback_t callback;
    void *data;
    event_handle_t handle;
    int do_auth;
};

static void notify_callback(elvin_handle_t server,
                            elvin_subscription_t subscription,
                            elvin_notification_t notification, int is_secure,
                            void *rock, elvin_error_t status);

struct subscription_callback_arg {
    event_subscription_callback_t callback;
    void *data;
    event_handle_t handle;
};

static void subscription_callback(elvin_handle_t server,
				  int result,
				  elvin_subscription_t subscription,
				  void *rock,
				  elvin_error_t status);

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

static char *
tuple_expression(address_tuple_t tuple, char *expression, int elen)
{
    char *retval = expression;
    int index = 0;

    if (tuple->site &&
	! addclause("SITE", tuple->site,
		    &expression[index], elen - index, &index))
	    return NULL;

    if (tuple->expt &&
	! addclause("EXPT", tuple->expt,
		    &expression[index], elen - index, &index))
	    return NULL;

    if (tuple->group &&
	! addclause("GROUP", tuple->group,
		    &expression[index], elen - index, &index))
	    return NULL;

    if (tuple->host &&
	! addclause("HOST", tuple->host,
		    &expression[index], elen - index, &index))
	    return NULL;
	
    if (tuple->objtype &&
	! addclause("OBJTYPE", tuple->objtype,
		    &expression[index], elen - index, &index))
	    return NULL;

    if (tuple->objname &&
	! addclause("OBJNAME", tuple->objname,
		    &expression[index], elen - index, &index))
	    return NULL;

    if (tuple->eventtype &&
	! addclause("EVENTTYPE", tuple->eventtype,
		    &expression[index], elen - index, &index))
	    return NULL;
    
    if (tuple->timeline &&
	! addclause("TIMELINE", tuple->timeline,
		    &expression[index], elen - index, &index))
	    return NULL;
    
    index += snprintf(&expression[index], elen - index,
		     "%s SCHEDULER == %d ",
		     (index ? "&&" : ""),
		     tuple->scheduler);

    return retval;
}

event_subscription_t
event_subscribe(event_handle_t handle, event_notify_callback_t callback,
		address_tuple_t tuple, void *data)
{
	return event_subscribe_auth(handle, callback, tuple, data, 1);
}

event_subscription_t
event_subscribe_auth(event_handle_t handle, event_notify_callback_t callback,
		     address_tuple_t tuple, void *data, int do_auth)
{
    elvin_subscription_t subscription;
    struct notify_callback_arg *arg;
    char expression[EXPRESSION_LENGTH];

    /* XXX: The declaration of expression has to go last, or the
       local variables on the stack after it get smashed.  Check
       Elvin for buffer overruns. */

    if (!handle || !callback || !tuple) {
        ERROR("invalid parameter\n");
        return NULL;
    }

    if (tuple_expression(tuple, expression, sizeof(expression)) == NULL)
	return NULL;
    
    TRACE("subscribing to event %s\n", expression);

    arg = xmalloc(sizeof(*arg));
    /* XXX: Free this in an event_unsubscribe.. */
    arg->callback = callback;
    arg->data = data;
    arg->handle = handle;
    arg->do_auth = do_auth;

    subscription = handle->subscribe(handle->server, expression, NULL, 1,
                                     notify_callback, arg, handle->status);
    if (subscription == NULL) {
        ERROR("could not subscribe to event %s: ", expression);
        elvin_error_fprintf(stderr, handle->status);
	free(arg);
        return NULL;
    }

    return subscription;
}

int
event_async_subscribe(event_handle_t handle, event_notify_callback_t callback,
		      address_tuple_t tuple, void *data,
		      event_subscription_callback_t scb, void *scb_data,
		      int do_auth)
{
    struct notify_callback_arg *arg;
    struct subscription_callback_arg *sarg;
    char expression[EXPRESSION_LENGTH];
    int retval;

    /* XXX: The declaration of expression has to go last, or the
       local variables on the stack after it get smashed.  Check
       Elvin for buffer overruns. */

    if (!handle || !callback || !tuple || !scb) {
        ERROR("invalid parameter\n");
        return 0;
    }

    if (tuple_expression(tuple, expression, sizeof(expression)) == NULL)
	return 0;
    
    TRACE("subscribing to event %s\n", expression);

    arg = xmalloc(sizeof(*arg));
    /* XXX: Free this in an event_unsubscribe.. */
    arg->callback = callback;
    arg->data = data;
    arg->handle = handle;
    arg->do_auth = do_auth;

    sarg = xmalloc(sizeof(*arg));
    /* XXX: Free this in an event_unsubscribe.. */
    sarg->callback = scb;
    sarg->data = scb_data;
    sarg->handle = handle;

    retval = elvin_async_add_subscription(handle->server,
					  expression,
					  NULL,
					  1,
					  notify_callback,
					  arg,
					  subscription_callback,
					  sarg,
					  handle->status);
    
    if (retval == 0) {
      free(arg);
      free(sarg);
    }

    return retval;
}

int
event_async_unsubscribe(event_handle_t handle, event_subscription_t es)
{
    int retval;
    
    if (!es) {
      ERROR("invalid parameter\n");
      return 0;
    }

    free(es->rock);
    es->rock = NULL;
    retval = elvin_async_delete_subscription(handle->server,
					     es,
					     NULL,
					     NULL,
					     handle->status);

    return retval;
}

int
event_unsubscribe(event_handle_t handle, event_subscription_t es)
{
    int retval;

    free(es->rock);
    es->rock = NULL;
    retval = handle->unsubscribe(handle->server, es, handle->status);
    
    return retval;
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
                elvin_notification_t elvin_notification, int is_secure,
                void *rock, elvin_error_t status)
{
    struct notify_callback_arg *arg = (struct notify_callback_arg *) rock;
    struct event_notification notification;
    event_handle_t handle;
    event_notify_callback_t callback;
    void *data;

    TRACE("received event notification\n");

    /*
     * If two subscriptions match an incoming event, and the first
     * notified callback _asynchronously_ unsubscribes the second
     * subscription, the second callback may still be called while the
     * unsubscribe operation is underway.  In this case, the
     * unsubscribe operation would have already cleared the second
     * subscription's associated "rock."  So just return now if the
     * rock has been cleared since this subscription is either on it's
     * way out soon, or is otherwise bogus.
     */
    if (arg == NULL) {
      return;
    }

    notification.elvin_notification = elvin_notification;
    notification.has_hmac = 0;
    handle = arg->handle;

    /* If MAC does not match, throw it away */
    if (arg->do_auth &&
	handle->keydata &&
	event_notification_check_hmac(handle, &notification)) {
	    ERROR("bad hmac\n");
        return;
    }

    if (0) {
	    struct timeval now;
	    
	    gettimeofday(&now, NULL);

	    INFO("note arrived at %ld:%ld\n", now.tv_sec, now.tv_usec);
    }
	
    callback = arg->callback;
    data = arg->data;

    callback(handle, &notification, data);
}

/*
 * Callback passed to handle->subscribe in event_subscribe. Used to
 * provide our own callback above Elvin's.
 */
static void
subscription_callback(elvin_handle_t server,
		      int result,
		      elvin_subscription_t subscription,
		      void *rock,
		      elvin_error_t status)
{
    struct subscription_callback_arg *arg =
	(struct subscription_callback_arg *) rock;
    event_handle_t handle;
    event_subscription_callback_t callback;
    void *data;

    TRACE("received subscription notification\n");
    assert(arg);

    handle = arg->handle;
    callback = arg->callback;
    data = arg->data;

    callback(handle, result, subscription, data);

    /* free the sarg allocated in async_subscribe */
    free(arg);
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

/*
 * Insert an HMAC into the notifcation. 
 */
#include <openssl/opensslv.h>
#include <openssl/hmac.h>

/*
 * The traversal function callback. Add to the hmac for each attribute.
 */
static int
hmac_traverse(void *rock, char *name, elvin_basetypes_t type,
              elvin_value_t value, elvin_error_t status)
{
	HMAC_CTX	*ctx = (HMAC_CTX *) rock;

	/*
	 * Do not include hmac in hmac computation!
	 */
	if (!strcmp(name, "__hmac__"))
		return 1;

	switch (type) {
	case ELVIN_INT32:
		HMAC_Update(ctx, (unsigned char *)&(value.i), sizeof(value.i));
		break;
		
	case ELVIN_INT64:
		HMAC_Update(ctx, (unsigned char *)&(value.h), sizeof(value.h));
		break;
		
	case ELVIN_REAL64:
		HMAC_Update(ctx, (unsigned char *)&(value.d), sizeof(value.d));
		break;
		
	case ELVIN_STRING:
		HMAC_Update(ctx, (unsigned char *)(value.s), strlen(value.s));
		break;
		
	case ELVIN_OPAQUE:
		HMAC_Update(ctx, (unsigned char *)(value.o.data),
			    value.o.length);
		break;

	default:
		ERROR("invalid parameter\n");
		return 0;
	}
	return 1;
}

int
event_notification_insert_hmac(event_handle_t handle,
			       event_notification_t notification)
{
	HMAC_CTX	ctx;
	unsigned char	mac[EVP_MAX_MD_SIZE];
	int		i, len = EVP_MAX_MD_SIZE;

	if (0)
	    INFO("event_notification_insert_hmac: %d %s\n",
		 handle->keylen, handle->keydata);

	if (notification->has_hmac) {
		event_notification_remove(handle, notification, "__hmac__");
		notification->has_hmac = 0;
	}

	memset(&ctx, 0, sizeof(ctx));
#if (OPENSSL_VERSION_NUMBER < 0x0090703f)
	HMAC_Init(&ctx, handle->keydata, handle->keylen, EVP_sha1());
#else	
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, handle->keydata, handle->keylen, EVP_sha1(), NULL);
#endif
	if (!elvin_notification_traverse(notification->elvin_notification,
				 hmac_traverse, &ctx, handle->status)) {
		HMAC_cleanup(&ctx);
		return 1;
	}
	HMAC_Final(&ctx, mac, &len);
	HMAC_cleanup(&ctx);

	if (0) {
		INFO("event_notification_insert_hmac: %d\n", len);
		for (i = 0; i < len; i += 4) {
			INFO("%x", *((unsigned int *)(&mac[i])));
		}
		INFO("\n");
	}

	/*
	 * Okay, now insert the MAC into the notification as an opaque field.
	 */
	if (!elvin_notification_add_opaque(notification->elvin_notification,
				   "__hmac__", mac, len, handle->status)) {
		ERROR("elvin_notification_add_opaque failed: ");
		elvin_error_fprintf(stderr, handle->status);
		return 1;
	}
	notification->has_hmac = 1;
    	return 0;
}

/*
 * Check HMAC. Return 0 if equal, 1 if they are not, -1 if fatal error.
 */
static int
event_notification_check_hmac(event_handle_t handle,
			      event_notification_t notification)
{
	HMAC_CTX	ctx;
	unsigned char	srcmac[EVP_MAX_MD_SIZE], mac[EVP_MAX_MD_SIZE];
	int		i, srclen, len = EVP_MAX_MD_SIZE;
	elvin_value_t	value;
	elvin_basetypes_t type;

	if (0)
	    INFO("event_notification_check_hmac: %d %s\n",
		 handle->keylen, handle->keydata);
		
	/*
	 * Pull out the MAC from the notification so we can compare it.
	 */
	if (!elvin_notification_get(notification->elvin_notification,
			    "__hmac__", &type, &value, handle->status)) {
		ERROR("MAC not present!\n");
		notification->has_hmac = 0;
		return -1;
	}
	srclen = value.o.length;
	assert(srclen <= EVP_MAX_MD_SIZE);
	memcpy(srcmac, (unsigned char *)value.o.data, value.o.length);

	if (0) {
		INFO("event_notification_check_hmac1: %d\n", srclen);
		for (i = 0; i < srclen; i += 4) {
			INFO("%x", *((unsigned int *)(&srcmac[i])));
		}
		INFO("\n");
	}
	
	memset(&ctx, 0, sizeof(ctx));
#if (OPENSSL_VERSION_NUMBER < 0x0090703f)
	HMAC_Init(&ctx, handle->keydata, handle->keylen, EVP_sha1());
#else	
	HMAC_CTX_init(&ctx);
	HMAC_Init_ex(&ctx, handle->keydata, handle->keylen, EVP_sha1(), NULL);
#endif
	
	/* Compute the MAC */
	if (!elvin_notification_traverse(notification->elvin_notification,
				 hmac_traverse, &ctx, handle->status)) {
		HMAC_cleanup(&ctx);
		return -1;
	}
	HMAC_Final(&ctx, mac, &len);
	HMAC_cleanup(&ctx);

	if (0) {
		INFO("event_notification_check_hmac2: %d\n", len);
		for (i = 0; i < len; i += 4) {
			INFO("%x", *((unsigned int *)(&mac[i])));
		}
		INFO("\n");
	}

	if (srclen != len || memcmp(srcmac, mac, len)) {
		ERROR("MAC mismatch!\n");
		return 1;
	}
	notification->has_hmac = 1;
    	return 0;
}

/*
 * Support for packing and unpacking a notification. Packing a notification
 * converts it to something the caller can pass around; a set of three arrays, 
 * types, names, values. Unpacking a notification takes those three arrays
 * and returns a new notification with those contents. For packing, the
 * caller provides three static arrays, and gives us the length of then. We
 * store the contents in those arrays, and return the actual length. The
 * arrays must be big enough ...
 */
struct pack_traverse_arg {
	int		maxlen;
	int		len;
	unsigned char  *data;
};

struct pack_bin {
	short		reclen;
	short		dlen;
	int		type;
	char		name[32];
	unsigned char   data[0];
};

/*
 * The traversal function callback.
 */
static int
pack_traverse(void *rock, char *name, elvin_basetypes_t type,
              elvin_value_t value, elvin_error_t status)
{
	struct pack_traverse_arg *packarg = (struct pack_traverse_arg *) rock;
	struct pack_bin		 *bin;
	int			  dlen = 0;
	unsigned char buf[BUFSIZ];

	bin = (struct pack_bin *) (packarg->data + packarg->len);

	switch (type) {
	case ELVIN_INT32:
		sprintf(buf, "%d", value.i);
		break;
		
	case ELVIN_INT64:
		sprintf(buf, "%lld", value.h);
		break;
		
	case ELVIN_REAL64:
		sprintf(buf, "%f", value.d);
		break;
		
	case ELVIN_STRING:
		if (strlen(value.s) >= BUFSIZ) {
			ERROR("pack_traverse: string too big\n");
			return 0;

		}
		strcpy(buf, value.s);
		break;
		
	case ELVIN_OPAQUE:
		if (value.o.length >= BUFSIZ) {
			ERROR("pack_traverse: opaque too big\n");
			return 0;

		}
		memcpy(buf, (unsigned char *)(value.o.data), value.o.length);
		buf[value.o.length] = (unsigned char) NULL;
		dlen = value.o.length + 1;
		break;

	default:
		ERROR("invalid parameter\n");
		return 0;
	}
	if (!dlen)
		dlen = strlen(buf) + 1;

	/*
	 * Watch for too much stuff.
	 */
	if (packarg->len + (dlen + sizeof(*bin)) >= packarg->maxlen) {
		ERROR("pack_traverse: Not enough room at %s!\n", name);
		return 0;
	}
	/*
	 * XXX Name is bogus. Fix later.
	 */
	if (strlen(name) >= sizeof(bin->name)) {
		ERROR("pack_traverse: Name too long %s!\n", name);
		return 0;
	}
	
	strcpy(bin->name, name);
	bin->type   = type;
	bin->dlen   = dlen;
	bin->reclen = roundup((dlen + sizeof(*bin)), sizeof(long));
	memcpy(bin->data, buf, dlen);
	packarg->len += bin->reclen;

	return 1;
}

/*
 * Extract stuff from inside notification and return.
 */
int
event_notification_pack(event_handle_t handle,
			event_notification_t notification,
			unsigned char *data, int *len)
{
	struct pack_traverse_arg	packarg;

	packarg.maxlen = *len;
	packarg.len    = 0;
	packarg.data   = data;
	
	if (!elvin_notification_traverse(notification->elvin_notification,
				 pack_traverse, &packarg, handle->status)) {
		return 1;
	}
	*len = packarg.len;
	return 0;
}

/*
 * Take raw data and stuff it into a notification. 
 */
int
event_notification_unpack(event_handle_t handle,
			  event_notification_t *notification,
			  unsigned char *data, int len)
{
	event_notification_t newnote = event_notification_alloc(handle, NULL);
	int		     rval, offset = 0;
	elvin_value_t	     value;
	
	if (! newnote)
		return -1;

	while (offset < len) {
		struct pack_bin	*bin = (struct pack_bin *) (data + offset);

		INFO("type: %d %s %s\n", bin->type, bin->name, bin->data);

		switch (bin->type) {
		case ELVIN_INT32:
			sscanf(bin->data, "%d", &(value.i));
			rval = event_notification_put_int32(handle, newnote,
						    bin->name, value.i);
			break;
		
		case ELVIN_INT64:
			sscanf(bin->data, "%lld", &(value.h));
			rval = event_notification_put_int64(handle, newnote,
						    bin->name, value.h);
			break;
		
		case ELVIN_REAL64:
			rval = event_notification_put_double(handle, newnote,
						    bin->name, value.d);
			sscanf(bin->data, "%lf", &(value.d));
			break;
		
		case ELVIN_STRING:
			rval = event_notification_put_string(handle, newnote,
						     bin->name, bin->data);
			break;
		
		case ELVIN_OPAQUE:
			rval = event_notification_put_opaque(handle, newnote,
						     bin->name, bin->data,
						     bin->dlen);
			break;
			
		default:
			ERROR("event_notification_unpack: invalid type\n");
			return 0;
		}
		if (!rval) {
			ERROR("event_notification_unpack: insert failed\n");
			return 0;
		}
		offset += bin->reclen;
	}
	*notification = newnote;
	return 0;
}

static char *match_quote(char *str)
{
	char *retval = NULL;
	int count;
	
	assert(str != NULL);
	assert(str[0] == '{');
	
	for (count = 1, str++; (count > 0) && *str; str++) {
		switch (*str) {
		case '{':
			count += 1;
			break;
		case '}':
			count -= 1;
			break;
		default:
			break;
		}
	}
	
	if (count == 0)
		retval = str - 1;
	
	return retval;
}

int event_arg_get(char *args, char *key, char **value_out)
{
	static char *WHITESPACE = " \t";
	
	int retval = -1;

	*value_out = NULL;
	args += strspn(args, WHITESPACE);
	while( (args[0] != '\0') && (retval < 0) ) {
		char *key_end;

		if (strlen(args) == 0) {
		}
		else if ((key_end = strchr(args, '=')) == NULL) {
			errno = EINVAL;
			break;
		}
		else if ((strlen(key) != (key_end-args)) || (strncasecmp(args, key, (key_end - args)) != 0)) {
			errno = ESRCH;
		}
		else if (key_end[1] == '{') {
			char *value_end;
			
			if ((value_end = match_quote(&key_end[1])) == NULL) {
				errno = EINVAL;
				break;
			}
			else {
				*value_out = &key_end[2];
				retval = (value_end - *value_out);
			}
		}
		else if (key_end[1] == '\'') {
			char *value_end;

			*value_out = &key_end[2];
			if ((value_end = strchr(*value_out, '\'')) == NULL) {
				errno = EINVAL;
				break;
			}
			else {
				retval = (value_end - *value_out);
			}
		}
		else {
			*value_out = &key_end[1];
			retval = strcspn(*value_out, WHITESPACE);
		}
		
		args += strcspn(args, WHITESPACE);
		args += strspn(args, WHITESPACE);
	}
	
	return retval;
}

int event_arg_dup(char *args, char *key, char **value_out)
{
	char *value;
	int retval;

	*value_out = NULL;
	if ((retval = event_arg_get(args, key, &value)) >= 0) {
		if ((*value_out = malloc(retval + 1)) != NULL) {
			strncpy(*value_out, value, retval);
			(*value_out)[retval] = '\0';
		}
		else {
			retval = -1;
			errno = ENOMEM;
		}
	}
	return retval;
}

event_notification_t event_notification_create_v(event_handle_t handle,
						 struct timeval **when_out,
						 ea_tag_t tag,
						 va_list args)
{
	event_notification_t retval = NULL;
	struct timeval *when = NULL;
	address_tuple_t tuple;

	if ((tuple = address_tuple_alloc()) == NULL) {
		ERROR("could not allocate address tuple");
		errno = ENOMEM;
	}
	else {
		char *arg_name, *event_args, *event_args_cursor;
		char event_args_buffer[1024] = "";

		event_args = event_args_buffer;
		event_args_cursor = event_args_buffer;
		
		while (tag != EA_TAG_DONE) {
			switch (tag) {
			case EA_Site:
				tuple->site = va_arg(args, char *);
				break;
			case EA_Experiment:
				tuple->expt = va_arg(args, char *);
				break;
			case EA_Group:
				tuple->group = va_arg(args, char *);
				break;
			case EA_Host:
				tuple->host = va_arg(args, char *);
				break;
			case EA_Type:
				tuple->objtype = va_arg(args, char *);
				break;
			case EA_Name:
				tuple->objname = va_arg(args, char *);
				break;
			case EA_Event:
				tuple->eventtype = va_arg(args, char *);
				break;
			case EA_Arguments:
				event_args = va_arg(args, char *);
				break;
			case EA_ArgInteger:
				arg_name = va_arg(args, char *);
				sprintf(event_args_cursor,
					" %s=%d",
					arg_name,
					va_arg(args, int));
				event_args_cursor += strlen(event_args_cursor);
				break;
			case EA_ArgFloat:
				arg_name = va_arg(args, char *);
				sprintf(event_args_cursor,
					" %s=%f",
					arg_name,
					va_arg(args, double));
				event_args_cursor += strlen(event_args_cursor);
				break;
			case EA_ArgString:
				arg_name = va_arg(args, char *);
				sprintf(event_args_cursor,
					" %s=%s",
					arg_name,
					va_arg(args, char *));
				event_args_cursor += strlen(event_args_cursor);
				break;
			case EA_When:
				when = va_arg(args, struct timeval *);
				break;
			default:
				ERROR("unknown tag value");
				errno = EINVAL;
				return 0;
			}
			tag = va_arg(args, ea_tag_t);
		}
		
		if ((retval = event_notification_alloc(handle,
						       tuple)) == NULL) {
			ERROR("could not allocate notification");
			errno = ENOMEM;
		}
		else {
			if (strlen(event_args) > 0) {
				event_notification_set_arguments(handle,
								 retval,
								 event_args);
			}
		}
		address_tuple_free(tuple);
		tuple = NULL;
	}

	if (when_out != NULL)
		*when_out = when;

	return retval;
}

event_notification_t event_notification_create(event_handle_t handle,
					       ea_tag_t tag,
					       ...)
{
	event_notification_t retval;
	va_list args;

	va_start(args, tag);
	retval = event_notification_create_v(handle, NULL, tag, args);
	va_end(args);
	
	return retval;
}

int event_do_v(event_handle_t handle, ea_tag_t tag, va_list args)
{
	event_notification_t en;
	struct timeval *when;
	int retval = -1;
	
	if ((en = event_notification_create_v(handle, &when,
					      tag, args)) == NULL) {
		ERROR("could not allocate notification");
		errno = ENOMEM;
	}
	else {
		struct timeval tv;
		
		if (when == NULL) {
			when = &tv;
			gettimeofday(when, NULL);
		}
		retval = event_schedule(handle, en, when);
		event_notification_free(handle, en);
		en = NULL;
	}
	
	return retval;
}

int event_do(event_handle_t handle, ea_tag_t tag, ...)
{
	va_list args;
	int retval;

	va_start(args, tag);
	retval = event_do_v(handle, tag, args);
	va_end(args);
	
	return retval;
}


int event_set_idle_period(event_handle_t handle, int seconds) {
  int retval;

  if (!handle) {
    ERROR("invalid parameter\n");
    return 0;
  }

  retval = elvin_handle_set_idle_period(handle->server, seconds,
				     handle->status);
  if (retval == 0) {
    ERROR("could not set elvin idle period to %i", seconds);
    elvin_error_fprintf(stderr, handle->status);
  }

  return retval;

}


int event_set_failover(event_handle_t handle, int dofail) {
  int retval;

  if (!handle) {
    ERROR("invalid parameter\n");
    return 0;
  }

  retval = elvin_handle_set_failover(handle->server, dofail,
				     handle->status);
  if (retval == 0) {
    ERROR("Could not set failover on event handle: ");
    elvin_error_fprintf(stderr, handle->status);
  }

  return retval;

}
