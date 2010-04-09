#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for a stack of Cisco Catalyst 6509 switches. The main purpose
# of this module is to contain knowledge of how to manage stack-wide operations
# (such as VLAN creation), and to collate the results of listing operations
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
# usage: new(string name, string stack_id, int debuglevel, list of devicenames)
# returns a new object blessed into the snmpit_cisco_stack class
#
sub new($$$$$@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my $debuglevel = shift;
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
    # The ID of this stack
    # 
    $self->{STACKID} = $stack_id;

    #
    # The name of the leader of this stack. We fall back on the old behavior of
    # using the stack name as the leader if the leader is not set
    #
    my $leader_name = getStackLeader($stack_id);
    if (!$leader_name) {
	$leader_name = $stack_id;
    }
    $self->{LEADERNAME} = $leader_name;

    #
    # Store the list of devices we're supposed to operate on
    #
    @{$self->{DEVICENAMES}} = @devicenames;

    #
    # Whether or not this stack uses VTP to keep the VLANs synchronized
    #
    $self->{VTP} = $uses_vtp;

    #
    # We'll use this to store any special arguments for VLAN creation
    #
    $self->{VLAN_SPECIALARGS} = {};

    #
    # Set by default now - don't create VLANs on switches that don't need
    # them. We always create VLANs on the leader, so that it can be used as a
    # sort of global list of what VLANs are available, and is atomically
    # lockable.
    #
    $self->{PRUNE_VLANS} = 1;

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
	    (/65\d\d/ || /40\d\d/ || /45\d\d/ || /29\d\d/ || /55\d\d/ || /35\d\d/)
		    && do {
		use snmpit_cisco;
		$device = new snmpit_cisco($devicename,$self->{DEBUG});
		if (!$device) {
		    die "Failed to create a device object for $devicename\n";
		} else {
		    $self->{DEVICES}{$devicename} = $device;
		    if ($devicename eq $self->{LEADERNAME}) {
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
	my $type = getDeviceType($self->{LEADERNAME});
	$self->{LEADER} = new snmpit_cisco($self->{LEADERNAME}, $self->{DEBUG});
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
    # We need to 'collate' the results from each switch by putting together
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
    # All we really need to do here is collate the results of listing the
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
    my %trunks;
    my @trunks;

    if ($self->{PRUNE_VLANS}) {
        #
        # Use this hash like a set to find out what switches might be involved
        # in this VLAN
        #
        my %switches;
        foreach my $switch (keys %map) {
            $switches{$switch} = 1;
        }

	#
	# Find out which ports are already in this VLAN - we might be adding
        # to an existing VLAN
        # TODO - provide a way to bypass this check for new VLANs, to speed
        # things up a bit
	#
        foreach my $switch ($self->switchesWithPortsInVlan($vlan_number)) {
            $switches{$switch} = 1;
        }

        #
        # Find out every switch which might have to transit this VLAN through
        # its trunks
        #
        %trunks = getTrunks();
        @trunks = getTrunksFromSwitches(\%trunks, keys %switches);
        foreach my $trunk (@trunks) {
            my ($src,$dst) = @$trunk;
            $switches{$src} = $switches{$dst} = 1;
        }

        #
        # Create the VLAN (if it doesn't exist) on every switch involved
        #
        foreach my $switch (keys %switches) {
            #
            # Check to see if the VLAN already exists on this switch
            #
            my $dev = $self->{DEVICES}{$switch};
            if (!$dev) {
                warn "ERROR: VLAN uses switch $switch, which is not in ".
                    "this stack\n";
                $errors++;
                next;
            }
            if ($dev->vlanNumberExists($vlan_number)) {
                if ($self->{DEBUG}) {
                    print "Vlan $vlan_id already exists on $dev\n";
                }
            } else {
                #
                # Check to see if we had any special arguments saved up for
                # this VLAN
                #
                my @otherargs = ();
                if (exists $self->{VLAN_SPECIALARGS}{$vlan_id}) {
                    @otherargs = @{$self->{VLAN_SPECIALARGS}{$vlan_id}}
                }

                #
                # Create the VLAN
                #
                my $res = $dev->createVlan($vlan_id,$vlan_number);
                if ($res == 0) {
                    warn "Error: Failed to create VLAN $vlan_id ($vlan_number)".
                         " on $switch\n";
                    $errors++;
                    next;
                }
            }
        }
    }

    my %BumpedVlans = ();
    #
    # Perform the operation on each switch
    #
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

	#
	# When making firewalls, may have to flush FDB entries from trunks
	#
	if (defined($device->{DISPLACED_VLANS})) {
	    foreach my $vlan (@{$device->{DISPLACED_VLANS}}) {
		$BumpedVlans{$vlan} = 1;
	    }
	    $device->{DISPLACED_VLANS} = undef;
	}
    }

    if ($vlan_id ne 'default') {
	# if PRUNE were not in effect, then %trunks and @trunks
	# aren't valid.  If VTP weren't enforced however,
	# merely calling setVlanOnTrunks() with just the specified
	# ports would miss transit-only switches.
	# maybe the phrase at the top should be if (PRUNE || !VTP ..)
	# expecially if running VTP automatically creates the interswitch
	# trunks.
	# I'll let Rob R. decide what to do about that -- sklower.
	$errors += (!$self->setVlanOnTrunks2($vlan_number,1,\%trunks,@trunks));
    }

    #
    # When making firewalls, may have to flush FDB entries from trunks
    #
    foreach my $vlan (keys %BumpedVlans) {
	foreach my $devicename ($self->switchesWithPortsInVlan($vlan)) {
	    my $dev = $self->{DEVICES}{$devicename};
	    foreach my $neighbor (keys %{$trunks{$devicename}}) {
		my $trunkIndex = $dev->getChannelIfIndex(
				    @{$trunks{$devicename}{$neighbor}});
		if (!defined($trunkIndex)) {
		    warn "unable to find channel information on $devicename ".
			 "for $devicename-$neighbor EtherChannel\n";
		    $errors += 1;
		} else {
		    $dev->resetVlanIfOnTrunk($trunkIndex,$vlan);
		}
	    }
	}
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

    if (@otherargs) {
        $self->{VLAN_SPECIALARGS}{$vlan_id} = @otherargs;
    }

    #
    # What we do here depends on whether this stack uses VTP to synchronize
    # VLANs or not
    #
    my $vlan_number;

    # XXX temp, see doMakeVlans in snmpit.in
    if ($::next_vlan_tag)
	{ $vlan_number = $::next_vlan_tag; $::next_vlan_tag = 0; }

    if ($self->{VTP} || $self->{PRUNE_VLANS}) {
	#
	# We just need to create the VLAN on the stack leader
	#
	#
	$vlan_number = $self->{LEADER}->createVlan($vlan_id,$vlan_number,@otherargs);
    } else {
	#
	# We need to create the VLAN on all devices
	# XXX - should we do the leader first?
	#
	foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}){
	    print "Creating VLAN on switch $devicename ... \n"
		if $self->{DEBUG};
	    my $device = $self->{DEVICES}{$devicename};
	    my $res = $device->createVlan($vlan_id,$vlan_number,@otherargs);
	    if (!$res) {
		#
		# Ooops, failed. Don't try any more
		#
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
    if ($vlan_number && @ports) {
	if ($self->setPortVlan($vlan_id,@ports)) {
	    print STDERR "*** Failed to add ports to vlan\n";
	}
    }
    return return $vlan_number;
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
# Given VLAN indentifiers from the database, finds the 802.1Q VLAN
# number for them. If no VLAN id is given, returns mappings for the entire
# stack.
#
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to 802.1Q VLAN numbers
#
#
sub findVlans($@) {
    my $self = shift;
    my @vlan_ids = @_;
    my %mapping = ();

    #
    # For now, on Cisco, we only need to do this on the leader
    #
    return $self->{LEADER}->findVlans(@vlan_ids);
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
	    warn "ERROR: Unable to remove VLAN $vlan_number from trunks!\n";
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
    my $vlan_removal_errors = 0;
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
	# If there have been any errors removing VLANs from other switches,
	# we don't continue trying to remove them - this is in an attempt to
	# avoid ending up with VLANs that exist on switches in the stack but
	# NOT the 'master' (the first one we try to create them on)
	#
	if (!$self->{VTP} && !$vlan_removal_errors) {
	    my $ok = $device->removeVlan(@existant_vlans);
	    if (!$ok) { $errors++; $vlan_removal_errors++; }
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
# Remove some ports from a single vlan,
#
# usage: removeSomePortsFromVlan(self, vlan identifier, port list)
#
# returns: 1 on success
# returns: 0 on failure
#
sub removeSomePortsFromVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;
    my $errors = 0;

    my %vlan_numbers = $self->{LEADER}->findVlans($vlan_id);
    
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
	warn "ERROR: Unable to remove VLAN $vlan_number from trunks!\n";
	#
	# We can keep going, 'cause we can still remove the VLAN
	#
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
	my %vlan_numbers = $device->findVlans($vlan_id);

	#
	# Only remove ports from the VLAN if it exists on this device.
	#
	next
	    if (!defined($vlan_numbers{$vlan_id}));
	    
	print "Removing ports on $devicename from VLAN $vlan_id\n"
	    if $self->{DEBUG};

	$errors += $device->removeSomePortsFromVlan($vlan_id, @ports);
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
    # All we really need to do here is collate the results of listing the
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
# Turns on trunking on a given port, allowing only the given VLANs on it
#
# usage: enableTrunking2(self, port, equaltrunking, vlan identifier list)
#
# formerly was enableTrunking() without the predicate to decline to put
# the port in dual mode.
#
# returns: 1 on success
# returns: 0 on failure
#
sub enableTrunking2($$$@) {
    my $self = shift;
    my $port = shift;
    my $equaltrunking = shift;
    my @vlan_ids = @_;

    #
    # Split up the ports among the devices involved
    #
    my %map = mapPortsToDevices($port);
    my ($devicename) = keys %map;
    my $device = $self->{DEVICES}{$devicename};
    if (!defined($device)) {
	warn "ERROR: Unable to find device entry for $devicename\n";
	return 0;
    }
    #
    # If !equaltrunking, the first VLAN given becomes the PVID for the trunk
    #
    my ($native_vlan_id, $vlan_number);
    if ($equaltrunking) {
	$native_vlan_id = "default";
	$vlan_number = 1;
    } else {
	$native_vlan_id = shift @vlan_ids;
	if (!$native_vlan_id) {
	    warn "ERROR: No VLAN passed to enableTrunking()!\n";
	    return 0;
	}
	#
	# Grab the VLAN number for the native VLAN
	#
	$vlan_number = $device->findVlan($native_vlan_id);
	if (!$vlan_number) {
	    warn "Native VLAN $native_vlan_id was not on $devicename";
	    # This is painful
	    my $error = $self->setPortVlan($native_vlan_id,$port);
	    if ($error) {
		    warn ", and couldn't add it\n"; return 0;
	    } else { warn "\n"; } 
	}
    }

    #
    # Simply make the appropriate call on the device
    #
    print "Enable trunking: Port is $port, native VLAN is $native_vlan_id\n"
	if ($self->{DEBUG});
    my $rv = $device->enablePortTrunking2($port, $vlan_number, $equaltrunking);

    #
    # If other VLANs were given, add them to the port too
    #
    if (@vlan_ids) {
	my @vlan_numbers;
	foreach my $vlan_id (@vlan_ids) {
	    #
	    # setPortVlan makes sure that the VLAN really does exist
	    # and will set up intervening trunks as well as adding the
	    # trunked port to that VLAN
	    #
	    my $error = $self->setPortVlan($vlan_id,$port);
	    if ($error) {
		warn "ERROR: could not add VLAN $vlan_id to trunk $port\n";
		next;
	    }
	    push @vlan_numbers, $vlan_number;
	}
	print "  add VLANs " . join(",",@vlan_numbers) . " to trunk\n"
	    if ($self->{DEBUG});
    }

    return $rv;

}

#
# Turns off trunking for a given port
#
# usage: disableTrunking(self, ports)
#
# returns: 1 on success
# returns: 0 on failure
#
sub disableTrunking($$) {
    my $self = shift;
    my $port = shift;

    #
    # Split up the ports among the devices involved
    #
    my %map = mapPortsToDevices($port);
    my ($devicename) = keys %map;
    my $device = $self->{DEVICES}{$devicename};
    if (!defined($device)) {
	warn "ERROR: Unable to find device entry for $devicename\n";
	return 0;
    }

    #
    # Simply make the appropriate call on the device
    #
    my $rv = $device->disablePortTrunking($port);

    return $rv;

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
        @switches = $self->switchesWithPortsInVlan($vlan_number);
    }

    #
    # Next, get a list of the trunks that are used to move between these
    # switches
    #
    my @trunks = getTrunksFromSwitches(\%trunks,@switches);
    return $self->setVlanOnTrunks2($vlan_number,$value,\%trunks,@trunks);
}
#
# Enables or disables (depending on $value) a VLAN on all the supplied
# trunks. Returns 1 on sucess, 0 on failure.
#
sub setVlanOnTrunks2($$$$@) {
    my $self = shift;
    my $vlan_number = shift;
    my $value = shift;
    my $trunkref = shift;
    my @trunks = @_;

    #
    # Now, we go through the list of trunks that need to be modifed, and
    # do it! We have to modify both ends of the trunk, or we'll end up wasting
    # the trunk bandwidth.
    #
    my $errors = 0;
    foreach my $trunk (@trunks) {
	my ($src,$dst) = @$trunk;

        #
        # For now - ignore trunks that leave the stack. We may have to revisit
        # this at some point.
        #
        if (!$self->{DEVICES}{$src} || !$self->{DEVICES}{$dst}) {
            next;
        }

	if (!$self->{DEVICES}{$src}) {
	    warn "ERROR - Bad device $src found in setVlanOnTrunks!\n";
	    $errors++;
	} else {
	    #
	    # Trunks might be EtherChannels, find the ifIndex
	    #
            my $trunkIndex = $self->{DEVICES}{$src}->
                             getChannelIfIndex(@{ $$trunkref{$src}{$dst} });
            if (!defined($trunkIndex)) {
                warn "ERROR - unable to find channel information on $src ".
		     "for $src-$dst EtherChannel\n";
                $errors += 1;
            } else {
		if (!$self->{DEVICES}{$src}->
                        setVlansOnTrunk($trunkIndex,$value,$vlan_number)) {
                    warn "ERROR - unable to set trunk on switch $src\n";
                    $errors += 1;
                }
	    }
	}
	if (!$self->{DEVICES}{$dst}) {
	    warn "ERROR - Bad device $dst found in setVlanOnTrunks!\n";
	    $errors++;
	} else {
	    #
	    # Trunks might be EtherChannels, find the ifIndex
	    #
            my $trunkIndex = $self->{DEVICES}{$dst}->
                             getChannelIfIndex(@{ $$trunkref{$dst}{$src} });
            if (!defined($trunkIndex)) {
                warn "ERROR - unable to find channel information on $dst ".
		     "for $src-$dst EtherChannel\n";
                $errors += 1;
            } else {
		if (!$self->{DEVICES}{$dst}->
                        setVlansOnTrunk($trunkIndex,$value,$vlan_number)) {
                    warn "ERROR - unable to set trunk on switch $dst\n";
                    $errors += 1;
                }
	    }
	}
    }

    return (!$errors);
}

#
# Not a 'public' function - only needs to get called by other functions in
# this file, not external functions.
#
# Get a list of all switches that have at least one port in the given
# VLAN - note that is take a VLAN number, not a VLAN ID
#
# Returns a possibly-empty list of switch names
#
sub switchesWithPortsInVlan($$) {
    my $self = shift;
    my $vlan_number = shift;
    my @switches = ();
    foreach my $devicename (keys %{$self->{DEVICES}}) {
        my $device = $self->{DEVICES}{$devicename};
	if ($device->vlanHasPorts($vlan_number)) {
	    push @switches, $devicename;
        }
    }
    return @switches;
}

#
# Openflow enable function for Cisco stack
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
sub getUsedOpenflowListenerPorts($$) {
    my $self = shift;
    my $ports = shift;

    foreach my $devicename (keys %{$self->{DEVICES}})
    {
	my $device = $self->{DEVICES}{$devicename};
	$device->getUsedOpenflowListenerPorts($ports);
    }
}

# End with true
1;
