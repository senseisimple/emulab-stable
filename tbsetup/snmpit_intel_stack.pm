#!/usr/bin/perl -w
#
# snmpit module for Intel EtherExpress 510T switches. This module is largely
# just a wrapper around snmpit_intel, but is here just in case we ever need to
# contact multiple switches (which may happen if we move to port-based (instead
# of MAC-based) VLANs.) For now, though, pretty much everything just calls
# $self->{LEADER}->foo()
#

package snmpit_intel_stack;
use strict;

$| = 1;				# Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libdb;

#
# Creates a new object. A list of devices that will be operated on is given
# so that the object knows which to connect to. A future version may not 
# require the device list, and dynamically connect to devices as appropriate
#
# For an Intel stack, the stack_id happens to also be the name of the stack
# leader.
#
# usage: new(string name, string stack_id, int debuglevel list of devicenames)
# returns a new object blessed into the snmpit_intel_stack class
#

sub new($$#@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my $debuglevel = shift;
    my @devicenames = @_; # Devicenames are not presently needed for Intel
			  # stacks


    #
    # Create the actual object
    #
    my $self = {};

    #
    # Set up some defaults
    #
    if (defined $debuglevel) {
	$self->{DEBUG} = $debuglevel;
    } else {
	$self->{DEBUG} = 0;
    }

    #
    # The stackid just happens to also be leader of the stack
    # 
    $self->{STACKID} = $stack_id;

    #
    # We only need to create 1 snmpit_intel object, since we only have to
    # talk to one (for now) to do all the setup we need.
    #
    #use snmpit_intel;
    #    $self->{LEADER} = new snmpit_intel($stack_id,$self->{DEBUG});

    bless($self,$class);
    return $self;
}

#
# NOTE: See snmpit_cisco_stack for descriptions of these functions.
# XXX: Document these functions
#

sub listVlans($) {
    my $self = shift;

    return $self->{LEADER}->listVlans();
}

sub listPorts($) {
    my $self = shift;

    return $self->{LEADER}->listPorts();
}

sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;

    return $self->{LEADER}->setPortVlan($vlan_id,@ports);
}

sub createVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @devicenames = @_; # Note: This is not used for Intel switches

    return $self->{LEADER}->createVlan($vlan_id);

}

sub addDevicesToVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @devicenames = @_; # Note: This is not used for Intel switches

    #
    # This function is not needed on Intel stacks, but it may be someday
    #
    if (!$self->vlanExists($vlan_id)) {
	return 1;
    }

    return 0;
}

sub vlanExists($$) {
    my $self = shift;
    my $vlan_id = shift;

    if ($self->{LEADER}->findVlan($vlan_id)) {
	return 1;
    } else {
	return 0;
    }
}


sub removeVlan($$) {
    my $self = shift; 
    my $vlan_id = shift;
    my $errors = 0;
		    
    #
    # First, make sure that the VLAN really does exist
    #
    my $vlan_number = $self->{LEADER}->findVlan($vlan_id);
    if (!$vlan_number) {
	warn "ERROR: VLAN $vlan_id not found on switch!";
	return 0;
    }

    my $ok = $self->{LEADER}->removeVlan($vlan_id);

    return ($ok && ($errors == 0));
}       

sub portControl ($$@) { 
    my $self = shift;
    my $cmd = shift;
    my @ports = @_;

    return $self->{LEADER}->portControl($cmd,@ports);
}

sub getStats($) {
    my $self = shift;

    return $self->{LEADER}->getStats();
}
