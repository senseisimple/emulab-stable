#!/usr/bin/perl -w

#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

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
# usage: new(string name, string stack_id, int debuglevel, list of devicenames)
# returns a new object blessed into the snmpit_cisco_stack class
#
sub new($$$#@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my $debuglevel = shift;
    my $community = shift;
    my $supports_private = shift;
    my $uses_vtp = shift;
    my @devicenames = @_;

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
    # Store the list of devices we're supposed to operate on
    #
    @{$self->{DEVICENAMES}} = @devicenames;

    #
    # Whether or not this stack uses VTP to keep the VLANs synchronized
    #
    $self->{VTP} = $uses_vtp;

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
	    (/cisco6509/ || /cisco4006/ || /cisco2980/) && do {
		use snmpit_cisco;
		$device = new snmpit_cisco($devicename,$self->{DEBUG},$type,
			$community,$supports_private);
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
	my $type = getDeviceType($self->{STACKID});
	$self->{LEADER} = new snmpit_cisco($self->{STACKID}, $self->{DEBUG},
	    $type, $community, $supports_private)
    }

    bless($self,$class);

    return $self;
}

#
# List all VLANs on all switches in the stack
#
# usage: listVlans(self)
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
	    ${$vlans{$vlan_id}}[0] = $vlan_number;
	    push @{${$vlans{$vlan_id}}[1]}, @$memberRef;
	}
    }

    #
    # Now, we put the information we've found in the format described by
    # the header comment for this function
    #
    my @vlanList;
    foreach my $vlan (sort {tbsort($a,$b)} keys %vlans) {
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
    # Grab the VLAN number
    #
    my $vlan_number = $self->{LEADER}->findVlan($vlan_id);
    if (!$vlan_number) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist!\n";
	return 1;
    }

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
	$errors += $device->setPortVlan($vlan_number,@{$map{$devicename}});
    }

    if ($vlan_id ne 'default') {
	$errors += (!$self->setVlanOnTrunks($vlan_number,1,@ports));
    }

    return $errors;
}

#
# Creates a VLAN with the given VLAN identifier on the stack. If ports are
# given, puts them into the newly created VLAN. It is an error to create a
# VLAN that already exists.
#
# usage: createVlan(self, vlan identfier, port list)
#
# returns: 1 on success
# returns: 0 on failure
#
sub createVlan($$$;$$$) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @{shift()};
    my @otherargs = @_;

    #
    # What we do here depends on whether this stack uses VTP to synchronize
    # VLANs or not
    #
    my $okay = 1;
    if ($self->{VTP}) {
	#
	# We just need to create the VLAN on the stack leader
	#
	#
	my $vlan_number = $self->{LEADER}->createVlan($vlan_id,undef,@otherargs);
	$okay = ($vlan_number != 0);
    } else {
	#
	# We need to create the VLAN on all devices
	# XXX - should we do the leader first?
	#
	my $vlan_number = undef;
	foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	    print "Creating VLAN on switch $devicename ... \n" if $self->{DEBUG};
	    my $device = $self->{DEVICES}{$devicename};
	    my $res = $device->createVlan($vlan_id,$vlan_number,@otherargs);
	    if (!$res) {
		#
		# Ooops, failed. Don't try any more
		#
		$okay = 0;
		last;
	    } else {
		#
		# Use the VLAN number we just got back for the other switches
		#
		$vlan_number = $res;
	    }
	}
    }

    #
    # We need to add the ports to VLANs at the stack level, since they are
    # not necessarily on the leader
    #
    if ($okay && @ports) {
	if ($self->setPortVlan($vlan_id,@ports)) {
	    $okay = 0;
	}
    }

    return $okay;
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

#
# Removes a VLAN from the stack. This implicitly removes all ports from the
# VLAN. It is an error to remove a VLAN that does not exist.
#
# usage: removeVlan(self, vlan identifiers)
#
# returns: 1 on success
# returns: 0 on failure
#
sub removeVlan($@) {
    my $self = shift;
    my @vlan_ids = @_;
    my $errors = 0;

    #
    # Exit early if no VLANs given
    #
    if (!@vlan_ids) {
	return 1;
    }

    my %vlan_numbers = $self->{LEADER}->findVlans(@vlan_ids);
    foreach my $vlan_id (@vlan_ids) {
	#
	# First, make sure that the VLAN really does exist
	#
	my $vlan_number = $vlan_numbers{$vlan_id};
	if (!$vlan_number) {
	    warn "ERROR: VLAN $vlan_id not found on switch!";
	    return 0;
	}

	#
	# Prevent the VLAN from being sent across trunks.
	#
	if (!$self->setVlanOnTrunks($vlan_number,0)) {
	    warn "ERROR: Unable to set up VLANs on trunks!\n";
	    #
	    # We can keep going, 'cause we can still remove the VLAN
	    #
	}
	
    }

    #
    # Now, we go through each device and remove all ports from the VLAN
    # on that device. Note the reverse sort order! This way, we do not
    # interfere with another snmpit processes, since createVlan tries
    # in 'forward' order (we will remove the VLAN from the 'last' switch
    # first, so the other snmpit will not see it free until it's been
    # removed from all switches)
    #
    foreach my $devicename (sort {tbsort($b,$a)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}{$devicename};
	my @existant_vlans = ();
	my %vlan_numbers = $device->findVlans(@vlan_ids);
	foreach my $vlan_id (@vlan_ids) {

	    #
	    # Only remove ports from the VLAN if it exists on this
	    # device. Do it in one pass for efficiency
	    #
	    if (defined $vlan_numbers{$vlan_id}) {
		push @existant_vlans, $vlan_numbers{$vlan_id};
	    }
	}

	print "Removing ports on $devicename from VLANS " . 
	    join(",",@existant_vlans)."\n" if $self->{DEBUG};

	$errors += $device->removePortsFromVlan(@existant_vlans);

	#
	# If this stack doesn't use VTP, delete the VLAN, too, while
	# we're at it. If it does, we remove all VLANs down below (we can't
	# do it until the ports have been cleared from all switches.)
	#
	if (!$self->{VTP}) {
	    my $ok = $device->removeVlan(@existant_vlans);
	    if (!$ok) { $errors++; }
	}
    }

    if ($self->{VTP}) {
	#
	# For efficiency, we remove all VLANs from the leader in one function
	# call. This can save a _lot_ of locking and unlocking.
	#
	if (!$errors) {
	    #
	    # Make a list of all the VLANs that really did exist
	    #
	    my @vlan_numbers;
	    my ($key, $value);
	    while (($key, $value) = each %vlan_numbers) {
		if ($value) {
		    push @vlan_numbers, $value;
		}
	    }

	    my $ok = $self->{LEADER}->removeVlan(@vlan_numbers);
	    if (!$ok) {
		$errors++;
	    }
	}
    }

    return ($errors == 0);
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
# Get port statistics for all devices in the stack
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

#
# Not a 'public' function - only needs to get called by other functions in
# this file, not external functions.
#
# Enables or disables (depending on $value) a VLAN on all appropriate
# switches in a stack. Returns 1 on sucess, 0 on failure.
#
# ONLY pass in @ports if you're SURE that they are the only ports in the
# VLAN - basically, only if you just created it. This is a shortcut, so
# that we don't have to ask all switches if they have any ports in the VLAN.
#
sub setVlanOnTrunks($$$;@) {
    my $self = shift;
    my $vlan_number = shift;
    my $value = shift;
    my @ports = @_;

    #
    # First, get a list of all trunks
    #
    my %trunks = getTrunks();

    #
    # Next, figure out which switches this VLAN exists on
    #
    my @switches;
    if (@ports) {
	#
	# I'd rather not have to go out to the switches to ask which ones
	# have ports in the VLAN. So, if they gave me ports, I'll just 
	# trust that those are the only ones in the VLAN
	#
	@switches = getDeviceNames(@ports);
    } else {
	#
	# Since this may be a hand-created (not from the database) VLAN, the
	# only way we can figure out which swtiches this VLAN spans is to
	# ask them all.
	#
	foreach my $devicename (keys %{$self->{DEVICES}}) {
	    my $device = $self->{DEVICES}{$devicename};
	    foreach my $line ($device->listVlans()) {
		my ($vlan_id, $vlan, $memberRef) = @$line;
		if (($vlan == $vlan_number)){
		    push @switches, $devicename;
		}
	    }
	}
    }

    #
    # Next, get a list of the trunks that are used to move between these
    # switches
    #
    my @trunks = getTrunksFromSwitches(\%trunks,@switches);

    #
    # Now, we go through the list of trunks that need to be modifed, and
    # do it! We have to modify both ends of the trunk, or we'll end up wasting
    # the trunk bandwidth.
    #
    my $errors = 0;
    foreach my $trunk (@trunks) {
	my ($src,$dst) = @$trunk;
	if (!$self->{DEVICES}{$src}) {
	    warn "ERROR - Bad device $src found in setVlanOnTrunks!\n";
	    $errors++;
	} else {
	    #
	    # On ciscos, we can use any port in the trunk, so we'll use the
	    # first
	    #
	    my $modport = $trunks{$src}{$dst}[0];
	    $errors += !($self->{DEVICES}{$src}->setVlansOnTrunk($modport,
		    $value,$vlan_number));
	}
	if (!$self->{DEVICES}{$dst}) {
	    warn "ERROR - Bad device $dst found in setVlanOnTrunks!\n";
	    $errors++;
	} else {
	    #
	    # On ciscos, we can use any port in the trunk, so we'll use the
	    # first
	    #
	    my $modport = $trunks{$dst}{$src}[0];
	    $errors += !($self->{DEVICES}{$dst}->setVlansOnTrunk($modport,
		    $value,$vlan_number));
	}
    }

    return (!$errors);
}

# End with true
1;
