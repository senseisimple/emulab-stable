#!/usr/bin/perl -w
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003-2003 University of Utah and the Flux Group.
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
# $rv = endStateWait();
#
# Stop listening for states. Call this soon after waitForState.
# This must be called before calling initStateWait again.
#

package StateWait;

use Exporter;
@ISA = "Exporter";
@EXPORT = qw ( initStateWait waitForState endStateWait );

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
$port   = "";
$URL    = "";
$debug  = 0;

my $handle;
my $tuple;

my @done = ();
my @failures = ();
my @nodes = ();
my %remain = ();

#
# Exported Sub-Routines / Functions
#

sub initStateWait( $@ ) {
    my $states = shift;
    @nodes = @_;
    # states is an arrayref
    if ($debug) {
	print "initStateWait: states=(".join(",",@$states).
	  ") nodes=(".join(",",@nodes).")\n";
    }
    if ($server eq "") { $server = TB_BOSSNODE; }
    if ($URL eq "") { $URL = "elvin://$server"; }
    if ($port ne "") { $URL .= ":$port"; }

    # Do the subscription for the right stuff, including all the
    # states for all the nodes, and the failure event for all nodes

    $handle = event_register($URL,0);
    if (!$handle) { die "Unable to register with event system\n"; }

    $tuple = address_tuple_alloc();
    if (!$tuple) { die "Could not allocate an address tuple\n"; }

    %$tuple = ( objtype   => TBDB_TBEVENT_NODESTATE #,
		eventtype   => join(",",@$states),
	        objname   => join(",",@nodes) );

    if ($debug > 1) {
	print "tuple = ('".join("', '",keys(%$tuple))."') => ('".
	  join("', '",values(%$tuple))."')\n";
    }

    if (!event_subscribe($handle,\&doEvent,$tuple)) {
        die "Could not subscribe to events\n";
    }

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
    if ($debug > 1) {
	print "Event: $time $site $expt $group $host $objtype $objname " .
	  "$eventtype\n";
    }
    my $n = $objname;
    if (@{$remain{$n}}[0] eq $eventtype) {
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
    #exit(0);
}

sub waitForState( $$;$ ) {
    my ($finished,$failed,$timeout) = @_;
    if (!defined($timeout)) { $timeout=0; }
    my $now = ($timeout > 0 ? time() : 0);
    my $deadline = ($timeout > 0 ? $now + $timeout : 1);
    my $total = scalar(@done) + scalar(@failures);
    while ($total < scalar(@nodes) && $now < $deadline) {
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
	  "failed=(".join(",",@$failed).")\n";
    }
    return 0;
}

sub endStateWait() {

    if (event_unregister($handle) == 0) {
        die "Unable to unregister with event system\n";
    }
    if ($debug) { print "endStateWait\n"; }
    return 0;
}

# Don't enable this unless you fix it first...
if (0) {
    # do some testing...
    my @states=("SHUTDOWN","ISUP");
    my @finished=();
    my @failed=("foo");
    print "Failed was (".join(",",@failed).")\n";
    initStateWait(["ISUP"],pc52);
    waitForState(\@finished,\@failed);
    endStateWait();
    print "Failed was (".join(",",@failed).")\n";

    initStateWait(\@states,pc31,pc32);
    waitForState(\@finished,\@failed);
    endStateWait();
    print "Failed was (".join(",",@failed).")\n";

}


# Always end a package successfully!
1;

