#!/usr/local/bin/perl -w
#
# snmpit module for a stack of Cisco Catalyst 6509 switches. The main purpose
# of this module is to contain knowledge of how to manage stack-wide operations
# (such as VLAN creation), and to coallate the results of listing operations
# on multiple switches
#

package snmpit_cisco_stack;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libdb;

#
# Creates a new object. A list of devices that will be operated on is given
# so that the object knows which to connect to. A future version may not 
# require the device list, and dynamically connect to devices as appropriate
#
# For a Cisco stack, the stack_id happens to also be the name of the stack
# leader.
#
# usage: new(string name, string stack_id, list of devicenames)
# returns a new object blessed into the snmpit_cisco_stack class
#
sub new($$@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my @devicenames = @_;

    #
    # Create the actual object
    #
    my $self = {};

    #
    # Set up some defaults
    #
    $self->{DEBUG} = 0;

    #
    # The stackid just happens to also be leader of the stack
    # 
    $self->{STACKID} = $stack_id;

    #
    # Store the list of devices we're supposed to operate on
    #
    @{$self->{DEVICENAMES}} = @devicenames;

    #
    # Make a device-dependant object for each switch
    #
    foreach my $devicename (@devicenames) {
	print("Making device object for $devicename\n") if $self->{DEBUG};
	my $type = getDeviceType($devicename);
	my $device;

	#
	# Check to see if this is a duplicate
	#
	if (defined($self->{DEVICES}{$devicename})) {
	    warn "WARNING: Device $device was specified twice, skipping\n";
	    next;
	}

	#
	# We check the type for two reasons: We may have multiple types of
	# ciscos in the future, and for some sanity checking to make sure
	# we weren't given devicenames for devices that aren't ciscos
	#
	SWITCH: for ($type) {
	    /cisco6509/ && do {
		use snmpit_cisco;
		$device = new snmpit_cisco($devicename,$self->{DEBUG});
		if (!$device) {
		    die "Failed to create a device object for $devicename\n";
		} else {
		    $self->{DEVICES}{$devicename} = $device;
		    if ($devicename eq $self->{STACKID}) {
			$self->{LEADER} = $device;
		    }
		    last;
		}
	    };
	    die "Device $devicename is not of a known type, skipping\n";
	}

    }

    #
    # Check for the stack leader, and create it if it hasn't been so far
    #
    if (!$self->{LEADER}) {
	# XXX: For simplicity, we assume for now that the leader is a Cisco
	 use snmpit_cisco;
	 $self->{LEADER} = new snmpit_cisco($self->{STACKID});
     }

    bless($self,$class);

    return $self;
}

#
# List all VLANs on all switches in the stack
#
# usage: vlistVlans(self)
#
# returns: A list of VLAN information. Each entry is an array reference. The
#	array is in the form [id, ddep, members] where:
#		id is the VLAN identifier, as stored in the database
#		ddep is an opaque string that is device-dependant (mostly for
#			debugging purposes)
#		members is a reference to an array of VLAN members
#
sub listVlans($) {
    my $self = shift;

    #
    # We need to 'coallate' the results from each switch by putting together
    # the results from each switch, based on the VLAN identifier
    #
    my %vlans = ();
    foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}{$devicename};
	foreach my $line ($device->listVlans()) {
	    my ($vlan_id, $vlan_number, $memberRef) = @$line;
	    if ($memberRef) {
		${$vlans{$vlan_id}}[0] = $vlan_number;
		push @{${$vlans{$vlan_id}}[1]}, @$memberRef;
	    }
	}
    }

    #
    # Now, we put the information we've found in the format described by
    # the header comment for this function
    #
    my @vlanList;
    foreach my $vlan (sort {tbsort($a,$b)] keys %vlans) {
	push @vlanList, [$vlan, @{$vlans{$vlan}}];
    } 
    return @vlanList;
}

#
# List all ports on all switches in the stack
#
# usage: listPorts(self)
#
# returns: A list of port information. Each entry is an array reference. The
#	array is in the form [id, enabled, link, speed, duplex] where:
#		id is the port identifier (in node:port form)
#		enabled is "yes" if the port is enabled, "no" otherwise
#		link is "up" if the port has carrier, "down" otherwise
#		speed is in the form "XMbps"
#		duplex is "full" or "half"
#
sub listPorts($) {
    my $self = shift;

    #
    # All we really need to do here is coallate the results of listing the
    # ports on all devices
    #
    my %portinfo = (); 
    foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}{$devicename};
	foreach my $line ($device->listPorts()) {
	    my $port = $$line[0];
	    if (defined $portinfo{$port}) {
		warn "WARNING: Two ports found for $port\n";
	    }
	    $portinfo{$port} = $line;
	}
    }

    return map $portinfo{$_}, sort {tbsort($a,$b)} keys %portinfo;
}

#
# Puts ports in the VLAN with the given identifier. Contacts the device
# appropriate for each port.
#
# usage: setPortVlan(self, vlan_id, list of ports)
# returns: the number of errors encountered in processing the request
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;

    my $errors = 0;

    #
    # Split up the ports among the devices involved
    #
    my %map = mapPortsToDevices(@ports);
    foreach my $devicename (keys %map) {
	my $device = $self->{DEVICES}{$devicename};
    	if (!defined($device)) {
    	    warn "Unable to find device entry for $devicename - some ports " .
	    	"will not be set up properly\n";
    	    $errors++;
    	    next;
    	}

	#
	# Simply make the appropriate call on the device
	#
	$errors += $device->setPortVlan($vlan_id,@{$map{$devicename}});
    }

    return $errors;
}

#
# Creates a VLAN with the given VLAN identifier on the stack. A device list
# is given to indicate which devices the VLAN must span. It is an error to
# create a VLAN that already exists.
#
# usage: createVlan(self, vlan identfier, device list)
#
# returns: 1 on success
# returns: 0 on failure
#
sub createVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @devicenames = @_; # Note: This is not used for Cisco switches

    #
    # We just need to create the VLAN on the stack leader
    #
    return $self->{LEADER}->createVlan($vlan_id);

}

#
# Adds devices to an existing VLAN, in preparation for adding ports on these
# devices. It is an error to call this function on a VLAN that does not
# exist. It is _not_ an error to add a VLAN to a device on which it already
# exists.
#
# usage: addDevicesToVlan(self, vlan identfier, device list)
#
# returns: the number of errors encountered in processing the request
#
sub addDevicesToVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @devicenames = @_; # Note: This is not used for Cisco switches

    #
    # This function is not needed on Cisco stacks, since all switches
    # share the same set of VLANs. We will, however, check to make sure
    # the VLAN really does exist
    #
    if (!$self->vlanExists($vlan_id)) {
	return 1;
    }

    return 0;
}

#
# Check to see if the given VLAN exists in the stack
#
# usage: vlanExists(self, vlan identifier)
#
# returns 1 if the VLAN exists
#         0 otherwise
#
sub vlanExists($$) {
    my $self = shift;
    my $vlan_id = shift;

    #
    # The leader holds the list of which VLANs exist
    #
    if ($self->{LEADER}->findVlan($vlan_id)) {
	return 1;
    } else {
	return 0;
    }

}

#
# Removes a VLAN from the stack. This implicitly removes all ports from the
# VLAN. It is an error to remove a VLAN that does not exist.
#
# usage: removeVlan(self, vlan identifier)
#
# returns: 1 on success
# returns: 0 on failure
#
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

    #
    # Now, we go through each device and remove all ports from the VLAN
    # on that device
    #
    foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}{$devicename};

	#
	# Only remove ports from the VLAN if it exists on this
	# device
	#
	if (defined(my $number = $device->findVlan($vlan_id))) {
		$errors += $device->removePortsFromVlan($vlan_id);
	}
    }

    my $ok = $self->{LEADER}->removeVlan($vlan_id);

    return ($ok && ($errors == 0));
}

#
# Set a variable associated with a port. 
# TODO: Need a list of variables here
#
sub portControl ($$@) { 
    my $self = shift;
    my $cmd = shift;
    my @ports = @_;
    my %portDeviceMap = mapPortsToDevices(@ports);
    my $errors = 0;
    # XXX: each
    while (my ($devicename,$ports) = each %portDeviceMap) {
	$errors += $self->{DEVICES}{$devicename}->portControl($cmd,@$ports);
    }
    return $errors;
}

#
#
#
sub getStats($) {
    my $self = shift;

    #
    # All we really need to do here is coallate the results of listing the
    # ports on all devices
    #
    my %stats = (); 
    foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}{$devicename};
	foreach my $line ($device->getStats()) {
	    my $port = $$line[0];
	    if (defined $stats{$port}) {
		warn "WARNING: Two ports found for $port\n";
	    }
	    $stats{$port} = $line;
	}
    }
    return map $stats{$_}, sort {tbsort($a,$b)} keys %stats;
}

# End with true
1;
