#!/usr/bin/perl -w
#
# snmpit module for Cisco Catalyst 6509 switches
#
# TODO: Standardize returning 0 on success/failure
# TODO: Fix uninitialized variable warnings in getStats()
#

package snmpit_cisco;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

#
# These are the commands that can be passed to the portControl function
# below
#
my %cmdOIDs =
(
    "enable" => ["ifAdminStatus","up"],
    "disable"=> ["ifAdminStatus","down"],
    "100mbit"=> ["portAdminSpeed","s100000000"],
    "10mbit" => ["portAdminSpeed","s10000000"],
    "full"   => ["portDuplex","full"],
    "half"   => ["portDuplex","half"],
    "auto"   => ["portAdminSpeed","autoDetect",
		 "portDuplex","auto"]
);

#
# Ports can be passed around in three formats:
# ifindex: positive integer corresponding to the interface index (eg. 42)
# modport: dotted module.port format, following the physical reality of
#	Cisco switches (eg. 5.42)
# nodeport: node:port pair, referring to the node that the switch port is
# 	connected to (eg. "pc42:1")
#
# See the function convertPortFormat below for conversions between these
# formats
#
my $PORT_FORMAT_IFINDEX  = 1;
my $PORT_FORMAT_MODPORT  = 2;
my $PORT_FORMAT_NODEPORT = 3;

#
# Creates a new object.
#
# usage: new($classname,$devicename,$debuglevel)
#        returns a new object, blessed into the snmpit_cisco class.
#
sub new($$;$) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $name = shift;
    my $debugLevel = shift;

    #
    # Create the actual object
    #
    my $self = {};

    #
    # Set the defaults for this object
    # 
    if (defined($debugLevel)) {
	$self->{DEBUG} = $debugLevel;
    } else {
	$self->{DEBUG} = 0;
    }
    $self->{BLOCK} = 1;
    $self->{NAME} = $name;

    if ($self->{DEBUG}) {
	print "snmpit_cisco module initializing... debug level $self->{DEBUG}\n";
    }

    #
    # Set up SNMP module variables, and connect to the device
    #
    $SNMP::debugging = ($self->{DEBUG} - 2) if $self->{DEBUG} > 2;
    my $mibpath = '/usr/local/share/snmp/mibs';
    &SNMP::addMibDirs($mibpath);
    &SNMP::addMibFiles("$mibpath/CISCO-STACK-MIB.txt",
	"$mibpath/CISCO-VTP-MIB.txt",
	"$mibpath/CISCO-VLAN-MEMBERSHIP-MIB.txt");
    
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		  # parses default list of Mib modules 
    $SNMP::use_enums = 1;	  # use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});
    $self->{SESS} =
	    new SNMP::Session(DestHost => $self->{NAME},Version => "2c");
    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "ERROR: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }

    #
    # The bless needs to occur before readifIndex(), since it's a class 
    # method
    #
    bless($self,$class);

    $self->readifIndex();

    return $self;
}

#
# Set a variable associated with a port. The commands to execute are given
# in the cmdOIs hash above
#
# usage: portControl($self, $command, @ports)
#	 returns 0 on success.
#	 returns number of failed ports on failure.
#	 returns -1 if the operation is unsupported
#
sub portControl ($$@) {
    my $self = shift;

    my $cmd = shift;
    my @ports = @_;

    $self->debug("portControl: $cmd -> (@ports)\n");

    #
    # Find the command in the %cmdOIDs hash (defined at the top of this file)
    #
    if (defined $cmdOIDs{$cmd}) {
	my @oid = @{$cmdOIDs{$cmd}};
	my $errors = 0;

	#
	# Convert the ports from the format they were given in to the format
	# required by the command
	#
	my $portFormat;
	if ($cmd =~ /(en)|(dis)able/) {
	    $portFormat = $PORT_FORMAT_IFINDEX;
	} else { 
	    $portFormat = $PORT_FORMAT_MODPORT;
	}
	my @portlist = $self->convertPortFormat($portFormat,@ports);

	#
	# Some commands involve multiple SNMP commands, so we need to make
	# sure we get all of them
	#
	while (@oid) {
	    my $myoid = shift @oid;
	    my $myval = shift @oid;
	    $errors += $self->UpdateField($myoid,$myval,@portlist);
	}
	return $errors;

    } else {
	#
	# Command not supported
	#
	print STDERR "Unsupported port control command '$cmd' ignored.\n";
	return -1;
    }
}

#
# Convert a set of ports to an alternate format. The input format is detected
# automatically. See the declarations of the constants at the top of this
# file for a description of the different port formats.
#
# usage: convertPortFormat($self, $output format, @ports)
#        returns a list of ports in the specified output format
#        returns undef if the output format is unknown
#
# TODO: Add debugging output, better comments, more sanity checking
#
sub convertPortFormat($$@) {
    my $self = shift;
    my $output = shift;
    my @ports = @_;


    #
    # Avoid warnings by exiting if no ports given
    # 
    if (!@ports) {
	return ();
    }

    #
    # We determine the type by sampling the first port given
    #
    my $sample = $ports[0];
    if (!$sample) {
	warn "convertPortFormat: Given a bad list of ports\n";
	return undef;
    }

    my $input;
    SWITCH: for ($sample) {
	(/^\d+$/) && do { $input = $PORT_FORMAT_IFINDEX; last; };
	(/^\d+\.\d+$/) && do { $input = $PORT_FORMAT_MODPORT; last; };
	$input = $PORT_FORMAT_NODEPORT; last;
    }

    #
    # It's possible the ports are already in the right format
    #
    if ($input == $output) {
	return @ports;
    }

    # Shark hack
    @ports = map {if (/(sh\d+)-\d(:\d)/) { "$1$2" } else { $_ }} @ports;
    # End shark hack

    if ($input == $PORT_FORMAT_IFINDEX) {
	if ($output == $PORT_FORMAT_MODPORT) {
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    return map portnum($self->{NAME}.":".$self->{IFINDEX}{$_}), @ports;
	}
    } elsif ($input == $PORT_FORMAT_MODPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    return map portnum($self->{NAME} . $_), @ports;
	}
    } elsif ($input == $PORT_FORMAT_NODEPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    return map $self->{IFINDEX}{(split /:/,portnum($_))[1]}, @ports;
	} elsif ($output == $PORT_FORMAT_MODPORT) {
	    return map { (split /:/,portnum($_))[1] } @ports;
	}
    }

    #
    # Some combination we don't know how to handle
    #
    warn "convertPortFormat: Bad input/output combination ($input/$output)\n";
    return undef;

}

#
# Obtain a lock on the VLAN edit buffer. This must be done before VLANS
# are created or removed. Will retry 5 times before failing
#
# usage: vlanLock($self)
#        returns 1 on success
#        returns 0 on failure
#
sub vlanLock($) {
    my $self = shift;

    my $EditOp = 'vtpVlanEditOperation'; # use index 1
    my $BufferOwner = 'vtpVlanEditBufferOwner'; # use index 1

    #
    # Try max_tries times before we give up, in case some other process just
    # has it locked.
    #
    my $tries = 1;
    my $max_tries = 20;
    while ($tries <= $max_tries) {
    
	#
	# Attempt to grab the edit buffer
	#
	my $grabBuffer = $self->{SESS}->set([$EditOp,1,"copy","INTEGER"]);

	#
	# Check to see if we were sucessful
	#
	$self->debug("Buffer Request Set gave " .
		(defined($grabBuffer)?$grabBuffer:"undef.") . "\n");
	if (! $grabBuffer) {
	    #
	    # Only print this message every five tries
	    #
	    if (!($tries % 5)) {
		print STDERR "VLAN edit buffer request failed - " .
			     "try $tries of $max_tries.\n";
	    }
	} else {
	    last;
	}
	$tries++;

	sleep(1);
    }

    if ($tries > $max_tries) {
	#
	# Admit defeat and exit
	#
	print STDERR "ERROR: Failed to obtain VLAN edit buffer lock\n";
	return 0;
    } else {
	#
	# Set the owner of the buffer to be the machine we're running on
	#
	my $me = `/usr/bin/uname -n`;
	chomp $me;
	$self->{SESS}->set([$BufferOwner,1,$me,"OCTETSTR"]);

	return 1;
    }

}

#
# Release a lock on the VLAN edit buffer. As part of releasing, applies the
# VLAN edit buffer.
#
# usage: vlanUnlock($self)
#
# TODO: Finish commenting, major cleanup, removal of obsolete features
#        
sub vlanUnlock($;$) {
    my $self = shift;
    my $force = shift;

    my $EditOp = 'vtpVlanEditOperation'; # use index 1
    my $ApplyStatus = 'vtpVlanApplyStatus'; # use index 1
    my $RetVal = $self->{SESS}->set([[$EditOp,1,"apply","INTEGER"]]);
    $self->debug("Apply set: '$RetVal'\n");

    $RetVal = $self->{SESS}->get([[$ApplyStatus,1]]);
    $self->debug("Apply gave $RetVal\n");
    while ($RetVal eq "inProgress") { 
	$RetVal = $self->{SESS}->get([[$ApplyStatus,1]]);
	$self->debug("Apply gave $RetVal\n");
    }

    my $ApplyRetVal = $RetVal;

    if ($RetVal ne "succeeded") {
	$self->debug("Apply failed: Gave $RetVal\n");
	# Only release the buffer if they've asked to force it.
	if (!$force) {
	    $RetVal = $self->{SESS}->set([[$EditOp,1,"release","INTEGER"]]);
	    $self->debug("Release: '$RetVal'\n");
	    if (! $RetVal ) {
		warn("VLAN Reconfiguration Failed. No changes saved.\n");
		return 0;
	    }
	}
    } else { 
	$self->debug("Apply Succeeded.\n");
	# If I succeed, release buffer
	$RetVal = $self->{SESS}->set([[$EditOp,1,"release","INTEGER"]]);
	if (! $RetVal ) {
	    warn("VLAN Reconfiguration Failed. No changes saved.\n");
	    return 0;
	}
	$self->debug("Release: '$RetVal'\n");
    }
    
    return $ApplyRetVal;
}

#
# Given a VLAN identifier from the database, find the cisco-specific VLAN
# number that is assigned to that VLAN. Retries several times (to account
# for propagation delays) unless the $no_retry option is given.
#
# usage: findVlan($self, $vlan_id,$no_retry)
#        returns the VLAN number for the given vlan_id if it exists
#        returns undef if the VLAN id is not found
#
sub findVlan($$;$) { 
    my $self = shift;
    my $vlan_id = shift;
    my $no_retry = shift;

    my $max_tries;
    if ($no_retry) {
	$max_tries = 1;
    } else {
	$max_tries = 10;
    }

    my $VlanName = "vtpVlanName"; # index by 1.vlan #

    #
    # We try this a few time, with 1 second sleeps, since it can take
    # a while for VLAN information to propagate
    #
    foreach my $try (1 .. $max_tries) {

	#
	# Walk the tree to find the VLAN names
	#
	my ($rows) = $self->{SESS}->bulkwalk(0,32,[$VlanName]);
	foreach my $rowref (@$rows) {
	    my ($name,$vlan_number,$vlan_name) = @$rowref;
	    #
	    # We get the VLAN number in the form 1.number - we need to strip
	    # off the '1.' to make it useful
	    #
	    $vlan_number =~ s/^1\.//;

	    $self->debug("Got $name $vlan_number $vlan_name\n",2);
	    if ($vlan_name eq $vlan_id) {
		return $vlan_number;
	    }
	}

	#
	# Wait before we try again
	#
	if ($try != $max_tries) {
	    $self->debug("VLAN find failed, trying again\n");
	    sleep 1;
	}
    }
    #
    # Didn't find it
    #
    return undef;
}

#
# Create a VLAN on this switch, with the given identifier (which comes from
# the database.) Picks its own switch-specific VLAN number to use.
#
# usage: createVlan($self, $vlan_id)
#        returns 1 on success
#        returns 0 on failure
#
sub createVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    my $okay = 0;

    #
    # NOTE: I'm not sure why I have to give these as numeric OIDs rather
    # than using the names.
    # TODO: Figure this out
    #
    my $VlanType = '.1.3.6.1.4.1.9.9.46.1.4.2.1.3.1'; # vlan # is index
    my $VlanName = '.1.3.6.1.4.1.9.9.46.1.4.2.1.4.1'; # vlan # is index
    my $VlanSAID = '.1.3.6.1.4.1.9.9.46.1.4.2.1.6.1'; # vlan # is index
    my $VlanRowStatus = '.1.3.6.1.4.1.9.9.46.1.4.2.1.11.1'; # vlan # is index

    if (!$self->vlanLock()) {
	return 0;
    }

    #
    # Find a free VLAN number to use. Since 1 is the default VLAN on
    # Ciscos, we start with number 2.
    # XXX: The maximum VLAN number is hardcoded at 1000
    #
    my $vlan_number = 2; # We need to start at 2
    my $RetVal = $self->{SESS}->get([$VlanRowStatus,$vlan_number]);
    $self->debug("Row $vlan_number got '$RetVal'\n",2);
    while (($RetVal ne 'NOSUCHINSTANCE') && ($vlan_number <= 1000)) {
	$vlan_number += 1;
	$RetVal = $self->{SESS}->get([[$VlanRowStatus,$vlan_number]]);
	$self->debug("Row $vlan_number got '$RetVal'\n",2);
    }
    if ($vlan_number > 1000) {
	#
	# We must have failed to find one
	#
	print STDERR "ERROR: Failed to find a free VLAN number\n";
	return 0;
    }

    $self->debug("Using Row $vlan_number\n");

    #
    # SAID is a funky security identifier that _must_ be set for VLAN
    # creation to suceeed.
    #
    my $SAID = pack("H*",sprintf("%08x",$vlan_number + 100000));

    print "  Creating VLAN $vlan_id as VLAN #$vlan_number ... ";

    #
    # Perform the actual creation. Yes, this next line MUST happen all in
    # one set command....
    #
    $RetVal = $self->{SESS}->set([[$VlanRowStatus,$vlan_number,"createAndGo","INTEGER"],
	    [$VlanType,$vlan_number,"ethernet","INTEGER"],
	    [$VlanName,$vlan_number,$vlan_id,"OCTETSTR"],
	    [$VlanSAID,$vlan_number,$SAID,"OCTETSTR"]]);
    print "",($RetVal? "Succeeded":"Failed"), ".\n";

    #
    # Check for success
    #
    if (!$RetVal) {
	print STDERR "VLAN Create '$vlan_id' as VLAN $vlan_number failed.\n";
	return 0;
    } else {
	$RetVal = $self->vlanUnlock();
	$self->debug("Got $RetVal from vlanUnlock\n");
	return 1;
    }
}

#
# Put the given ports in the given VLAN. The VLAN is given as a VLAN
# identifier from the database.
#
# usage: setPortVlan($self,$vlan_id, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_id = shift;
    my @ports = @_;

    my $PortVlanMemb = "vmVlan"; #index is ifIndex

    my $errors = 0;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 1;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Convert ports from the format the were passed in to IfIndex format
    #
    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@ports);

    #
    # We'll keep track of which ports suceeded, so that we don't try to
    # enable/disable, etc. ports that failed.
    #
    my @okports = ();
    foreach my $port (@portlist) {

	# 
	# Make sure the port didn't get mangled in conversion
	#
	if (!defined $port) {
	    print STDERR "Port not found, skipping\n";
	    $errors++;
	    next;
	}
	$self->debug("Putting port $port in VLAN $vlan_number\n");

	#
	# Do the acutal SNMP command
	#
	my $RetVal = $self->{SESS}->set([$PortVlanMemb,$port,$vlan_number,
					 'INTEGER']);
	if (!$RetVal) {
	    print STDERR "$port VLAN change failed with $RetVal.\n";
	    $errors++;
	    next;
	} else {
	    push @okports, $port;
	}
    }

    #
    # Ports going into VLAN 1 are being taken out of circulation, so we
    # disable them. Otherwise, we need to make sure they get enabled.
    #
    if ($vlan_number == 1) {
	$self->debug("Disabling " . join(',',@ports) . "...");
	if ( my $rv = $self->portControl("disable",@ports) ) {
	    print STDERR "Port disable had $rv failures.\n";
	    $errors += $rv;
	}
    } else {
	$self->debug("Enabling "  . join(',',@ports) . "...");
	if ( my $rv = $self->portControl("enable",@ports) ) {
	    print STDERR "Port enable had $rv failures.\n";
	    $errors += $rv;
	}
    }

    return $errors;
}

#
# Remove all ports from the given VLAN. The VLAN is given as a VLAN
# identifier from the database.
#
# usage: removePortsFromVlan(self,int vlan)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removePortsFromVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 1;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Get a list of the ports in the VLAN
    #
    #
    my $VlanPortVlan = ["vlanPortVlan"]; # index by module.port, gives vlan #

    #
    # Walk the tree to find VLAN membership
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32,$VlanPortVlan);
    my @ports;
    foreach my $rowref (@$rows) {
    	my ($name,$modport,$port_vlan_number) = @$rowref;
    	$self->debug("Got $name $modport $port_vlan_number\n");
	if ($port_vlan_number == $vlan_number) {
	    push @ports, $modport;
	}
    }

    $self->debug("About to remove ports " . join(",",@ports) . "\n");
    if (@ports) {
	return $self->setPortVlan("default",@ports);
    } else {
	return 0;
    }
}

#
# Remove the given VLAN from this switch. This presupposes that all of its
# ports have already been removed with removePortsFromVlan(). The VLAN is
# given as a VLAN identifier from the database.
#
# usage: removeVlan(self,int vlan)
#	 returns 1 on success
#	 returns 0 on failure
#
#
sub removeVlan($$) {
    my $self = shift;
    my $vlan_id = shift;

    #
    # Find the real VLAN number from the passed VLAN ID
    #
    my $vlan_number = $self->findVlan($vlan_id);
    if (!defined($vlan_number)) {
	print STDERR "ERROR: VLAN with identifier $vlan_id does not exist\n";
	return 0;
    }
    $self->debug("Found VLAN with ID $vlan_id: $vlan_number\n");

    #
    # Need to lock the VLAN edit buffer
    #
    if (!$self->vlanLock()) {
    	return 0;
    }

    #
    # Perform the actual removal
    #
    my $VlanRowStatus = '.1.3.6.1.4.1.9.9.46.1.4.2.1.11.1'; # vlan # is index
    print "  Removing VLAN #$vlan_number ... ";
    my $RetVal = $self->{SESS}->set([$VlanRowStatus,$vlan_number,"destroy","INTEGER"]);
    print ($RetVal ? "Succeeded.\n" : "Failed.\n");

    #
    # Unlock whether successful or not
    #
    $self->vlanUnlock();

    if (! defined $RetVal) { 
	print STDERR "VLAN #$vlan_number does not exist on this switch.\n";
	return 0;
    } else {
	$self->vlanUnlock();
	return 1;
    }

}

#
# TODO: Cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    # returns 0 on success, # of failed ports on failure
    $self->debug("UpdateField: '@_'\n");
    my ($OID,$val,@ports)= @_;
    my $Status = 0;
    my $err = 0;
    foreach my $port (@ports) {
	my $trans = $self->{IFINDEX}{$port};
	if (defined $trans) {
	    if (defined (portnum("$self->{NAME}:$trans"))) {
		$trans = "$trans,".portnum("$self->{NAME}:$trans");
	    } else {
		$trans = "$trans,".portnum("$self->{NAME}:$port");
	    }
	} else {
	    $trans = "???";
	}
	$self->debug("Checking port $port ($trans) for $val...");
	$Status = $self->{SESS}->get([[$OID,$port]]);
	if (!defined $Status) {
	    warn "Port $port ($trans), change to $val: No answer from device\n";
	    return -1;		# return error
	} else {
	    $self->debug("Okay.\n");
	    $self->debug("Port $port was $Status\n");
	    if ($Status ne $val) {
		$self->debug("Setting $port to $val...");
		# Don't use async
		my $result = $self->{SESS}->set([$OID,$port,$val,"INTEGER"]);
		$self->debug("Set returned '$result'\n") if (defined $result);
		if ($self->{BLOCK}) {
		    my $n = 0;
		    while ($Status ne $val) {
			$Status=$self->{SESS}->get([[$OID,$port]]);
			$self->debug("Value for $port was $Status\n");
			select (undef, undef, undef, .25); # wait .25 seconds
			$n++;
			if ($n > 20) {
			    $err++;
			    $self->debug("Timing out...");
			    last;
			}
		    }
		    $self->debug("Okay.\n");
		} else {
		    $self->debug("\n");
		}
	    }
	}
    }
    # returns 0 on success, # of failed ports on failure
    $err;
}

#
# List all VLANs on the device
#
# usage: listVlans($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listVlans($) {
    my $self = shift;

    $self->debug("Getting VLAN info...\n");
    # We don't need VlanIndex really...
    my $VlanName = ["vtpVlanName"]; # index by 1.vlan #
    my $VlanPortVlan = ["vlanPortVlan"]; # index by module.port, gives vlan #

    #
    # Walk the tree to find the VLAN names
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32,$VlanName);
    my %Names = ();
    foreach my $rowref (@$rows) {
	my ($name,$vlan_number,$vlan_name) = @$rowref;
	#
	# We get the VLAN number in the form 1.number - we need to strip
	# off the '1.' to make it useful
	#
	$vlan_number =~ s/^1\.//;

	$self->debug("Got $name $vlan_number $vlan_name\n",3);
	if (!$Names{$vlan_number}) {
	    $Names{$vlan_number} = $vlan_name;
	}
    }

    #
    # Walk the tree for the VLAN members
    #
    ($rows) = $self->{SESS}->bulkwalk(0,32,$VlanPortVlan);
    my %Members = ();
    foreach my $rowref (@$rows) {
	my ($name,$modport,$vlan_number) = @$rowref;
	$self->debug("Got $name $modport $vlan_number\n",3);
	my $node;
	($node = portnum("$self->{NAME}:$modport")) || ($node = "port$modport");
	push @{$Members{$vlan_number}}, $node;
    }

    #
    # Build a list from the name and membership lists
    #
    my @list = ();
    foreach my $vlan_id (sort {$a <=> $b} keys %Names) {
    	if ($vlan_id != 1) {
	    #
    	    # Special case for Cisco - VLAN 1 is special and should not
    	    # be reported
	    #
    	    push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
    	}
    }
    $self->debug(join("\n",@list)."\n",2);

    return @list;
}

#
# List all ports on the device
#
# usage: listPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
# TODO: convert to bulkwalk
#
sub listPorts($) {
    my $self = shift;

    my %Able = ();
    my %Link = ();
    my %speed = ();
    my %duplex = ();
    my $ifTable = ["ifAdminStatus",0];
    my @data=();

    # TODO: Clean up this section and convert to bulkwalk
    #do one to get the first field...
    $self->debug("Getting port information...\n");
    do {{
	$self->{SESS}->getnext($ifTable);
	@data = @{$ifTable};
	if ($data[0] eq "ifLastChange") {
	    $ifTable = ["portAdminSpeed",0];
	    $self->{SESS}->getnext($ifTable);
	    @data = @{$ifTable};
	}
	if (! defined $self->{IFINDEX}{$data[1]}) { next; }
	my $port = portnum("$self->{NAME}:$data[1]")
	    || portnum("$self->{NAME}:".$self->{IFINDEX}{$data[1]});
	if (! defined $port) { next; }
	$self->debug("$port\t$data[0]\t$data[2]\n",2);
	if    ($data[0]=~/AdminStatus/) {$Able{$port}=($data[2]=~/up/?"yes":"no");}
	elsif ($data[0]=~/ifOperStatus/)         {  $Link{$port}=$data[2];}
	elsif ($data[0]=~/AdminSpeed/)           { $speed{$port}=$data[2];}
	elsif ($data[0]=~/Duplex/)               {$duplex{$port}=$data[2];}
	# Insert stuff here to get ifSpeed if necessary... AdminSpeed is the
	# _desired_ speed, and ifSpeed is the _real_ speed it is using
	}} while ( $data[0] =~
	    /(i(f).*Status)|(port(AdminSpeed|Duplex))/) ;

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %Able ) {
	my $vlan;
	if (! defined ($speed{$id}) ) { $speed{$id} = " "; }
	if (! defined ($duplex{$id}) ) { $duplex{$id} = " "; }
	$speed{$id} =~ s/s([10]+)000000/${1}Mbps/;
	push @rv, [$id,$Able{$id},$Link{$id},$speed{$id},$duplex{$id}];
    }
    return @rv;
}

# 
# Get statistics for ports on the switch
#
# usage: getPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
# TODO: Clean up undefined variable warnings
#
sub getStats ($) {
    my $self = shift;

    #
    # Walk the tree for the VLAN members
    #
    my $vars = new SNMP::VarList(['ifInOctets'],['ifInUcastPkts'],
    				 ['ifInNUcastPkts'],['ifInDiscards'],
				 ['ifInErrors'],['ifInUnknownProtos'],
				 ['ifOutOctets'],['ifOutUcastPkts'],
				 ['ifOutNUcastPkts'],['ifOutDiscards'],
				 ['ifOutErrors'],['ifOutQLen']);
    my @stats = $self->{SESS}->bulkwalk(0,32,$vars);

    #
    # We need to flip the two-dimentional array we got from bulkwalk on
    # its side, and convert ifindexes into node:port
    #
    my $i = 0;
    my %stats;
    foreach my $array (@stats) {
	while (@$array) {
	    my ($name,$ifindex,$value) = @{shift @$array};
	    my ($port) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$ifindex);
	    if ($port) {
		${$stats{$port}}[$i] = $value;
	    }
	}
	$i++;
    }

    return map [$_,@{$stats{$_}}], sort {tbsort($a,$b)} keys %stats;

}

#
# Enable, or disable,  port on a trunk
#
# usage: removeVlan(self, vlan_number, modport, value)
#	 vlan_number: The cisco-native VLAN top operate on
#        modport: module.port of the trunk to operate on
#        value: 0 to disallow the VLAN on the trunk, 1 to allow it
#        Returns 1 on success, 0 otherwise
#
sub setVlanOnTrunk($$$$) {
    my $self = shift;
    my ($vlan_number, $modport, $value) = @_;

    #
    # Some error checking
    #
    if (($value != 1) && ($value != 0)) {
	die "Invalid value $value passed to setVlanOnTrunk\n";
    }
    if ($vlan_number == 1) {
	die "VLAN 1 passed to setVlanOnTrunk\n"
    }

    my $ifIndex = $self->{IFINDEX}{$modport};

    #
    # Get the exisisting bitfield for allowed VLANs on the trunk
    #
    my $bitfield = $self->{SESS}->get(["vlanTrunkPortVlansEnabled",$ifIndex]);
    my $unpacked = unpack("B*",$bitfield);
    
    # Put this into an array of 1s and 0s for easy manipulation
    my @bits = split //,$unpacked;

    # Just set the bit of the one we want to change
    $bits[$vlan_number] = $value;

    # Pack it back up...
    $unpacked = join('',@bits);
    $bitfield = pack("B*",$unpacked);

    # And save it back...
    if (!$self->{SESS}->set(["vlanTrunkPortVlansEnabled",$ifIndex,$bitfield,
	    "OCTETSTR"])) {
	return 1;
    } else {
	return 0;
    }

}

#
# Reads the IfIndex table from the switch, for SNMP functions that use 
# IfIndex rather than the module.port style. Fills out the objects IFINDEX
# members,
#
# usage: readifIndex(self)
#        returns nothing
#
sub readifIndex($) {
    my $self = shift;

    #
    # Walk the tree for portIfIndex
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32,["portIfIndex"]);
   
    foreach my $rowref (@$rows) {
	my ($name,$modport,$ifindex) = @$rowref;

	$self->{IFINDEX}{$modport} = $ifindex;
	$self->{IFINDEX}{$ifindex} = $modport;
    }
}

#
# Prints out a debugging message, but only if debugging is on. If a level is
# given, the debuglevel must be >= that level for the message to print. If
# the level is omitted, 1 is assumed
#
# Usage: debug($self, $message, $level)
#
sub debug($$:$) {
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
