#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2004 University of Utah and the Flux Group.
# All rights reserved.
#

#
# StateWait.pm
#
# A library for waiting for nodes to reach certain states.
#
# $rv = initStateWait(\@states, @nodes);
#
# Call this first. Make sure to call it _before_ performing any
# action that may trigger one or more of the states you're
# expecting to see. Each node must see the list of states in the
# proper order, ie if @states=(stateA, stateB), it will not
# complete until the first time stateB is seen after stateA has
# been seen. Returns 0 on success.
#
# $rv = waitForState(\@finished, \@failed[, $timeout);
#
# Do the actual waiting. Blocks until each node has either reached the
# desired state(s) or failed, or timeout is reached (if given and
# non-zero). Returns lists of nodes.
#
# $rv = cancelWait($node);
#
# "Nevermind..." for state waiting. Returns 0 on success.
#
# $rv = endStateWait();
#
# Stop listening for states. Call this soon after waitForState.
# This must be called before calling initStateWait again.
#

package StateWait;

use Exporter;
@ISA = "Exporter";
@EXPORT = qw ( initStateWait waitForState cancelWait endStateWait );

# Any 'use' or 'require' stuff must come here, after 'package'
# Note that whatever script is using us has already done a "use lib" to
# be able to use us. So we don't need to do one, or be configged.
use event;
use libtestbed;
use libdb;

# Set these to nothing here, so they can be set before calling init
# by the caller
# Don't my them, or it won't work!
$server = "";
$port   = TB_BOSSEVENTPORT();
$URL    = "";
$debug  = 0;

$handle=0;
$tuple=0;

my @done;
my @failures;
my $nodecount;
my %remain;

#
# Exported Sub-Routines / Functions
#

sub initStateWait( $@ ) {
    my $states = shift;
    my @nodes = @_;
    @done=();
    @failures=();
    $nodecount=0;
    %remain=();
    $nodecount = scalar(@nodes);
    # states is an arrayref
    if ($debug) {
	print "initStateWait: states=(".join(",",@$states).
	  ") nodes=(".join(",",@nodes).")\n";
    }
    if ($server eq "") { $server = "localhost"; }
    if ($URL eq "") { $URL = "elvin://$server"; }
    if ($port ne "") { $URL .= ":$port"; }

    # Do the subscription for the right stuff, including all the
    # states for all the nodes, and the failure event for all nodes

    if ($handle==0) {
	if ($debug) { print "Getting handle for $URL - "; }
	$handle = event_register($URL,0);
	if ($debug) { print "returned - "; }
	if (!$handle) { die "Unable to register with event system\n"; }
	if ($debug) { print "Success: $handle\n"; }
    }

    if ($debug) { print "Getting tuple - "; }
    $tuple = address_tuple_alloc();
    if (!$tuple) { die "Could not allocate an address tuple\n"; }

    %$tuple = ( objtype   => join(",",TBDB_TBEVENT_NODESTATE,
				  TBDB_TBEVENT_TBFAILED),
		eventtype => join(",",@$states, TBDB_COMMAND_REBOOT  ) );
		#eventtype => join(",",@$states, TBDB_COMMAND_REBOOT  ),
	        #objname   => join(",",@nodes) );

    if ($debug) { print "Success: $tuple\n"; }
    if ($debug > 0) {
	print "tuple = ('".join("', '",keys(%$tuple))."') => ('".
	  join("', '",values(%$tuple))."')\n";
    }

    if ($debug) { print "Subscribing - "; }
    if (!event_subscribe($handle,\&doEvent,$tuple)) {
        die "Could not subscribe to events\n";
    }
    if ($debug) { print "Success.\n"; }

    foreach $n (@nodes) {
	my @l = @$states;
	$remain{$n} = \@l;
    }

    if ($debug) {
	foreach $n (@nodes) { print "$n==>(".join(",",@{$remain{$n}}).")\n"; }
    }

    return 0;
}

sub doEvent( $$$ ) {
    my ($handle,$event) = @_;

    my $time      = time();
    my $site      = event_notification_get_site($handle, $event);
    my $expt      = event_notification_get_expt($handle, $event);
    my $group     = event_notification_get_group($handle, $event);
    my $host      = event_notification_get_host($handle, $event);
    my $objtype   = event_notification_get_objtype($handle, $event);
    my $objname   = event_notification_get_objname($handle, $event);
    my $eventtype = event_notification_get_eventtype($handle, $event);
    if ($debug ) {
	print "Event: $time $site $expt $group $host $objtype $objname " .
	  "$eventtype\n";
    }
    my $n = $objname;
    if (defined($remain{$n}) && $objtype eq TBDB_TBEVENT_TBFAILED) {
	# It is a failed boot... add it to the failures list.
	if ($debug) { print "Got $eventtype failure for $n... Aborting!\n"; }
	push(@failures,$n);
	delete($remain{$n});
    }
    if (defined($remain{$n}) && @{$remain{$n}}[0] eq $eventtype) {
	# this is the next state we were waiting for
	if ($debug) { print "Got $eventtype for $n\n" };
	shift(@{$remain{$n}});
	if ($debug) { print "$n==>(".join(",",@{$remain{$n}}).")\n"; }
	if (scalar(@{$remain{$n}}) == 0) {
	    if ($debug) { print "Done with $n!\n"; }
	    push(@done,$n);
	    delete($remain{$n});
	}
    }
}

sub waitForState( $$;$ ) {
    my ($finished,$failed,$timeout) = @_;
    if (!defined($timeout)) { $timeout=0; }
    my $now = ($timeout > 0 ? time() : 0);
    my $deadline = ($timeout > 0 ? $now + $timeout : 1);
    my $total = scalar(@done) + scalar(@failures);
    while ($total < $nodecount && $now < $deadline) {
	if ($timeout > 0) {
	    if ($debug) {
		print "waitForState: done=(".join(",",@done)."), ".
		  "failures=(".join(",",@failures).")\n";
		print "Polling for ".($deadline-$now)." seconds\n";
	    }
	    event_poll_blocking($handle,($deadline-$now)*1000);
	    $now = time();
	} else {
	    event_poll_blocking($handle,0);
	}
	$total = scalar(@done) + scalar(@failures);
    }
    # finished and failed are arrayrefs
    @$finished=@done;
    @$failed=@failures;
    if ($debug) {
	print "waitForState: finished=(".join(",",@$finished)."), ".
	  "failed=(".join(",",@$failed)."), ".
	    "notdone=(".join(",",keys %remain).")\n";
	print "  Wait for state returning now...\n"
    }
    return 0;
}

sub cancelWait( $ ) {
    # Important thing to remember: This will never get called while
    # we're in the middle of a waitForState call, so we just need to
    # make sure things are consistent next time they call it.
    my $node = @_;
    if (!defined($remain{$node})) {
	return 1;
    }
    delete $remain{$node};
    $nodecount--;
    return 0;
}

sub endStateWait() {
    %remain = ();
    $tuple = address_tuple_free($tuple);
    if ($debug) { print "endStateWait\n"; }
    return 0;
}

# Always end a package successfully!
1;

END {
    if ($handle!=0 && event_unregister($handle) == 0) {
        warn "Unable to unregister with event system\n";
    }
    1;
}
