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
# $rv = waitForState(\@failed);
#
# Do the actual waiting. Blocks until each node has either reached
# the desired state(s) or failed. Returns number of failures.
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
use event;

my $server = "boss";
my $port   = "";
my $URL    = "elvin://$server";


#
# Exported Sub-Routines / Functions
#

sub initStateWait( $@ ) {
    my ($states, @nodes) = @_;
    # states is an arrayref
    print "initStateWait: states=(".join(",",@$states).
      ") nodes=(".join(",",@nodes).")\n";
    return 0;
}

sub waitForState( $ ) {
    my ($failed) = @_;
    # failed is an arrayref
    @$failed=();
    print "waitForState: failed=(".join(",",@$failed).")\n";
    return 0;
}

sub endStateWait() {

    print "endStateWait\n";
    return 0;
}

if (1) {
    # do some testing...
    my @states=("SHUTDOWN","ISUP");
    my @failed=("foo");
    print "Failed was (".join(",",@failed).")\n";
    initStateWait(["ISUP"],pc52);
    waitForState(\@failed);
    endStateWait();
    print "Failed was (".join(",",@failed).")\n";

    initStateWait(\@states,pc31,pc32);
    waitForState(\@failed);
    endStateWait();
    print "Failed was (".join(",",@failed).")\n";

}


# Always end a package successfully!
1;

