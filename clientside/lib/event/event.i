/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * event.i - SWIG interface file for the Testbed event systems
 */
%module event
%{
#define NO_EVENT_MACROS
#include "event.h"
%}

/*
 * Rename the C event_subscribe(), event_poll(), and event_poll_blocking(), so
 * that we can replace these with perl functions of the same name.
 */
%rename event_subscribe c_event_subscribe;
%rename event_poll c_event_poll;
%rename event_poll_blocking c_event_poll_blocking;

/*
 * We have to replace this one, because it works in a way very foreign to
 * perl. We write our own version below.
 */
%rename event_notification_get_string c_event_notification_get_string;

/*
 * Prevent anyone from using this function
 */
%rename event_main dont_use_this_function_because_it_does_not_work;

/*
 * Simply allow access to everything in event.h
 *
 * NOTE: not all event functions have been well-tested in perl!!!
 */
%echo "*** Warnings about function pointers can be ignored"
%include "event.h"

struct timeval {
  long tv_sec;
  long tv_usec;
};

/*
 * Glue needed to support callbacks under perl
 */
%inline %{

	/*
	 * Queue of notifications that have been received
	 */
	struct callback_data {
		event_notification_t callback_notification;
		struct callback_data *next;
	};
	typedef struct callback_data *callback_data_t;

	callback_data_t callback_data_list;

	/*
	 * Simple wrappers, since we don't want to (maybe can't) call
	 * malloc/free from perl
	 */
	callback_data_t allocate_callback_data() {
		return (callback_data_t)malloc(sizeof(callback_data_t));
	}

	void free_callback_data(callback_data_t data) {
		free(data);
	}

	callback_data_t dequeue_callback_data() {
		callback_data_t data = callback_data_list;
		if (callback_data_list) {
			callback_data_list = callback_data_list->next;
		}
		return data;
	}

	void enqueue_callback_data(callback_data_t data) {
		callback_data_t *pos = &callback_data_list;
		while (*pos) {
			pos = &((*pos)->next);
		}
		*pos = data;
	}

	/*
	 * Stub callback that simply pushes a new entry onto the list
	 * of data for the perl callback function
	 */
	void perl_stub_callback(event_handle_t handle,
		event_notification_t notification, void *data) {
		callback_data_t new_data;

		new_data = allocate_callback_data();
		new_data->callback_notification =
			event_notification_clone(handle,notification);
		new_data->next = NULL;

		enqueue_callback_data(new_data);
	}

	/*
	 * Call event_subscribe using the stub function
	 */
	event_subscription_t stub_event_subscribe(event_handle_t handle,
			address_tuple_t tuple) {
		return event_subscribe(handle,perl_stub_callback,tuple,NULL);

       }
%}

/*
 * Wrappers for return-argument functions
 */

%rename perl_event_notification_get_string event_notification_get_string;

%inline %{

	/*
	 * We use a global buffer to avoid leaking memory - SWIG/XS/perl
	 * will take the return value of char* functions and convert them 
	 * to perl strings, which makes a copy of the string. So, if we
	 * return malloc()ed memory, it gets leaked. But, the event system
	 * requires us to pass in a buffer to fill....
	 */
	char event_string_buffer[1024];

	char *perl_event_notification_get_string(event_handle_t handle,
			event_notification_t notification, char *name) {

		int rv;

		rv = event_notification_get_string(handle,notification,name,
			event_string_buffer,sizeof(event_string_buffer));
		if (!rv) {
			return NULL;
		} else {
			return event_string_buffer;
		}
      }
%}

/*
 * 'macros' - Since SWIG doesn't grok macros, these are here to get the
 * same functionality (we could do this in event.h, but I didn't want to
 * rock the boat too much.) Also, we get a chance to put them in a more
 * perl-friendly format.
 */
%inline %{

	char *event_notification_get_site(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,"SITE");
	}

	char* event_notification_get_expt(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,"EXPT");
	}

	char* event_notification_get_group(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,"GROUP");
	}

	char *event_notification_get_host(event_handle_t handle,
			event_notification_t note) {
        	return perl_event_notification_get_string(handle,note,"HOST");
	}

	char *event_notification_get_objtype(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,
							  "OBJTYPE");
	}

	char *event_notification_get_objname(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,
							  "OBJNAME");
	}

	char* event_notification_get_eventtype(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,
							  "EVENTTYPE");
	}

	char* event_notification_get_arguments(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,"ARGS");
	}

	int event_notification_set_arguments(event_handle_t handle,
			event_notification_t note, char *buf) {
		return event_notification_put_string(handle,note,"ARGS", buf);
	}

	char *event_notification_get_sender(event_handle_t handle,
			event_notification_t note) {
		return perl_event_notification_get_string(handle,note,
	    						  "___SENDER___");
	}

	int event_notification_set_sender(event_handle_t handle,
			event_notification_t note, char *buf) {
		return event_notification_put_string(handle,note,
						     "___SENDER___",buf);
	}
%}
