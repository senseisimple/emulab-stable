This is the documentation for the Perl API to the event system...

Theoretically, most event library calls should 'just work.' See the lists below
to see which have been tested, and which are known not to work. SWIG does not
grok macros -- some have been 'translated' into regular functions, but not all.

Note that you don't get any magic perl garbage collection, etc, for C objects.
So, you still have to call address_tuple_alloc/address_tuple_free, etc.

Also, SWIG modules do not export functions. This means that you have to either
prefix everthing with 'event::' (cumbersome, since most event system stuff
starts with 'event_' already), or cheat, by putting yourself in the module's
namespace with the line 'package event;'

Examples of the use of the perl event system are in the examples/ directory,
and mirror the C examples.

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
