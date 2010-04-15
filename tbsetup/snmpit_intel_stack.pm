#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

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

sub new($$$@) {

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
    # talk to one (for now) to do all the setup we need. We fall back on the
    # old behavior of using the stack name as the leader if the leader is not
    # set
    #
    my $leader_name = getStackLeader($stack_id);
    if (!$leader_name) {
        $leader_name = $stack_id;
    }
    $self->{LEADERNAME} = $leader_name;

    use snmpit_intel;
    $self->{LEADER} = new snmpit_intel($leader_name,$self->{DEBUG});

    #
    # Check for failed object creation
    #
    if (!$self->{LEADER}) {
        #
        # The snmpit_intel object has already printed an error message,
        # so we'll just return an error
        #
        return undef;
    }

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

sub createVlan($$$;@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @{shift()};
    #
    # We ignore other args for now, since Intels don't support private VLANs
    #

    return $self->{LEADER}->createVlan($vlan_id,@ports);

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

    if ($self->{LEADER}->findVlan($vlan_id,1)) {
	return 1;
    } else {
	return 0;
    }
}

#
# Return a list of which VLANs from the input set exist on this stack
#
# usage: existantVlans(self, vlan identifiers)
#
# returns: a list containing the VLANs from the input set that exist on this
# 	stack
#
sub existantVlans($@) {
    my $self = shift;
    my @vlan_ids = @_;

    #
    # The leader holds the list of which VLANs exist
    #
    my %mapping = $self->{LEADER}->findVlans(@vlan_ids);

    my @existant = ();
    foreach my $vlan_id (@vlan_ids) {
	if (defined $mapping{$vlan_id}) {
	    push @existant, $vlan_id;
	}
    }

    return @existant;

}

sub removeVlan($@) {
    my $self = shift; 
    my @vlan_ids = @_;
		    
    my $errors = 0;
    foreach my $vlan_id (@vlan_ids) {
	#
	# First, make sure that the VLAN really does exist
	#
	my $vlan_number = $self->{LEADER}->findVlan($vlan_id);
	if (!$vlan_number) {
	    warn "ERROR: VLAN $vlan_id not found on switch!";
	    $errors++;
	}
	my $ok = $self->{LEADER}->removeVlan($vlan_id);
	if (!$ok) {
	    $errors++;
	}
    }

    return ($errors == 0);
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


#
# Openflow enable function for Intel stack
# Till now it just report an error.
#
# enableOpenflow(self, vlan_id)
# return # of errors
#
sub enableOpenflow($$) {
    my $self = shift;
    my $vlan_id = shift;
    
    my $errors = 0;
    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	my $vlan_number = $device->findVlan($vlan_id, 2);
	if (!$vlan_number) {
	    #
	    # Not sure if this is an error or not.
	    # It might be possible that not all devices in a stack have the given VLAN.
	    #
	    print "$device has no VLAN $vlan_id, ignore it. \n" if $self->{DEBUG};
	} else {
	    if ($device->isOpenflowSupported()) {
		print "Enabling Openflow on $devicename for VLAN $vlan_id".
		    "..." if $self->{DEBUG};

		my $ok = $device->enableOpenflow($vlan_number);
		if (!$ok) { $errors++; }
		else {print "Done! \n" if $self->{DEBUG};}
	    } else {
		#
		# TODO: Should this be an error?
		#
		warn "ERROR: Openflow is not supported on $devicename \n";
		$errors++;
	    }
	}
    }
        
    return $errors;
}

#
# Disable Openflow
#
# disableOpenflow(self, vlan_id);
# return # of errors
#
sub disableOpenflow($$) {
    my $self = shift;
    my $vlan_id = shift;
    
    my $errors = 0;
    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	my $vlan_number = $device->findVlan($vlan_id, 2);
	if (!$vlan_number) {
	    #
	    # Not sure if this is an error or not.
	    # It might be possible that not all devices in a stack have the given VLAN.
	    #
	    print "$device has no VLAN $vlan_id, ignore it. \n" if $self->{DEBUG};
	} else {
	    if ($device->isOpenflowSupported()) {
		print "Disabling Openflow on $devicename for VLAN $vlan_id".
		    "..." if $self->{DEBUG};

		my $ok = $device->disableOpenflow($vlan_number);
		if (!$ok) { $errors++; }
		else {print "Done! \n" if $self->{DEBUG};}
	    } else {
		#
		# TODO: Should this be an error?
		#
		warn "ERROR: Openflow is not supported on $devicename \n";
		$errors++;
	    }
	}
    }
        
    return $errors;
}

#
# Set Openflow controller on VLAN
#
# setController(self, vlan_id, controller);
# return # of errors
#
sub setOpenflowController($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $controller = shift;
    
    my $errors = 0;
    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	my $vlan_number = $device->findVlan($vlan_id, 2);
	if (!$vlan_number) {
	    #
	    # Not sure if this is an error or not.
	    # It might be possible that not all devices in a stack have the given VLAN.
	    #
	    print "$device has no VLAN $vlan_id, ignore it. \n" if $self->{DEBUG};
	} else {
	    if ($device->isOpenflowSupported()) {
		print "Setting Openflow controller on $devicename for VLAN $vlan_id".
		    "..." if $self->{DEBUG};

		my $ok = $device->setOpenflowController($vlan_number, $controller);
		if (!$ok) { $errors++; }
		else {print "Done! \n" if $self->{DEBUG};}
	    } else {
		#
		# TODO: Should this be an error?
		#
		warn "ERROR: Openflow is not supported on $devicename \n";
		$errors++;
	    }
	}
    }
        
    return $errors;
}

#
# Set Openflow listener on VLAN
#
# setListener(self, vlan_id, listener);
# return # of errors
#
# This function might be replaced by a enableListener(vlan_id)
# that sets the listener on switches automatically and returns
# the listener connection string.
#
sub setOpenflowListener($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $listener = shift;
    
    my $errors = 0;
    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	my $vlan_number = $device->findVlan($vlan_id, 2);
	if (!$vlan_number) {
	    #
	    # Not sure if this is an error or not.
	    # It might be possible that not all devices in a stack have the given VLAN.
	    #
	    print "$device has no VLAN $vlan_id, ignore it. \n" if $self->{DEBUG};
	} else {
	    if ($device->isOpenflowSupported()) {
		print "Setting Openflow listener on $devicename for VLAN $vlan_id".
		    "..." if $self->{DEBUG};

		my $ok = $device->setOpenflowListener($vlan_number, $listener);
		if (!$ok) { $errors++; }
		else {print "Done! \n" if $self->{DEBUG};}
	    } else {
		#
		# TODO: Should this be an error?
		#
		warn "ERROR: Openflow is not supported on $devicename \n";
		$errors++;
	    }
	}	
    }
        
    return $errors;
}

#
# Get used Openflow listener ports
#
# getUsedOpenflowListenerPorts(self, vlan_id)
#
sub getUsedOpenflowListenerPorts($$) {
    my $self = shift;
    my $vlan_id = shift;
    my %ports = ();

    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	my $vlan_number = $device->findVlan($vlan_id, 2);
	if (!$vlan_number) {
	    #
	    # Not sure if this is an error or not.
	    # It might be possible that not all devices in a stack have the given VLAN.
	    #
	    print "$device has no VLAN $vlan_id, ignore it. \n" if $self->{DEBUG};
	} else {
	    if ($device->isOpenflowSupported()) {		
		my %tmports = $device->getUsedOpenflowListenerPorts();
		@ports{ keys %tmports } = values %tmports;		
	    } else {
		#
		# YES this be an error because the VLAN is on it.
		#
		warn "ERROR: Openflow is not supported on $devicename \n";
	    }
	}	
    }

    return %ports;
}

# End with true
1;
