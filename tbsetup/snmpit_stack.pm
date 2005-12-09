#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2004, Regents, University of California.
# Modified from an Netbed/Emulab module, Copyright (c) 2000-2003, University of
# Utah
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
# the stack_id happens to also be the name of the stack leader.
#
# usage: new(string name, string stack_id, int debuglevel, list of devicenames)
# returns a new object blessed into the snmpit_stack class
#

sub new($$$@) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $stack_id = shift;
    my $debugLevel = shift;
    my @devicenames = @_;

    #
    # Create the actual object
    #
    my $self = {};
    my $device;

    #
    # Set up some defaults
    #
    if (defined $debugLevel) {
	$self->{DEBUG} = $debugLevel;
    } else {
	$self->{DEBUG} = 0;
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
	# We check the type for two reasons: to determine which kind of
	# object to create, and for some sanity checking to make sure
	# we weren't given devicenames for devices that aren't switches.
	#
	SWITCH: for ($type) {
	    (/cisco65\d\d/ || /cisco40\d\d/ || /cisco45\d\d/ || /cisco29\d\d/ || /cisco55\d\d/)
		    && do {
		use snmpit_cisco;
		$device = new snmpit_cisco($devicename,$self->{DEBUG});
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
	    die "Device $devicename is not of a known type, skipping\n";
	}
	unless ($device) {
		warn "WARNING: Couldn't create device object for $devicename\n";
		next;
	}
	$self->{DEVICES}{$devicename} = $device;
	if ($devicename eq $self->{LEADERNAME}) {
	    $self->{LEADER} = $device;
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

sub lock($) {
    my $self = shift;
    my $stackid = $self->{STACKID};
    my $token = "snmpit_$stackid";
    my $old_umask = umask(0);
    die if (TBScriptLock($token) != TBSCRIPTLOCK_OKAY());
    umask($old_umask);
}

sub unlock($) {
	TBScriptUnlock();
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
    my %mapping = $self->findVlans();
    my %map;

    #
    # Grab the VLAN number
    #
    my $vlan_number = $mapping{$vlan_id};
    if (!$vlan_number) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist!\n";
	return 1;
    }

    #
    # Split up the ports among the devices involved
    #
    %map = mapPortsToDevices(@ports);
    foreach my $devicename (keys %map) {
	my $device = $self->{DEVICES}{$devicename};
    	if (!defined($device)) {
    	    warn "Unable to find device entry for $devicename - some ports " .
	    	"will not be set up properly\n";
    	    $errors++;
    	    next;
    	}
	#
	# We might be adding ports to a switch which doesn't have this
	# VLAN yet . . .
	#
	if (!($device->findVlan($vlan_id)))
	    { $errors += !($device->createVlan($vlan_id,$vlan_number)); }
	$errors += $device->setPortVlan($vlan_number,@{$map{$devicename}});
    }

    $errors += (!$self->setVlanOnTrunks($vlan_number,1,@ports));

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

    $self->debug("stack::newVlanNumber $vlan_id\n");
    my %vlans = $self->findVlans();
    my $number = $vlans{$vlan_id};

    if (defined($number)) { return 0; }
    my @numbers = values %vlans;
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
    my $okay = 1;
    my %map;


    # We ignore other args for now, since generic stacks don't support
    # private VLANs and VTP;

    $self->lock();
    LOCKBLOCK: {
	#
	# We need to create the VLAN on all pertinent devices
	#
	my ($vlan_number, $res, $devicename, $device);
	$vlan_number = $self->newVlanNumber($vlan_id);
	if ($vlan_number == 0) { last LOCKBLOCK;}
	print "  Creating VLAN $vlan_id as VLAN #$vlan_number on " .
                 "$self->{STACKID} ... ";
	%map = mapPortsToDevices(@ports);
	foreach $devicename (sort {tbsort($a,$b)} keys %map) {
	    $device = $self->{DEVICES}{$devicename};
	    $res = $device->createVlan($vlan_id, $vlan_number);
	    if (!$res) {
		#
		# Ooops, failed. Don't try any more
		#
		$okay = 0;
		print " Failed\n";
		last;
	    }
	}

	#
	# We need to populate each VLAN on each switch.
	#
	$self->debug( "adding ports @ports to VLAN $vlan_id \n");
	if ($okay && @ports) {
	    if ($self->setPortVlan($vlan_id,@ports)) {
		$okay = 0;
	    }
	}
	print " Succeeded\n";

    }
    $self->unlock();
    return $okay;
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

    $self->debug("snmpit_stack::findVlans( @vlan_ids )\n",2);
    foreach $devicename (sort {tbsort($a,$b)} keys %{$self->{DEVICES}}) {
	$device = $self->{DEVICES}->{$devicename};
	my %dev_map = $device->findVlans(@vlan_ids);
	my ($id,$num,$oldnum);
	while (($id,$num) = each %dev_map) {
		if (defined($mapping{$id})) {
		    $oldnum = $mapping{$id};
		    if (defined($num) && ($num != $oldnum))
			{ warn "incompatible 802.1Q tag assignments for $id\n";}
		} else
		    { $mapping{$id} = $num; }
	}
	if (($count > 0) && ($count = scalar (keys %mapping)))
		{ return %mapping ;}
    }
    return %mapping;
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
# Turns on trunking on a given port, allowing only the given VLANs on it
#
# usage: enableTrunking(self, port, vlan identifier list)
#
# returns: 1 on success
# returns: 0 on failure
#
sub enableTrunking($$@) {
    my $self = shift;
    my $port = shift;
    my @vlan_ids = @_;

    #
    # On a Cisco, the first VLAN given becomes the native VLAN for the trunk
    #
    my $native_vlan_id = shift @vlan_ids;
    if (!$native_vlan_id) {
	print STDERR "ERROR: No native VLAN passed to enableTrunking()!\n";
	return 0;
    }

    #
    # Grab the VLAN number for the native VLAN
    #
    my $vlan_number = $self->{LEADER}->findVlan($native_vlan_id);
    if (!$vlan_number) {
	print STDERR "ERROR: Native VLAN $native_vlan_id does not exist!\n";
	return 0;
    }

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
    print "Enable trunking: Port is $port, native VLAN is $native_vlan_id\n"
	if ($self->{DEBUG});
    my $rv = $device->enablePortTrunking($port, $vlan_number);

    #
    # If other VLANs were given, add them to the port too
    #
    if (@vlan_ids) {
	my %vlan_numbers = $self->{LEADER}->findVlans(@vlan_ids);
	my @vlan_numbers;
	foreach my $vlan_id (@vlan_ids) {
	    #
	    # First, make sure that the VLAN really does exist
	    #
	    my $vlan_number = $vlan_numbers{$vlan_id};
	    if (!$vlan_number) {
		warn "ERROR: VLAN $vlan_id not found on switch!";
		next;
	    }
	    push @vlan_numbers, $vlan_number;
	}
	print "  add VLANs " . join(",",@vlan_numbers) . " to trunk\n"
	    if ($self->{DEBUG});
	if (!$device->setVlansOnTrunk($port,1,@vlan_numbers)) {
	    warn "ERROR: could not add VLANs " .
		join(",",@vlan_numbers) . " to trunk";
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

# End with true
1;
