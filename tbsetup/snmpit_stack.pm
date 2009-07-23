#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# Copyright (c) 2004-2009 Regents, University of California.
# All rights reserved.
#

package snmpit_stack;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libdb;
use libtestbed;

#
# Creates a new object. A list of devices that will be operated on is given
# so that the object knows which to connect to. A future version may not 
# require the device list, and dynamically connect to devices as appropriate
#
# usage: new(string name, string stack_id, int debuglevel, list of devicenames)
# returns a new object blessed into the snmpit_stack class
#

sub new($$$$@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my $debuglevel = shift;
    my $quiet = shift;
    my @devicenames = @_;

    #
    # Create the actual object
    #
    my $self = {};
    my $device;

    #
    # Set up some defaults
    #
    if (defined $debuglevel) {
	$self->{DEBUG} = $debuglevel;
    } else {
	$self->{DEBUG} = 0;
    }
    if (defined $quiet) {
	$self->{QUIET} = $quiet;
    } else {
	$self->{QUIET} = 0;
    }

    $self->{STACKID} = $stack_id;
    $self->{MAX_VLAN} = 4095;
    $self->{MIN_VLAN} = 2;
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

    # The following line will let snmpit_stack be used interchangeably
    # with snmpit_cisco_stack
    # $self->{ALLVLANSONLEADER} = 1;
    $self->{ALLVLANSONLEADER} = 0;

    #
    # The following two lines will let us inherit snmpit_cisco_stack
    # (someday), (and can help pare down lines of diff in the meantime)
    #
    $self->{PRUNE_VLANS} = 1;
    $self->{VTP} = 0;


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
	    warn "WARNING: Device $devicename was specified twice, skipping\n";
	    next;
	}

	#
	# We check the type for two reasons: to determine which kind of
	# object to create, and for some sanity checking to make sure
	# we weren't given devicenames for devices that aren't switches.
	#
	SWITCH: for ($type) {
	    (/cisco/) && do {
		use snmpit_cisco;
		$device = new snmpit_cisco($devicename,
					   $self->{DEBUG},$self->{QUIET});
		last;
		}; # /cisco/
	    (/foundry1500/ || /foundry9604/)
		    && do {
		use snmpit_foundry;
		$device = new snmpit_foundry($devicename,$self->{DEBUG});
		last;
		}; # /foundry.*/
	    (/nortel1100/ || /nortel5510/)
		    && do {
		use snmpit_nortel;
		$device = new snmpit_nortel($devicename,$self->{DEBUG});
		last;
		}; # /nortel.*/
	    (/hp/)
		    && do {
		use snmpit_hp;
		$device = new snmpit_hp($devicename,$self->{DEBUG});
		last;
		}; # /hp.*/
	    die "Device $devicename is not of a known type, skipping\n";
	}

		# indented to minimize diffs
		if (!$device) {
		    die "Failed to create a device object for $devicename\n";
		} else {
		    $self->{DEVICES}{$devicename} = $device;
		    if ($devicename eq $self->{LEADERNAME}) {
			$self->{LEADER} = $device;
		    }
		}

	if (defined($device->{MIN_VLAN}) &&
	    ($self->{MIN_VLAN} < $device->{MIN_VLAN}))
		{ $self->{MIN_VLAN} = $device->{MIN_VLAN}; }
	if (defined($device->{MAX_VLAN}) &&
	    ($self->{MAX_VLAN} > $device->{MAX_VLAN}))
		{ $self->{MAX_VLAN} = $device->{MAX_VLAN}; }

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
#	array is in the form [id, num, members] where:
#		id is the VLAN identifier, as stored in the database
#		num is the 802.1Q vlan tag number.
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
    my $vlan_number = $self->findVlan($vlan_id);
    if (!$vlan_number) {
	print STDERR
	"ERROR: VLAN with identifier $vlan_id does not exist on stack " .
	$self->{STACKID} . "\n" ;
	return 1;
    }

    #
    # Split up the ports among the devices involved
    #
    my %map = mapPortsToDevices(@ports);
    my %trunks = getTrunks();
    my @trunks;

    if (1) {
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
                    print "Vlan $vlan_id already exists on $switch\n";
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
# Allocate a vlan number currently not in use on the stack.
# Is called from createVlan, here for  clarity and to
# lower the lines of diff from snmpit_cisco_stack.pm
#
# usage: newVlanNumber(self, vlan_identifier)
#
# returns a number in $self->{VLAN_MIN} ... $self->{VLAN_MAX}
# or zero indicating failure: either that the id exists,
# or the number space is full.
#
sub newVlanNumber($$) {
    my $self = shift;
    my $vlan_id = shift;
    my %vlans;

    $self->debug("stack::newVlanNumber $vlan_id\n");
    if ($self->{ALLVLANSONLEADER}) {
	%vlans = $self->{LEADER}->findVlans();
    } else {
	%vlans = $self->findVlans();
    }
    my $number = $vlans{$vlan_id};

    if (defined($number)) { return 0; }
    my @numbers = sort values %vlans;
    $self->debug("newVlanNumbers: numbers ". "@numbers" . " \n");
    $number = $self->{MIN_VLAN}-1;
    my $lim = $self->{MAX_VLAN};
    do { ++$number }
	until (!(grep {$_ == $number} @numbers) || ($number > $lim));
    return $number <= $lim ? $number : 0;
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
    my $vlan_number;
    my %map;
    my $errortype = "Creating";


    # We ignore other args for now, since generic stacks don't support
    # private VLANs and VTP;

    $self->lock();
    LOCKBLOCK: {
	#
	# We need to create the VLAN on all pertinent devices
	#
	my ($res, $devicename, $device);
	$vlan_number = $self->newVlanNumber($vlan_id);
	if ($vlan_number == 0) { last LOCKBLOCK;}
	print "Creating VLAN $vlan_id as VLAN #$vlan_number on stack " .
                 "$self->{STACKID} ... \n" if (! $self->{QUIET});
	if ($self->{ALLVLANSONLEADER}) {
		$res = $self->{LEADER}->createVlan($vlan_id, $vlan_number);
		$self->unlock();
		if (!$res) { goto failed; }
	}
	%map = mapPortsToDevices(@ports);
	foreach $devicename (sort {tbsort($a,$b)} keys %map) {
	    if ($self->{ALLVLANSONLEADER} &&
		($devicename eq $self->{LEADERNAME})) { next; }
	    $device = $self->{DEVICES}{$devicename};
	    $res = $device->createVlan($vlan_id, $vlan_number);
	    if (!$res) {
		goto failed;
	    }
	}

	#
	# We need to populate each VLAN on each switch.
	#
	$self->debug( "adding ports @ports to VLAN $vlan_id \n");
	if (@ports) {
	    if ($self->setPortVlan($vlan_id,@ports)) {
		$errortype = "Adding Ports to";
	    failed:
		#
		# Ooops, failed. Don't try any more
		#
		if ($self->{QUIET}) {
		    print "$errortype VLAN $vlan_id as VLAN #$vlan_number on ".
			"stack $self->{STACKID} ... \n";
		}
		print "Failed\n";
		$vlan_number = 0;
		last LOCKBLOCK;
	    }
	}
	print "Succeeded\n" if (! $self->{QUIET});

    }
    $self->unlock();
    return $vlan_number;
}

#
# Given VLAN indentifiers from the database, finds the 802.1Q VLAN
# number for them. If no VLAN id is given, returns mappings for the entire
# switch.
# 
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to 802.1Q VLAN numbers
#
sub findVlans($@) {
    my $self = shift;
    my @vlan_ids = @_;
    my ($count, $device, $devicename) = (scalar(@vlan_ids));
    my %mapping = ();

    $self->debug("snmpit_stack::findVlans( @vlan_ids )\n");
    foreach $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	$self->debug("stack::findVlans calling $devicename\n");
	$device = $self->{DEVICES}->{$devicename};
	my %dev_map = $device->findVlans(@vlan_ids);
	my ($id,$num,$oldnum);
	while (($id,$num) = each %dev_map) {
		if (defined($mapping{$id})) {
		    $oldnum = $mapping{$id};
		    if (defined($num) && ($num != $oldnum))
			{ warn "Incompatible 802.1Q tag assignments for $id\n" .
                               "    Saw $num on $device->{NAME}, but had " .
                               "$oldnum before\n";}
		} else
		    { $mapping{$id} = $num; }
	}
#	if (($count > 0) && ($count == scalar (values %mapping))) {
#		my @k = keys %mapping; my @v = values %mapping;
#		$self->debug("snmpit_stack::findVlans would bail here"
#		 . " k = ( @k ) , v = ( @v )\n");
#	}
    }
    return %mapping;
}

#
# Given a single VLAN indentifier, find the 802.1Q VLAN tag for it. 
# 
# usage: findVlan($self, $vlan_id)
#        returns the number if found
#        0 otherwise;
#
sub findVlan($$) {
    my ($self, $vlan_id) = @_;

    $self->debug("snmpit_stack::findVlan( $vlan_id )\n");
    foreach my $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	my $device = $self->{DEVICES}->{$devicename};
	my %dev_map = $device->findVlans($vlan_id);
	my $vlan_num = $dev_map{$vlan_id};
	if (defined($vlan_num)) { return $vlan_num; }
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
    my %mapping = $self->findVlans();

    if (defined($mapping{$vlan_id})) {
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

    my %mapping = $self->findVlans(@vlan_ids);

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

    my %vlan_numbers = $self->findVlans(@vlan_ids);
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
    LOOP: foreach my $devicename (sort {tbsort($b,$a)} keys %{$self->{DEVICES}})
    {
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
	next LOOP if (scalar(@existant_vlans) == 0);

	print "Removing ports on $devicename from VLANS " . 
	    join(",",@existant_vlans)."\n" if $self->{DEBUG};

	$errors += $device->removePortsFromVlan(@existant_vlans);

	#
	# Since mixed stacks doesn't use VTP, delete the VLAN, too.
	#
	my $ok = $device->removeVlan(@existant_vlans);
	if (!$ok) { $errors++; }
    }

    return ($errors == 0);
}

#
# Remove some ports from a single vlan. Ports should not be in trunk mode.
#
# usage: removeSomePortsFromVlan(self, vlanid, portlist)
#
# returns: 1 on success
# returns: 0 on failure
#
sub removeSomePortsFromVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;
    my $errors = 0;
    
    my %vlan_numbers = $self->findVlans($vlan_id);
    
    #
    # First, make sure that the VLAN really does exist
    #
    if (!exists($vlan_numbers{$vlan_id})) {
	warn "ERROR: VLAN $vlan_id not found on switch!";
	return 0;
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

	my $vlan_number = $vlan_numbers{$vlan_id};
	    
	print "Removing ports on $devicename from VLAN $vlan_id ($vlan_number)\n"
	    if $self->{DEBUG};

	$errors += $device->removeSomePortsFromVlan($vlan_number, @ports);
    }

    return ($errors == 0);
}

#
# Remove some ports from a single trunk.
#
# usage: removeSomePortsFromTrunk(self, vlanid, portlist)
#
# returns: 1 on success
# returns: 0 on failure
#
sub removeSomePortsFromTrunk($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;
    my $errors = 0;
    
    my %vlan_numbers = $self->findVlans($vlan_id);
    
    #
    # First, make sure that the VLAN really does exist
    #
    if (!exists($vlan_numbers{$vlan_id})) {
	warn "ERROR: VLAN $vlan_id not found on switch!";
	return 0;
    }

    #
    # Now, we go through each device and remove all ports from the trunk
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

	my $vlan_number = $vlan_numbers{$vlan_id};
	    
	print "Removing ports on $devicename from VLAN $vlan_id ($vlan_number)\n"
	    if $self->{DEBUG};

	foreach my $port (@ports) {
	    return 0
		if (! $device->setVlanOnTrunks($port, 0, $vlan_number));
	}
    }

    return 1;
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
# PS By sklower since this is only used in adding ports or even more
# interswitch trunks, no harm is done if we ask if the the vlan exists
# (we erroneously include interswitch trunks).  doing a listVlans() is a
# VERY expensive operation.

sub switchesWithPortsInVlan($$) {
    my $self = shift;
    my $vlan_number = shift;
    my @switches = ();
    foreach my $devicename (keys %{$self->{DEVICES}}) {
        my $dev = $self->{DEVICES}{$devicename};
	if ($dev->vlanNumberExists($vlan_number)) {
	    push @switches, $devicename;
        }
    }
    return @switches;
}

#
# Prints out a debugging message, but only if debugging is on. If a level is
# given, the debuglevel must be >= that level for the message to print. If
# the level is omitted, 1 is assumed
#
# Usage: debug($self, $message, $level)
#
sub debug($$;$) {
    my $self = shift;
    my $string = shift;
    my $debuglevel = shift;
   if (!(defined $debuglevel)) {
	$debuglevel = 1;
    }
    if ($self->{DEBUG} >= $debuglevel) {
	print STDERR $string;
    }
}

my $lock_held = 0;

sub lock($) {
    my $self = shift;
    my $stackid = $self->{STACKID};
    my $token = "snmpit_$stackid";
    my $old_umask = umask(0);
    die if (TBScriptLock($token,0,1800) != TBSCRIPTLOCK_OKAY());
    umask($old_umask);
    $lock_held = 1;
}

sub unlock($) {
	if ($lock_held) { TBScriptUnlock(); $lock_held = 0;}
}


# End with true
1;
