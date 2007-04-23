# This file was created automatically by SWIG 1.3.29.
# Don't modify this file, modify the SWIG interface instead.
package event;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package eventc;
bootstrap event;
package event;
@EXPORT = qw( $MAXHOSTNAMELEN event_handle_server_set event_handle_server_get event_handle_status_set event_handle_status_get event_handle_keydata_set event_handle_keydata_get event_handle_keylen_set event_handle_keylen_get event_handle_do_loop_set event_handle_do_loop_get event_handle_connect_set event_handle_connect_get event_handle_disconnect_set event_handle_disconnect_get event_handle_mainloop_set event_handle_mainloop_get event_handle_notify_set event_handle_notify_get event_handle_subscribe_set event_handle_subscribe_get event_handle_unsubscribe_set event_handle_unsubscribe_get new_event_handle delete_event_handle event_notification_pubsub_notification_set event_notification_pubsub_notification_get event_notification_has_hmac_set event_notification_has_hmac_get new_event_notification delete_event_notification address_tuple_site_set address_tuple_site_get address_tuple_expt_set address_tuple_expt_get address_tuple_group_set address_tuple_group_get address_tuple_host_set address_tuple_host_get address_tuple_objtype_set address_tuple_objtype_get address_tuple_objname_set address_tuple_objname_get address_tuple_eventtype_set address_tuple_eventtype_get address_tuple_scheduler_set address_tuple_scheduler_get address_tuple_timeline_set address_tuple_timeline_get new_address_tuple delete_address_tuple $ADDRESSTUPLE_ALL $OBJECTTYPE_TESTBED $OBJECTTYPE_TRAFGEN address_tuple_alloc address_tuple_free $EVENT_HOST_ANY $EVENT_NULL $EVENT_TEST $EVENT_SCHEDULE $EVENT_TRAFGEN_START $EVENT_TRAFGEN_STOP event_register event_register_withkeyfile event_register_withkeydata event_register_withkeyfile_withretry event_register_withkeydata_withretry event_unregister c_event_poll c_event_poll_blocking dont_use_this_function_because_it_does_not_work event_stop_main event_notify event_schedule event_notification_alloc event_notification_free event_notification_clone event_notification_get_double event_notification_get_int32 event_notification_get_int64 event_notification_get_opaque c_event_notification_get_string event_notification_put_double event_notification_put_int32 event_notification_put_int64 event_notification_put_opaque event_notification_put_string event_notification_remove c_event_subscribe event_subscribe_auth event_async_subscribe event_unsubscribe event_async_unsubscribe event_notification_insert_hmac event_set_idle_period event_set_failover event_arg_get event_arg_dup $EA_TAG_DONE $EA_Site $EA_Experiment $EA_Group $EA_Host $EA_Type $EA_Name $EA_Event $EA_Arguments $EA_ArgInteger $EA_ArgFloat $EA_ArgString $EA_When event_notification_create_v event_notification_create event_do_v event_do xmalloc xrealloc make_timestamp timeval_tv_sec_set timeval_tv_sec_get timeval_tv_usec_set timeval_tv_usec_get new_timeval delete_timeval callback_data_callback_notification_set callback_data_callback_notification_get callback_data_next_set callback_data_next_get new_callback_data delete_callback_data $callback_data_list allocate_callback_data free_callback_data dequeue_callback_data enqueue_callback_data perl_stub_callback stub_event_subscribe $event_string_buffer event_notification_get_string event_notification_get_site event_notification_get_expt event_notification_get_group event_notification_get_host event_notification_get_objtype event_notification_get_objname event_notification_get_eventtype event_notification_get_arguments event_notification_set_arguments event_notification_get_sender event_notification_set_sender );

# ---------- BASE METHODS -------------

package event;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package event;

*address_tuple_alloc = *eventc::address_tuple_alloc;
*address_tuple_free = *eventc::address_tuple_free;
*event_register = *eventc::event_register;
*event_register_withkeyfile = *eventc::event_register_withkeyfile;
*event_register_withkeydata = *eventc::event_register_withkeydata;
*event_register_withkeyfile_withretry = *eventc::event_register_withkeyfile_withretry;
*event_register_withkeydata_withretry = *eventc::event_register_withkeydata_withretry;
*event_unregister = *eventc::event_unregister;
*c_event_poll = *eventc::c_event_poll;
*c_event_poll_blocking = *eventc::c_event_poll_blocking;
*dont_use_this_function_because_it_does_not_work = *eventc::dont_use_this_function_because_it_does_not_work;
*event_stop_main = *eventc::event_stop_main;
*event_notify = *eventc::event_notify;
*event_schedule = *eventc::event_schedule;
*event_notification_alloc = *eventc::event_notification_alloc;
*event_notification_free = *eventc::event_notification_free;
*event_notification_clone = *eventc::event_notification_clone;
*event_notification_get_double = *eventc::event_notification_get_double;
*event_notification_get_int32 = *eventc::event_notification_get_int32;
*event_notification_get_int64 = *eventc::event_notification_get_int64;
*event_notification_get_opaque = *eventc::event_notification_get_opaque;
*c_event_notification_get_string = *eventc::c_event_notification_get_string;
*event_notification_put_double = *eventc::event_notification_put_double;
*event_notification_put_int32 = *eventc::event_notification_put_int32;
*event_notification_put_int64 = *eventc::event_notification_put_int64;
*event_notification_put_opaque = *eventc::event_notification_put_opaque;
*event_notification_put_string = *eventc::event_notification_put_string;
*event_notification_remove = *eventc::event_notification_remove;
*c_event_subscribe = *eventc::c_event_subscribe;
*event_subscribe_auth = *eventc::event_subscribe_auth;
*event_async_subscribe = *eventc::event_async_subscribe;
*event_unsubscribe = *eventc::event_unsubscribe;
*event_async_unsubscribe = *eventc::event_async_unsubscribe;
*event_notification_insert_hmac = *eventc::event_notification_insert_hmac;
*event_set_idle_period = *eventc::event_set_idle_period;
*event_set_failover = *eventc::event_set_failover;
*event_arg_get = *eventc::event_arg_get;
*event_arg_dup = *eventc::event_arg_dup;
*event_notification_create_v = *eventc::event_notification_create_v;
*event_notification_create = *eventc::event_notification_create;
*event_do_v = *eventc::event_do_v;
*event_do = *eventc::event_do;
*xmalloc = *eventc::xmalloc;
*xrealloc = *eventc::xrealloc;
*make_timestamp = *eventc::make_timestamp;
*allocate_callback_data = *eventc::allocate_callback_data;
*free_callback_data = *eventc::free_callback_data;
*dequeue_callback_data = *eventc::dequeue_callback_data;
*enqueue_callback_data = *eventc::enqueue_callback_data;
*perl_stub_callback = *eventc::perl_stub_callback;
*stub_event_subscribe = *eventc::stub_event_subscribe;
*event_notification_get_string = *eventc::event_notification_get_string;
*event_notification_get_site = *eventc::event_notification_get_site;
*event_notification_get_expt = *eventc::event_notification_get_expt;
*event_notification_get_group = *eventc::event_notification_get_group;
*event_notification_get_host = *eventc::event_notification_get_host;
*event_notification_get_objtype = *eventc::event_notification_get_objtype;
*event_notification_get_objname = *eventc::event_notification_get_objname;
*event_notification_get_eventtype = *eventc::event_notification_get_eventtype;
*event_notification_get_arguments = *eventc::event_notification_get_arguments;
*event_notification_set_arguments = *eventc::event_notification_set_arguments;
*event_notification_get_sender = *eventc::event_notification_get_sender;
*event_notification_set_sender = *eventc::event_notification_set_sender;

############# Class : event::event_handle ##############

package event::event_handle;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( event );
%OWNER = ();
%ITERATORS = ();
*swig_server_get = *eventc::event_handle_server_get;
*swig_server_set = *eventc::event_handle_server_set;
*swig_status_get = *eventc::event_handle_status_get;
*swig_status_set = *eventc::event_handle_status_set;
*swig_keydata_get = *eventc::event_handle_keydata_get;
*swig_keydata_set = *eventc::event_handle_keydata_set;
*swig_keylen_get = *eventc::event_handle_keylen_get;
*swig_keylen_set = *eventc::event_handle_keylen_set;
*swig_do_loop_get = *eventc::event_handle_do_loop_get;
*swig_do_loop_set = *eventc::event_handle_do_loop_set;
*swig_connect_get = *eventc::event_handle_connect_get;
*swig_connect_set = *eventc::event_handle_connect_set;
*swig_disconnect_get = *eventc::event_handle_disconnect_get;
*swig_disconnect_set = *eventc::event_handle_disconnect_set;
*swig_mainloop_get = *eventc::event_handle_mainloop_get;
*swig_mainloop_set = *eventc::event_handle_mainloop_set;
*swig_notify_get = *eventc::event_handle_notify_get;
*swig_notify_set = *eventc::event_handle_notify_set;
*swig_subscribe_get = *eventc::event_handle_subscribe_get;
*swig_subscribe_set = *eventc::event_handle_subscribe_set;
*swig_unsubscribe_get = *eventc::event_handle_unsubscribe_get;
*swig_unsubscribe_set = *eventc::event_handle_unsubscribe_set;
sub new {
    my $pkg = shift;
    my $self = eventc::new_event_handle(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        eventc::delete_event_handle($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : event::event_notification ##############

package event::event_notification;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( event );
%OWNER = ();
%ITERATORS = ();
*swig_pubsub_notification_get = *eventc::event_notification_pubsub_notification_get;
*swig_pubsub_notification_set = *eventc::event_notification_pubsub_notification_set;
*swig_has_hmac_get = *eventc::event_notification_has_hmac_get;
*swig_has_hmac_set = *eventc::event_notification_has_hmac_set;
sub new {
    my $pkg = shift;
    my $self = eventc::new_event_notification(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        eventc::delete_event_notification($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : event::address_tuple ##############

package event::address_tuple;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( event );
%OWNER = ();
%ITERATORS = ();
*swig_site_get = *eventc::address_tuple_site_get;
*swig_site_set = *eventc::address_tuple_site_set;
*swig_expt_get = *eventc::address_tuple_expt_get;
*swig_expt_set = *eventc::address_tuple_expt_set;
*swig_group_get = *eventc::address_tuple_group_get;
*swig_group_set = *eventc::address_tuple_group_set;
*swig_host_get = *eventc::address_tuple_host_get;
*swig_host_set = *eventc::address_tuple_host_set;
*swig_objtype_get = *eventc::address_tuple_objtype_get;
*swig_objtype_set = *eventc::address_tuple_objtype_set;
*swig_objname_get = *eventc::address_tuple_objname_get;
*swig_objname_set = *eventc::address_tuple_objname_set;
*swig_eventtype_get = *eventc::address_tuple_eventtype_get;
*swig_eventtype_set = *eventc::address_tuple_eventtype_set;
*swig_scheduler_get = *eventc::address_tuple_scheduler_get;
*swig_scheduler_set = *eventc::address_tuple_scheduler_set;
*swig_timeline_get = *eventc::address_tuple_timeline_get;
*swig_timeline_set = *eventc::address_tuple_timeline_set;
sub new {
    my $pkg = shift;
    my $self = eventc::new_address_tuple(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        eventc::delete_address_tuple($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : event::timeval ##############

package event::timeval;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( event );
%OWNER = ();
%ITERATORS = ();
*swig_tv_sec_get = *eventc::timeval_tv_sec_get;
*swig_tv_sec_set = *eventc::timeval_tv_sec_set;
*swig_tv_usec_get = *eventc::timeval_tv_usec_get;
*swig_tv_usec_set = *eventc::timeval_tv_usec_set;
sub new {
    my $pkg = shift;
    my $self = eventc::new_timeval(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        eventc::delete_timeval($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : event::callback_data ##############

package event::callback_data;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( event );
%OWNER = ();
%ITERATORS = ();
*swig_callback_notification_get = *eventc::callback_data_callback_notification_get;
*swig_callback_notification_set = *eventc::callback_data_callback_notification_set;
*swig_next_get = *eventc::callback_data_next_get;
*swig_next_set = *eventc::callback_data_next_set;
sub new {
    my $pkg = shift;
    my $self = eventc::new_callback_data(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        eventc::delete_callback_data($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package event;

*MAXHOSTNAMELEN = *eventc::MAXHOSTNAMELEN;
*ADDRESSTUPLE_ALL = *eventc::ADDRESSTUPLE_ALL;
*OBJECTTYPE_TESTBED = *eventc::OBJECTTYPE_TESTBED;
*OBJECTTYPE_TRAFGEN = *eventc::OBJECTTYPE_TRAFGEN;
*EVENT_HOST_ANY = *eventc::EVENT_HOST_ANY;
*EVENT_NULL = *eventc::EVENT_NULL;
*EVENT_TEST = *eventc::EVENT_TEST;
*EVENT_SCHEDULE = *eventc::EVENT_SCHEDULE;
*EVENT_TRAFGEN_START = *eventc::EVENT_TRAFGEN_START;
*EVENT_TRAFGEN_STOP = *eventc::EVENT_TRAFGEN_STOP;
*EA_TAG_DONE = *eventc::EA_TAG_DONE;
*EA_Site = *eventc::EA_Site;
*EA_Experiment = *eventc::EA_Experiment;
*EA_Group = *eventc::EA_Group;
*EA_Host = *eventc::EA_Host;
*EA_Type = *eventc::EA_Type;
*EA_Name = *eventc::EA_Name;
*EA_Event = *eventc::EA_Event;
*EA_Arguments = *eventc::EA_Arguments;
*EA_ArgInteger = *eventc::EA_ArgInteger;
*EA_ArgFloat = *eventc::EA_ArgFloat;
*EA_ArgString = *eventc::EA_ArgString;
*EA_When = *eventc::EA_When;

my %__callback_data_list_hash;
tie %__callback_data_list_hash,"event::callback_data", $eventc::callback_data_list;
$callback_data_list= \%__callback_data_list_hash;
bless $callback_data_list, event::callback_data;
*event_string_buffer = *eventc::event_string_buffer;
1;
#
# CODE PAST THIS POINT WAS NOT AUTOMATICALLY GENERATED BY SWIG
#
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
# For now, this has to get cat'ed onto the end of event.pm, since it
# doesn't seem possible to get SWIG to just pass it through into the
# output file
#

#
# Stash away the given callback and data, and call the C event_subscribe
# function that uses a stub callback
#
sub event_subscribe($$$;$) {
	my ($handle,$function,$tuple,$data) = @_;
	$event::callback = $function;
	$event::callback_data = $data;
	return stub_event_subscribe($handle,$tuple);
}

#
# Clear $callback_ready, call the C event_poll function, and see if the
# C callback got called (as evidenced by $callback_ready getting set) If it
# did, call the perl callback function. Handle both blocking and non-blocking
# versions of the call
#
sub internal_event_poll($$$) {
	my ($handle, $block, $timeout) = @_;

	my $rv;
	if ($block) {
		$rv = c_event_poll_blocking($handle,$timeout);
	} else {
		$rv = c_event_poll($handle);
	}

	if ($rv) {
		die "Trouble in event_poll - returned $rv\n";
	}

	while (my $data = dequeue_callback_data() ) {
		&$event::callback($handle,$data->{callback_notification},
				$event::callback_data);
		event_notification_free($handle,$data->{callback_notification});
		free_callback_data($data);
	}

	return 0;
}

#
# Wrapper for the internal polling function, non-blocking version
#
sub event_poll($) {
	my $handle = shift;
	return &internal_event_poll($handle,0,0);
}

#
# Same as above, but for the blocking version
#
sub event_poll_blocking($$) {
	my ($handle, $timeout) = @_;
	return &internal_event_poll($handle,1,$timeout);
}

#
# NOTE: The following line will only work if this module is included by
# a file that has already done a 'use lib' to get the path to testbed
# libraries in the INC path. But, since they had to do that to get this
# library anyway, shouldn't be a problem. (Didn't want to have to make
# this a .in file.)
#
use libtestbed;

#
# Conveniece functions - Intended to work like DBQuery* from libdb .
# Much of this code shamlessly ripped off from libdb.pm
#

#
# Warn after a failed event send. First argument is the error
# message to display. The contents of $EventErrorString is also printed.
# 
# usage: EventWarn(char *message)
#
sub EventWarn($) {
	my($message) = $_[0];
	my($text, $progname);

	#
	# Must taint check $PROGRAM_NAME cause it comes from outside. Silly!
	#
	if ($PROGRAM_NAME =~ /^([-\w.\/]+)$/) {
		$progname = $1;
	} else {
		$progname = "Tainted";
	}

	$text = "$message - In $progname\n" .
		"$EventErrorString\n";

	print STDERR "*** $text";
}

#
# Same as above, but die after the warning.
# 
# usage: EventFatal(char *message);
#
sub EventFatal($) {
	my($message) = $_[0];

	EventWarn($message);

	die("\n");
}


#
# Conveniece function - Intended to work like DBQueryFatal from libdb
#
sub EventSendFatal(@) {
	my @tuple = @_;
    
	my $result = EventSend(@tuple);

	if (!$result) {
		EventFatal("Event Send failed");
	}

	return $result;
}

#
# Conveniece function - Intended to work like DBQueryWarn from libdb
#
sub EventSendWarn(@) {
	my @tuple = @_;
    
	my $result = EventSend(@tuple);

	if (!$result) {
		EventWarn("Event Send failed");
	}

	return $result;
}

#
# Register with the event system. You would use this if you are not
# running on boss. The inline registration below is a convenience for
# testbed software, but is technically bad practice. See the END block
# below where we disconnect at exit.
#
sub EventRegister(;$$) {
        my ($host, $port) = @_;

	if ($event::EventSendHandle) {
	        if (event_unregister($event::EventSendHandle) == 0) {
			warn "Could not unregister with event system";
		}
		$event::EventSendHandle = undef;
	}

	$host = TB_EVENTSERVER()
	    if (!defined($host));

	my $URL = "elvin://$host";
	$URL   .= ":$port"
	    if (defined($port));
	
	$event::EventSendHandle = event_register($URL,0);
	
	if (!$event::EventSendHandle) {
		$EventErrorString = "Unable to register with the event system";
		return undef;
	}

	return 1;
}

sub EventSend(@) {
	my %tuple_values = @_;

	#
	# Only connect on the first call - thereafter, just use the existing
	# handle. The handle gets disconnected in the END block below
	#
	if (!$event::EventSendHandle) {
		EventRegister("localhost", TB_BOSSEVENTPORT());

		if (!$event::EventSendHandle) {
			$EventErrorString =
				"Unable to register with the event system";
			return undef;
		}
	}

	my $tuple = address_tuple_alloc();
	if (!$tuple) {
		$EventErrorString = "Unable to allocate an address tuple";
		return undef;
	}

	#
	# Set the values the user requested
	#
	%$tuple = %tuple_values;

	my $notification = event_notification_alloc($event::EventSendHandle,
		$tuple);
	if (!$notification) {
		$EventErrorString = "Could not allocate notification";
		return undef;
	}

	if (!event_notify($event::EventSendHandle, $notification)) {
		$EventErrorString = "Could not send event notification";
		return undef;
	}

	event_notification_free($event::EventSendHandle, $notification);
	address_tuple_free($tuple);

	return 1;
}

#
# After a fork, undef the handle to the event system so that we form a
# a new connection in the child. Do not disconnect from the child; I have
# no idea what that will do to the parent connection.
#
sub EventFork() {
    $event::EventSendHandle = undef;
}

#
# When we exit, unregister with the event system if we're connected
#
END {
    	if ($event::EventSendHandle) {
		if (event_unregister($event::EventSendHandle) == 0) {
			warn "Could not unregister with event system";
		}
	}
}

push @EXPORT, qw(event_subscribe event_poll event_poll_blocking EventSend
	EventSendFatal EventSendWarn EventFork EventRegister);
1;

