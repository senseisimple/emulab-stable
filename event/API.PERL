#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#

This is the documentation for the Perl API to the event system...

Theoretically, most event library calls should 'just work.' See the lists below
to see which have been tested, and which are known not to work. SWIG does not
grok macros -- some have been 'translated' into regular functions, but not all.

Note that you don't get any magic perl garbage collection, etc, for C objects.
So, you still have to call address_tuple_alloc/address_tuple_free, etc.

Examples of the use of the perl event system are in the examples/ directory,
and mirror the C examples.

The perl API adds a few new functions, intended to be similar to the DBQuery*
functions from libdb . They are _only_ intended as a quick-and-dirty way to
send events from programs whose primary purpose is _not_ dealing with the event
system. This is because they reconnect to the event server on every event, and
can only pass data through the address tuple. These functions get called like
so:
EventSendFatal(objtype => 'TBEXAMPLE',
               eventtype = 'FOO',
	       host => $event::ADRESSTUPLE_ALL);
The functions (names should be self-explanatory) are:
EventSendFatal
EventSendWarn
EventSend

The following functions have been tested, and the same as their C counterparts:
address_tuple_alloc
address_tuple_free
event_notification_alloc
event_notification_clone
event_notification_free
event_notify
event_unregister
event_subscribe
event_poll
event_notification_put_string (put_* should work)

The following functions have changed slightly, or have caveats:
event_register - Works fine, but DO NOT pass anything but 0 as the second
	argument (no threading support)
event_notification_get_string - Rather than taking a string reference and 
	length, just returns a string. Returns undef if it can't find the
	string in the notification. This also goes for all macros based on
	this function

The following functions do NOT work:
event_main - Only polling is avialable at this point, due to the glue we have
	to use to get callbacks to work
event_notification_get_* (except string) - Return arguments are somewhat
	problematic. Shouldn't be too hard to implement, but there seemed no
	immedeate need (since strings are the bits we use the most.)
