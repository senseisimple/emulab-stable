/*
 * event.i - SWIG interface file for the Testbed event systems
 */
%module event
%{
#define NO_EVENT_MACROS
#include "event.h"
%}

/*
 * Rename the C event_subscribe() and event_poll(), so that we can replace
 * these with perl functions of the same name.
 */
%rename event_subscribe c_event_subscribe;
%rename event_poll c_event_poll;

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

/*
 * Glue needed to support callbacks under perl
 */
%inline %{
	/*
	 * Set to 1 by the stub callback below if a notification is ready to be
	 * processed by a callback.
	 */
	int callback_ready;

	/*
	 * Set when callback_ready is also set
	 */
	event_notification_t callback_notification;

	/*
	 * Stub callback that simply sets callback_ready and
	 * callback_notification
	 */
	void perl_stub_callback(event_handle_t handle,
		event_notification_t notification, void *data) {
		callback_ready = 1;
		callback_notification = event_notification_clone(handle,
			notification);
		if (!callback_notification) {
			/*
			 * event_notification_clone will have already reported
			 * an error message, so we don't have to again
			 */
			callback_ready = 0;
		}
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
