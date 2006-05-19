#!/usr/bin/perl -w

# This file was modified from an Netbed/Emulab module.
# Modfications Copyright (c) 2004, Regents, University of California.
#

#
# This file is part of the Netbed/Emulab network testbed software.
# In brief, you can't redistribute it or use it for commercial purposes,
# and you must give appropriate credit and return improvements to Utah.
# See the file LICENSE at the root of the source tree for details.
# 
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#

#
# snmpit module for Nortel level 2 switches
#

package snmpit_nortel;
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
    "1000mbit"=> ["rcPortAdminSpeed","mbps1000"],
    "100mbit"=> ["rcPortAdminSpeed","mbps100"],
    "10mbit" => ["rcPortAdminSpeed","mbps10"],
    "auto"   => ["rcPortAutoNegotiate","true"],
    "full"   => ["rcPortAdminDuplex","full"],
    "half"   => ["rcPortAdminDuplex","half"],
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
# usage: new($classname,$devicename,$debuglevel,$community)
#        returns a new object, blessed into the snmpit_intel class.
#
sub new($$$;$) {

    # The next two lines are some voodoo taken from perltoot(1)
    my $proto = shift;
    my $class = ref($proto) || $proto;

    my $name = shift;
    my $debugLevel = shift;
    my $community = shift;

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
    $self->{CONFIRM} = 1;
    $self->{NAME} = $name;

    #
    # Get config options from the database
    #
    my $options = getDeviceOptions($self->{NAME});
    if (!$options) {
	warn "ERROR: Getting switch options for $self->{NAME}\n";
	return undef;
    }

    $self->{MIN_VLAN}         = $options->{'min_vlan'};
    $self->{MAX_VLAN}         = $options->{'max_vlan'};

    if ($community) { # Allow this to over-ride the default
	$self->{COMMUNITY}    = $community;
    } else {
	$self->{COMMUNITY}    = $options->{'snmp_community'};
    }

    #
    # set up hashes for pending vlans
    #
    $self->{IFINDEX} = {};
    $self->{PORTINDEX} = {};

    # other global variables
    $self->{DOALLPORTS} = 1;
    $self->{DOALLPORTS} = 0;
    $self->{SKIPIGMP} = 1;

    if ($self->{DEBUG}) {
	print "snmpit_nortel initializing $self->{NAME}, " .
	    "debug level $self->{DEBUG}\n" ;   
    }


    #
    # Set up SNMP module variables, and connect to the device
    #
    $SNMP::debugging = ($self->{DEBUG} - 2) if $self->{DEBUG} > 2;
    my $mibpath = '/usr/local/share/snmp/mibs';
    &SNMP::addMibDirs($mibpath);
    &SNMP::addMibFiles("$mibpath/SNMPv2-SMI.txt", "$mibpath/SNMPv2-TC.txt", 
	               "$mibpath/SNMPv2-MIB.txt", "$mibpath/IANAifType-MIB.txt",
		       "$mibpath/IF-MIB.txt",
		       "$mibpath/RAPID-CITY.txt");
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		  # parses default list of Mib modules 
    $SNMP::use_enums = 1;	  # use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});

    $self->{SESS} = new SNMP::Session(DestHost => $self->{NAME},Version => "2c",
	Timeout => 4000000, Retries=> 12, Community => $self->{COMMUNITY});

    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "WARNING: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }
    #
    # The bless needs to occur before readifIndex(), since it's a class 
    # method
    #
    bless($self,$class);

    #
    # Sometimes the SNMP session gets created when there is no connectivity
    # to the device so let's try something simple
    #
    my $test_case = $self->get1("sysDescr", 0);
    if (!defined($test_case)) {
	warn "WARNING: Unable to retrieve via SNMP from $self->{NAME}\n";
	return undef;

    }

    $self->readifIndex();

    return $self;
}

# Attempt to repeat an action until it succeeds

sub hammer($$$;$) {
    my ($self, $closure, $id, $retries) = @_;

    if (!defined($retries)) { $retries = 12; }
    for my $i (1 .. $retries) {
	my $result = $closure->();
	if (defined($result) || ($retries == 1)) { return $result; }
	warn $id . " ... will try again\n";
	sleep 1;
    }
    warn  $id . " .. giving up\n";
    return undef;
}

# shorthand

sub get1($$$) {
    my ($self, $obj, $instance) = @_;
    my $id = $self->{NAME} . ":get1 $obj.$instance";
    my $closure = sub () {
	my $RetVal = snmpitGet($self->{SESS}, [$obj, $instance], 1);
	if (!defined($RetVal)) { sleep 4;}
	return $RetVal;
    };
    my $RetVal = $self->hammer($closure, $id, 40);
    if (!defined($RetVal)) {
	warn "$id failed - $snmpit_lib::snmpitErrorString\n";
    }
    return $RetVal;
}

# Translate a list of ifIndexes to a PortSet

sub listToPortSet($@)
{
    my $self = shift;
    my @ports = @_;

    my ($max, $portbitstring, $portSet, $port);
    my @portbits;

    $self->debug("listToPortsSet: input @ports\n",2);
    if (scalar @ports) {
	@ports = sort { $b <=> $a} @ports;
	$max = ($ports[0] | 7);
    } else { $max = 7 ; }
    $port = 0;
    while ($port <= $max) { $portbits[$port] = 48 ; $port++; }
    while (scalar @ports) { $port = pop @ports; $portbits[$port] = 49; }
    $self->debug("portbits after insertions: @portbits\n",2);
    $portbitstring = pack "C*", @portbits;
    $self->debug("listToPortSet output string $portbitstring \n");
    $portSet = pack "B*", $portbitstring ;
    return $portSet;
}
# Translate a PortSet to a list of ifIndexes

sub portSetToList($$)
{
    my $self = shift;
    my $portset = shift;

    my ($max, $portbitstring, $portSet, $port);
    my @portbits;
    my @ports = ();

    $portbitstring = unpack "B*", $portset;
    @portbits = unpack "C*", $portbitstring;

    $max = scalar @portbits;
    $port = 0;
    while ($port < $max) {

	if ($portbits[$port] == 49) {
	    push @ports, $port;
	}
	$port++;
    }
    return @ports;
}

# Compare two PortSets for equality ignoring trailing zeroes, etc.

sub portSetDiffers($$$)
{
    my $self = shift;
    my $setA = shift;
    my $setB = shift;

    my @listA = $self->portSetToList($setA);
    my @listB = $self->portSetToList($setB);

    while (@listA) {
	my $itemA = pop @listA;
	my $itemB;
	if (@listB) { $itemB = pop @listB; } else { return 1;}
	if ($itemA != $itemB) {return 1;}
    }
    if (@listB) {return 1;} else {return 0;}
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

    my @p = map { portnum($_) } @ports;

    #
    # Find the command in the %cmdOIDs hash (defined at the top of this file).
    # Some commands involve multiple SNMP commands, so we need to make sure
    # we get all of them
    #
    if (defined $cmdOIDs{$cmd}) {
	my @oid = @{$cmdOIDs{$cmd}};
	my $errors = 0;

	while (@oid) {
	    my $myoid = shift @oid;
	    my $myval = shift @oid;
	    $errors += $self->UpdateField($myoid,$myval,@ports);
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
    if (!defined($sample)) {
	warn "convertPortFormat: Given a bad list of ports\n";
	return undef;
    }

    my $input;
    SWITCH: for ($sample) {
	(/^\d+$/) && do { $input = $PORT_FORMAT_IFINDEX; last; };
	(/^\d+\.\d+$/) && do { $input = $PORT_FORMAT_MODPORT; last; };
	(/^$self->{NAME}\.\d+\/\d+$/) && do { $input = $PORT_FORMAT_MODPORT;
		@ports = map {/^$self->{NAME}\.(\d+)\/(\d+)$/; "$1.$2";} @ports; last; };
	$input = $PORT_FORMAT_NODEPORT; last;
    }

    #
    # It's possible the ports are already in the right format
    #
    if ($input == $output) {
	$self->debug("Not converting, input format = output format\n",3);
	return @ports;
    }

    if ($input == $PORT_FORMAT_IFINDEX) {
	if ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting ifindex to modport\n",3);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting ifindex to nodeport\n",3);
	    return map portnum($self->{NAME}.":".$self->{IFINDEX}{$_}), @ports;
	}
    } elsif ($input == $PORT_FORMAT_MODPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting modport to ifindex\n",3);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting modport to nodeport\n",3);
	    return map portnum($self->{NAME} . ":$_"), @ports;
	}
    } elsif ($input == $PORT_FORMAT_NODEPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting nodeport to ifindex\n",3);
	    return map $self->{IFINDEX}{(split /:/,portnum($_))[1]}, @ports;
	} elsif ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting nodeport to modport\n",3);
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
# Check to see if the given 802.1Q VLAN tag exists on the switch
#
# usage: vlanNumberExists($self, $vlan_number)
#        returns 1 if the VLAN exists, 0 otherwise
#
sub vlanNumberExists($$) {
    my ($self, $vlan_number) = @_;

    my $rv = $self->get1("rcVlanId", $vlan_number);
    if (!defined($rv) || !$rv || $rv eq "NOSUCHINSTANCE") {
	return 0;
    }
    return 1;
}

#
# Given VLAN indentifiers from the database, finds the 802.1Q VLAN
# number for them. If not VLAN id is given, returns mappings for the entire
# switch.
# 
# usage: findVlans($self, @vlan_ids)
#        returns a hash mapping VLAN ids to 802.1Q VLAN numbers
#        any VLANs not found have NULL VLAN numbers
#
sub findVlans($@) { 
    my $self = shift;
    my @vlan_ids = @_;
    my %mapping = ();
    my $id = $self->{NAME} . "::findVlans";
    my ($count, $name, $vlan_number, $vlan_name) = (scalar(@vlan_ids));

    if ($count > 0) { @mapping{@vlan_ids} = undef; }

    #
    # Find all VLAN names. Do one to get the first field...
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32, ["rcVlanName"]);
    foreach my $rowref (@$rows) {
	($name,$vlan_number,$vlan_name) = @$rowref;
	$self->debug("$id: Got $name $vlan_number $vlan_name\n");
	#
	# We only want the names - we ignore everything else
	#
	    if (!@vlan_ids || exists $mapping{$vlan_name}) {
		$self->debug("$id: $vlan_name=>$vlan_number\n",2);
		$mapping{$vlan_name} = $vlan_number;
	    }
    }

    return %mapping;
}

#
# Given a VLAN identifier from the database, find the 802.1Q VLAN
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
    my $id = $self->{NAME} . ":findVlan";

    $self->debug("$id ( $vlan_id )\n",2);
    my $max_tries = $no_retry ? 1 : 5;
    #
    # We try this a few times, with 5 second sleeps, since it can take
    # a while for VLAN information to propagate
    #
    my $closure = sub () {
	my %mapping = $self->findVlans($vlan_id);
	my $vlan_number = $mapping{$vlan_id};
	if (defined($vlan_number)) { return $vlan_number; }
	sleep 4;
	return undef;
    };
    return $self->hammer($closure,$id,$max_tries);
}

#   
# Create a VLAN on this switch, with the given identifier (which comes from
# the database) and given 802.1Q tag number.
#
# usage: createVlan($self, $vlan_id, $vlan_number)
#        returns the new VLAN number on success
#        returns 0 on failure
#
# Creation of a vlan under the RAPID-CITY MIB only seems to require
# the type (vlanbyport), and a spanning tree group, (all vlans
# can share the same universal STG), but it's convenient to also
# name it at this point.  We'll try to add ports later.
# we have to supply all parameters in the same SNMP packet, however.
#
sub createVlan($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $vlan_number = shift;
    my $id = $self->{NAME} . ":findVlan";

    if (!defined($vlan_number)) {
	warn "$id called without supplying vlan_number";
	return 0;
    }
    my $check_number = $self->findVlan($vlan_id,1);
    if (defined($check_number)) {
	if ($check_number != $vlan_number) {
		warn "$id: recreating vlan id $vlan_id which has ".
		     "existing vlan number $check_number with the new number ".
		     "$vlan_number\n";
		     return 0;
	}
     }
    my $vlan ="$vlan_number";
    my $VlanRowStatus = 'rcVlanRowStatus'; # vlan # is index
    my $VlanName = 'rcVlanName'; # vlan # is index
    my $VlanType = 'rcVlanType'; # vlan # is index
    my $VlanStg = 'rcVlanStgId'; # vlan # is index
    $self->debug("createVlan: name $vlan_id number $vlan_number \n");
    #
    # Perform the actual creation. Yes, this next line MUST happen all in
    # one set command....
    #
    my $closure = sub () {
    	my $RetVal = $self->{SESS}->set(
	    [[$VlanRowStatus,$vlan, "createAndGo","INTEGER"],
	     [$VlanType,$vlan,"byPort","INTEGER"],
	     [$VlanStg,$vlan,1,"INTEGER"],
	     [$VlanName,$vlan,$vlan_id,"OCTETSTR"]]);
	if (defined($RetVal)) { return $RetVal; }
	#
	# Sometimes we loose responses, or on the second time around
	# it might refuse to create a vlan that's already there, so wait
	# a bit to see if it exists (also so as to not get too
	# agressive with the switch which can cause to crash with
	# certain sets, e.g. IGMP stuff)
	#
	sleep (9);
	$RetVal = $self->get1($VlanRowStatus, $vlan);
	if (defined($RetVal) && ($RetVal ne "active")) { return undef ;}
	return $RetVal;
    };
    my $RetVal = $self->hammer($closure, "$id: creation");
    if (!defined($RetVal)) { return 0; }
    print "  Creating VLAN $vlan_id as VLAN #$vlan_number on " .
	    "$self->{NAME} ... ";


    # You'ld think you'ld be able to add IgmpSnoopEnable to the above as one
    # more item, but that caused it to fail, even though it wins as a separate
    # packet.

    # There also something in the IGMP magic that causes the switch
    # to get totally confused, so we're going to throw in some superfluous
    # queries just to make sure things happened and slow down our interactions.

    $RetVal = $self->findVlan($vlan_id);
    if (!defined($RetVal) || ("$RetVal" ne $vlan)) {
	warn "$id: created vlan $vlan_id with number $RetVal" .
	  "instead of $vlan_number\n";
    }

    if ($self->{SKIPIGMP}) { return $vlan_number ; }
    my $IgmpEnable = 'rcVlanIgmpSnoopEnable';
    $RetVal = $self->get1($IgmpEnable, $vlan);

    $closure = sub () {
	my $check = $self->{SESS}->set([[$IgmpEnable,$vlan,"true","INTEGER"]]);
	if (!defined($check) || ($check ne "true")) { sleep (19); return undef;}
	return $check;
    };
    $RetVal = $self->hammer($closure, "$id: setting snooping");
    if (!defined($RetVal)) { return 0; }

    $closure = sub () {
	my $check = $self->get1($IgmpEnable, $vlan);
	if (!defined($check) || ($check ne "true")) { sleep (4); return undef ;}
	return $check;
    };
    $RetVal = $self->hammer($closure, "$id: checking snooping");
    if (!defined($RetVal)) { return 0; }

    return $vlan_number;
}

sub snmpSetCheckPortSet($$$$)
{
    my ($self, $obj, $vlan_number, $SetVal) = @_;
    my $id = $self->{NAME} . "::SetCheckPortSet";
    my $set_arg = [$obj,$vlan_number,$SetVal,"OCTETSTR"];
    my $counter = 0;
    my ($inner_closure, $outer_closure, $final_check);

    $inner_closure = sub () {
	my $ctr_p =\$counter;
	$$ctr_p++;
	my $check = $self->get1($obj, $vlan_number);
	if (!defined($check)) { sleep 2; return undef; }
	if ($self->portSetDiffers($check, $SetVal)) {
	    if ($$ctr_p == 8) {
		my @oldIgmpList = $self->portSetToList($SetVal);
		my @newIgmpList = $self->portSetToList($check);
		print "SetCheckPortSet result for vlan # $vlan_number " 
		. "differs from what was set -  old is @oldIgmpList\n " 
		. "new is @newIgmpList \n";
	    }
	    return undef;
	}
	return $check;
    };
    $outer_closure = sub () {
	my $ctr_p = \$counter;
	$$ctr_p = 0;
	my $RetVal = $self->{SESS}->set($set_arg);
	if (!defined($RetVal)) {
	    my $reason = "";
	    if (defined($snmpit_lib::snmpitErrorString)) {
		    $reason = $snmpit_lib::snmpitErrorString;
	    };
	    warn "$id: SetCheckPortSet set $obj may have failed: $reason\n";
	    sleep 4;
	}
	return $self->hammer($inner_closure, $id, 8);
    };
    $final_check = $self->hammer($outer_closure, $id, 4);
    if (defined($final_check)) { return 0; }
    return 1;
}

#
# Put the given ports in the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: setPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
# nortel specific: if the vlan_number < 0 *remove* the ports from the vlan
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $errors = 0;
    my $obj = "rcVlanPortMembers";
    my $id = $self->{NAME} . "::setPortVlan";
    $self->debug("$id: $vlan_number ");
	   
    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #

    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX, @ports);
    $self->debug("ports: " . join(",",@ports) . "\n");
    $self->debug("as ifIndexes: " . join(",",@portlist) . "\n");

    #
    # We need to remove any normal ports from other vlans to which
    # they may belong; the nortels don't automatically do this, unlike
    # foundry's and cisco's.
    #
    my (%vlansToPorts, $porttype, $vlans, $portIndex);
    my (@vlanList, $vlan, @memberlist);
    %vlansToPorts = ();
    foreach $portIndex (@portlist) {
	$porttype = $self->get1("rcVlanPortType", $portIndex);
	if (defined($porttype)) {
	    $self->debug("ifIndex $portIndex has type $porttype");
	    if ($porttype eq "access") {
		$vlans = $self->get1("rcVlanPortVlanIds", $portIndex);
		if (defined($vlans)) {
		    @vlanList = unpack "n*", $vlans;
		    $self->debug(" and is in vlans @vlanList");
		    foreach $vlan (@vlanList) {
			if ($vlan != $vlan_number) {
			    if (!defined($vlansToPorts{$vlan})) {
				$vlansToPorts{$vlan} = [];
			    }
			    push @{$vlansToPorts{$vlan}}, $portIndex;
			}
		    }
		}
	    }
	}
	$self->debug("\n");
    }
    my @bumpedlist = keys %vlansToPorts;
    foreach $vlan (@bumpedlist) {
	$self->delPortVlan($vlan, @{$vlansToPorts{$vlan}});
    }
    if (@bumpedlist = grep {$_ != 1;} @bumpedlist) {
	@{$self->{DISPLACED_VLANS}} = @bumpedlist;
    }

    my $portSet = $self->get1($obj, $vlan_number);
    if (!$portSet) {
	printf STDERR "$id: Could not get current list for VLAN $vlan_number\n";
	return 1;
    }
    my @newlist;
    my @oldlist = $self->portSetToList($portSet);
    push @oldlist, @portlist;
    my $SetVal = $self->listToPortSet(@oldlist);
    $self->debug("$id: members of VLAN $vlan_number [ @oldlist ]\n");

    if ($self->snmpSetCheckPortSet($obj, $vlan_number, $SetVal)) {
	return 1;
    }
    #
    # We need to make sure the ports get enabled.
    #

    $self->debug("Enabling "  . join(',',@ports) . "...\n");
    if ( my $rv = $self->portControl("enable",@ports) ) {
	print STDERR "Port enable had $rv failures.\n";
	$errors += $rv;
    }

    if ($self->{SKIPIGMP}) { return $errors; }

    my $igmp = "rcVlanIgmpVer1SnoopMRouterPorts";
    my $IgnoredVal = $self->{SESS}->get("$igmp.$vlan_number");
    if ($self->snmpSetCheckPortSet($igmp, $vlan_number, $SetVal)) {
	return 1;
    }

    return $errors;
}

sub not_in($$@) {
    my $self = shift;
    my $value = shift;
    my @list = @_;

    return 0 == scalar(grep {$_ == $value;} @list);
}

#
# Remove the given ports from the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: delPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub delPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $obj = "rcVlanPortMembers";
    $self->debug($self->{NAME} . "::delPortVlan $vlan_number ");
	   
    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #

    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX, @ports);
    $self->debug("ports: " . join(",",@ports) . "\n");
    $self->debug("as ifIndexes: " . join(",",@portlist) . "\n");

    my $portSet = $self->{SESS}->get("$obj.$vlan_number");

    if (!$portSet) {
	printf STDERR "Could not get current list for VLAN $vlan_number\n";
	return 1;
    }
    my @oldlist = $self->portSetToList($portSet);

    $self->debug("oldlist of VLAN $vlan_number [ @oldlist ]\n");
    $self->debug("portlist to drop is [ @portlist ]\n");
    @oldlist =  grep {$self->not_in($_, @portlist);} @oldlist;
    $self->debug("after set diff oldlist is [ @oldlist ]\n");

    my $SetVal = $self->listToPortSet(@oldlist);

    return $self->snmpSetCheckPortSet($obj, $vlan_number, $SetVal);
}


#
# Remove all ports from the given VLANS. Each VLAN is given as a VLAN
# 802.1Q tag value.
#
# usage: removePortsFromVlan(self,@vlan)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removePortsFromVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;
    my $errors = 0;
    my $id = $self->{NAME} . "::removePortsFromVlan";

    my ($name,$index,$value,$modport,$portIndex,$portString);
    foreach my $vlan_number (@vlan_numbers) {
	#
	# Do one to get the first field...
	#
	$self->debug("$id: number $vlan_number\n");
	my $field = ["rcVlanPortMembers",$vlan_number,"OCTETSTRING"];
	$value = $self->{SESS}->get($field);
	if (!$value) {
		print STDERR "No port member list for VLAN $vlan_number\n";
		return 1;
	}
	my @portlist = $self->portSetToList($value);
	$self->debug("removePortsFromVlan $vlan_number: @portlist\n");
	foreach $portIndex (@portlist) {
	    #
	    # disable this port, unless it is tagged.
	    #
	    $value= $self->{SESS}->get(["rcVlanPortType",$portIndex]);
	    if (defined($value) && ($value eq "access")) {
		$self->debug("disabling port $portIndex  "
				. "from vlan $vlan_number ( $value )\n" );
		$value = $self->{SESS}->set
		(["ifAdminStatus",$portIndex,"down","INTEGER"]);
		$value = $self->{SESS}->get
		(["ifAdminStatus",$portIndex]);
	    }
	}
    }
    return $errors;
}

#
# Remove the given VLANs from this switch. Removes all ports from the VLAN,
# so it's not necessary to call removePortsFromVlan() first. The VLAN is
# given as a VLAN identifier from the database.
#
# usage: removeVlan(self,int vlan)
#	 returns 1 on success
#	 returns 0 on failure
#
#
sub removeVlan($@) {
    my $self = shift;
    my @vlan_numbers = @_;
    my $errors = 0;
    my $DeleteOID = "rcVlanRowStatus";
    my $name = $self->{NAME};



    foreach my $vlan_number (@vlan_numbers) {
	#
	# Perform the actual removal
	#
	my $RetVal = undef;
	print "  Removing VLAN # $vlan_number ... ";
	$RetVal = $self->{SESS}->set([[$DeleteOID,$vlan_number,"destroy","INTEGER"]]);
	if ($RetVal) {
	    print "Removed VLAN $vlan_number on switch $name.\n";
	} else {
	    print STDERR "Removing VLAN $vlan_number failed on switch $name.\n";
	}
	# The next call is to buy time to let switch consolidate itself
	# we've seen bizarre failures when quickly creating and destroying
	# vlans with IGMP snooping enabled.

	$RetVal = $self->{SESS}->get([$DeleteOID,$vlan_number]);
	my ($rows) = $self->{SESS}->bulkwalk(0,32, ["rcVlanName"]);
    }
    return ($errors == 0) ? 1 : 0;
}

#
# XXX: Major cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    my ($OID,$val,@ports)= @_;
    my $id = $self->{NAME} . "::UpdateField";

    $self->debug("$id: OID $OID value $val ports @ports\n");

    my $Status = 0;
    my $result = 0;
    my ($portname, $module, $port, $row, $modport);


    foreach my $portname (@ports) {
	#
	# Check the input format - the ports might already be in
	# switch:module.port form, or we may have to convert them to it with
	# portnum
	#
	if ($portname =~ /:(\d+)\.(\d+)/) {
	    $module = $1; $port = $2;
	} else {
	    $portname = portnum($portname);
	    if (!$portname) { next; }
	    $portname =~ /:(\d+)\.(\d+)/;
	    $module = $1; $port = $2;
	}
	$modport = "$module.$port";
	$self->debug("port $portname ( $modport ), " );
	if ($OID =~ /^ifAdmin/) {
		$row = $self->{IFINDEX}{$modport};
	} else {
		$row = $self->{PORTINDEX}{$modport};
	}
	$self->debug("checking row $row for $val ...\n");
	$Status = $self->{SESS}->get([$OID,$row]);
	if (!defined $Status) {
	    print STDERR "Port $portname, change to $val: No answer from device\n";
	    $result = -1;
	} else {
	    $self->debug("Port $portname, row $row was $Status\n");
	    if ($Status ne $val) {
		$self->debug("Setting $portname (r $row) to $val...");
		$self->{SESS}->set([[$OID,$row,$val,"INTEGER"]]);
		my $count = 6;
		while (($Status ne $val) && (--$count > 0)) { 
		    sleep 1;
		    $Status = $self->{SESS}->get([$OID,$row]);
		    $self->debug("Value for $portname is currently $Status\n");
		}
		$result =  ($count > 0) ? 0 : -1;
		$self->debug($result ? "failed.\n" : "succeeded.\n");
	    }
	}

    }
    return $result;
}

#
# Determine if a VLAN has any members 
# (Used by stack->switchesWithPortsInVlan())
#
sub vlanHasPorts($$) {
    my ($self, $vlan_number) = @_;

    my $portset = $self->get1("rcVlanPortMembers",$vlan_number);
    if (defined($portset)) {
	my @ports = $self->portSetToList($portset);
	if (@ports) { return 1; }
    }
    return 0;
}

#
# List all VLANs on the device
#
# usage: listVlans($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listVlans($) {
    my $self = shift;

    my %Names = ();
    my %Numbers = ();
    my %Members = ();
    my ($vlan_name, $oid, $vlan_number, $value, $rowref);
    my ($modport, $node, $ifIndex, @portlist, @memberlist);
    $self->debug($self->{NAME} . ":listVlans()\n",3);

    #
    # Walk the tree to find the VLAN names
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32,"rcVlanName");
    foreach $rowref (@$rows) {
	($oid, $vlan_number, $vlan_name) = @$rowref;
	$self->debug("Got $oid $vlan_number $vlan_name\n",3);
	if (!$Names{$vlan_number}) {
	    $Names{$vlan_number} = $vlan_name;
	    @{$Members{$vlan_number}} = ();
	}
    }

    #
    #  Walk the tree for the VLAN members
    #
    ($rows) = $self->{SESS}->bulkwalk(0,32,"rcVlanPortMembers");
    foreach $rowref (@$rows) {
	($oid,$vlan_number,$value) = @$rowref;
	@portlist = $self->portSetToList($value);
	$self->debug("Got $oid $vlan_number @portlist\n",3);

	foreach $ifIndex (@portlist) {
	    ($node) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$ifIndex);
	    if (!$node) {
		$modport = $self->convertPortFormat
			($PORT_FORMAT_MODPORT,$ifIndex);
		$node = $self->{NAME} . ":$modport";
	    }
	    push @{$Members{$vlan_number}}, $node;
	    if (!$Names{$vlan_number}) {
		$self->debug("listVlans: WARNING: port $self->{NAME}.$modport in non-existant " .
		    "VLAN $vlan_number\n");
	    }
	}
    }

    #
    # Build a list from the name and membership lists
    #
    my @list = ();
    foreach my $vlan_id (sort keys %Names) {
	if ($vlan_id !=  1) {
	    push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
	}
    }

    #$self->debug($self->{NAME} .":". join("\n",(map {join ",", @$_} @list))."\n");
    return @list;
}

#
# List all ports on the device
#
# usage: listPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
sub listPorts($) {
    my $self = shift;

    my %Able = ();
    my %Link = ();
    my %auto = ();
    my %speed = ();
    my %duplex = ();

    my $ifTable = ["ifAdminStatus",0];

    #
    # Get the ifAdminStatus (enabled/disabled) and ifOperStatus
    # (up/down)
    #
    my ($varname, $modport, $ifIndex, $portIndex, $status, $portname);
    $self->{SESS}->getnext($ifTable);
    do {
	($varname,$ifIndex,$status) = @{$ifTable};
	$self->debug("$varname $ifIndex $status\n");
	if ($varname =~ /AdminStatus/) { 
	    $Able{$ifIndex} = ($status =~/up/ ? "yes" : "no");
	}
	$self->{SESS}->getnext($ifTable);
    } while ( $varname =~ /^ifAdminStatus$/) ;

    #
    # Get the port configuration, including speed, duplex, and whether or not
    # it is autoconfiguring
    #
    foreach $ifIndex (keys %Able) {
	$portIndex = $self->{PORTINDEX}{$ifIndex};
	if ($status = $self->{SESS}->get(["ifOperStatus",$ifIndex])) {
	    $Link{$ifIndex} = $status;
	}
	if ($status = $self->{SESS}->get(["rcPortOperDuplex",$ifIndex])) {
	    $duplex{$ifIndex} = $status;
	}
	if ($status = $self->{SESS}->get(["rcPortOperSpeed",$ifIndex])) {
	    $speed{$ifIndex} = $status;
	}
    };

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %Able ) {
	$modport = $self->{IFINDEX}{$id};
	$portname = $self->{NAME} . ":$modport";
	my $port = portnum($portname);

	#
	# Skip ports that don't seem to have anything interesting attached
	#
	if (!$port && $self->{DOALLPORTS}) {
#		$port = $portname;
		$port = "n:$modport";
	}
	if (!$port) {
	    $self->debug("$id ($modport) not connected, skipping\n");
	    next;
	}
	if (! defined ($speed{$id}) ) { $speed{$id} = " "; }
	if (! defined ($duplex{$id}) ) { $duplex{$id} = " "; }
	push @rv, [$port,$Able{$id},$Link{$id},$speed{$id},$duplex{$id}];
    }
    return @rv;
}

# 
# Get statistics for ports on the switch
#
# usage: getPorts($self)
# see snmpit_cisco_stack.pm for a description of return value format
#
#
sub getStats ($) {
    my $self = shift;

    my $ifTable = ["ifInOctets",0];
    my %inOctets=();
    my %inUcast=();
    my %inNUcast=();
    my %inDiscard=();
    my %inErr=();
    my %inUnkProt=();
    my %outOctets=();
    my %outUcast=();
    my %outNUcast=();
    my %outDiscard=();
    my %outErr=();
    my %outQLen=();
    my ($varname, $port, $value);

    #
    # Walk the whole stats tree, and fill these ha
    #
    $self->{SESS}->getnext($ifTable);
    do {
	($varname,$port,$value) = @{$ifTable};
	$self->debug("getStats: Got $varname, $port, $value\n");
	    if ($varname =~ /InOctets/) {
		$inOctets{$port} = $value;
	    } elsif ($varname =~ /InUcast/) {
		$inUcast{$port} = $value;
	    } elsif ($varname =~ /InNUcast/) {
		$inNUcast{$port} = $value;
	    } elsif ($varname =~ /InDiscard/) {
		$inDiscard{$port} = $value;
	    } elsif ($varname =~ /InErrors/) {
		$inErr{$port} = $value;
	    } elsif ($varname =~ /InUnknownP/) {
		$inUnkProt{$port} = $value;
	    } elsif ($varname =~ /OutOctets/) {
		$outOctets{$port} = $value;
	    } elsif ($varname =~ /OutUcast/) {
		$outUcast{$port} = $value;
	    } elsif ($varname =~ /OutNUcast/) {
		$outNUcast{$port} = $value;
	    } elsif ($varname =~ /OutDiscard/) {
		$outDiscard{$port} = $value;
	    } elsif ($varname =~ /OutErrors/) {
		$outErr{$port} = $value;
	    } elsif ($varname =~ /OutQLen/) {
		$outQLen{$port} = $value;
	    }
	$self->{SESS}->getnext($ifTable);
    } while ( $varname =~ /^i[f](In|Out)/) ;

    #
    # Put all of the data gathered in the loop into a list suitable for
    # returning
    #
    my @rv = ();
    foreach my $id ( keys %inOctets ) {
	$port = portnum($self->{NAME} . ":" . $self->{IFINDEX}{$id});

	#
	# Skip ports that don't seem to have anything interesting attached
	#
	if (!$port) {
	    $self->debug("$id does not seem to be connected, skipping\n");
	    next;
	}
	push @rv, [$port,$inOctets{$id},$inUcast{$id},$inNUcast{$id},
		$inDiscard{$id},$inErr{$id},$inUnkProt{$id},$outOctets{$id},
		$outUcast{$id},$outNUcast{$id},$outDiscard{$id},$outErr{$id},
		$outQLen{$id}];

    }
    return @rv;

}

#
# Used to flush FDB entries easily
#
# usage: resetVlanIfOnTrunk(self, modport, vlan)
#
sub resetVlanIfOnTrunk($$$) {
    my ($self, $modport, $vlan) = @_;
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    $self->debug($self->{NAME} . ":resetVlansIfOnTrunk m $modport "
		    . "vlan $vlan ifIndex $ifIndex\n",1);
    my $vlans = $self->get1("rcVlanPortVlanIds", $ifIndex);
    if (defined($vlans)) {
	my @vlanList = unpack "n*", $vlans;
	if (grep {$_ == $vlan;} @vlanList) {
	    $self->setVlansOnTrunk($modport,0,$vlan);
	    $self->setVlansOnTrunk($modport,1,$vlan);
	}
    }
    return 0;
}

#
# Get the ifindex for an EtherChannel (trunk given as a list of ports)
#
# usage: getChannelIfIndex(self, ports)
#        Returns: undef if more than one port is given, and no channel is found
#           an ifindex if a channel is found and/or only one port is given
#
# N.B. by Sklower - cisco's use this and so it gets called from _stack.pm
#
sub getChannelIfIndex($@) {
    my $self = shift;
    my @ports = @_;
    my @ifIndexes = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@ports);

    my $ifindex = undef;

    #
    # Try to get a channel number for each one of the ports in turn - we'll
    # take the first one we get
    #
    foreach my $port (@ifIndexes) {
        if ($port) { $ifindex = $port; last; }
    }
    return $ifindex;
}


#
# Enable, or disable,  port on a trunk
#
# usage: setVlansOnTrunk(self, modport, value, vlan_numbers)
#        modport: module.port of the trunk to operate on
#        value: 0 to disallow the VLAN on the trunk, 1 to allow it
#	 vlan_numbers: An array of 802.1Q VLAN numbers to operate on
#        Returns 1 on success, 0 otherwise
#
sub setVlansOnTrunk($$$$) {
    my ($self, $modport, $value, @vlan_numbers) = @_;
    my ($portindex,$RetVal);
    my $errors = 0;
    my $id = $self->{NAME} . "::setVlansOnTrunk";

    #
    # Some error checking
    #
    if (($value != 1) && ($value != 0)) {
	die "Invalid value $value passed to setVlansOnTrunk\n";
    }
    if (grep(/^1$/,@vlan_numbers)) {
	die "VLAN 1 passed to setVlansOnTrunk\n";
    }
    $self->debug("$id: m $modport v $value nums @vlan_numbers\n");
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    $portindex = $self->{PORTINDEX}{$ifIndex};

    #
    # Make sure ostensible trunk is either trunk or dual mode
    #
    $RetVal = $self->get1("rcVlanPortType",$portindex);
    if (defined($RetVal) && ($RetVal eq "access")) {
	warn "$id: port $modport is normal port, " .
		"refusing to add vlan(s) @vlan_numbers\n";
	return 0;
    }

    foreach my $vlan_number (@vlan_numbers) {
	$RetVal = ($value == 1) ? $self->setPortVlan($vlan_number,$modport) :
				    $self->delPortVlan($vlan_number,$modport);
	if ($RetVal) {
	    $errors++;
	    warn "$id:couldn't add/remove port $modport on vlan $vlan_number\n";
	}
    }
    return !$errors;
}
#
# Clear the list of allowed VLANs from a trunk
#
# usage: clearAllVlansOnTrunk(self, modport)
#        modport: module.port of the trunk to operate on
#        Returns 1 on success, 0 otherwise
#
sub clearAllVlansOnTrunk($$) {
    my ($self, $modport) = @_;
    my ($portIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    my $errors = 0;

    my $vlans = $self->get1("rcVlanPortVlanIds", $portIndex);
    if (defined($vlans)) {
	my @vlaninfo = unpack "n*", $vlans;
	while (scalar @vlaninfo) {
	    my $number = pop @vlaninfo;
	    my $RetVal = $self->delPortVlan($number , $modport);
	    if (!defined($RetVal) || ! $RetVal ) {
		print STDERR "Couldn't remove $modport from VLAN $number\n";
		$errors++;
	    }
	}
    } else  { $errors++; }
    return !$errors;
}
#
# Enable trunking on a port
#
# usage: enablePortTrunking(self, modport, nativevlan)
#        modport: module.port of the trunk to operate on
#        nativevlan: VLAN number of the native VLAN for this trunk
#        Returns 1 on success, 0 otherwise
#
sub enablePortTrunking($$$) {
    my $self = shift;
    my ($port,$native_vlan) = @_;

    if (!defined($native_vlan) || ($native_vlan <= 1)) {
	warn "Error: innappropriate or missing PVID for trunk\n";
	return 0;
    }
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    my $portIndex = $self->{PORTINDEX}{$ifIndex};
    #
    # temporarily set port type to "access" and add to native_vlan
    # portSetVlan will clear out all other vlans and set the PVID
    #
    my $portType = ["rcVlanPortType",$portIndex,1,"INTEGER"];
    my $rv = $self->{SESS}->set($portType);
    if (!defined($rv)) {
	warn "enablePortTrunking: Unable to set trunk port type to access\n";
	return 0;
    }
    $rv = $self->setPortVlan($native_vlan, $port);
    if ($rv) {
	warn "ERROR: Unable to add Trunk $port to PVID $native_vlan\n";
	return 0;
    } 
    #
    # Set port type to "untagPvidOnly"
    #
    $portType = ["rcVlanPortType",$portIndex,5,"INTEGER"];
    $rv = $self->{SESS}->set($portType);
    if (!defined($rv)) {
	warn "enablePortTrunking: Unable to set port type to untagPvidOnly\n";
	return 0;
    }
    $rv = $self->{SESS}->get("rcVlanPortType.$portIndex");
    return 1;
}

#
# Disable trunking on a port
#
# usage: disablePortTrunking(self, modport)
#        modport: module.port of the trunk to operate on
#        Returns 1 on success, 0 otherwise
#
sub disablePortTrunking($$) {
    my $self = shift;
    my ($port) = @_;

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    my $portIndex = $self->{PORTINDEX}{$ifIndex};
    my $native_vlan = $self->{SESS}->get("rcVlanPortDefaultVlanId.$portIndex");

    #
    # Set port type back to "access", and adding it to its native Vlan
    # will remove it from any other vlans to which it may belong
    #
    my $portType = ["rcVlanPortType",$portIndex,1,"INTEGER"];
    my $rv = $self->{SESS}->set($portType);
    if (!defined($rv)) {
	warn "ERROR: Unable to set port type to access\n";
	return 0;
    }
    $rv = $self->{SESS}->set($portType);
    if (!defined($native_vlan) || ($native_vlan == 0)) {
	$native_vlan = 1;
    }
    $rv = $self->setPortVlan($native_vlan, $port);
	
    return ($rv == 0);
}

# for accellar's this function is not necessary, the port index is the ifIndex
# it might be necessary for 5510's.
#
# Reads the IfIndex table from the switch, for SNMP functions that use 
# IfIndex rather than the module.port style. Fills out the objects IFINDEX
# members,
#
# usage: readifIndex(self)
#        returns nothing but sets instance variables IFINDEX and PORTINDEX
#
sub readifIndex($) {
    my $self = shift;
    my ($name,$ifindex,$iidoid);
    my ($oidbits, $oidshift) = (15, 4);
    $self->debug($self->{NAME} . "readifIndex:\n",2);

    if (getDeviceType($self->{NAME}) =~ /^nortel5/) {
	($oidbits, $oidshift) = (63, 6);
    }
    my ($rows) = snmpitBulkwalkFatal($self->{SESS}, ["rcPortIndex"]);
    foreach my $rowref (@$rows) {
	($name,$ifindex,$iidoid) = @$rowref;
	$self->debug("got $name, $ifindex, iidoid $iidoid ",3);
	my $port = $iidoid & $oidbits;
	my $mod = 1 + ($iidoid >> $oidshift);
	my $modport = "$mod.$port";
	my $portindex = $iidoid ;
	$self->{IFINDEX}{$modport} = $ifindex;
	$self->{IFINDEX}{$ifindex} = $modport;
	$self->{PORTINDEX}{$modport} = $portindex;
	$self->{PORTINDEX}{$ifindex} = $portindex;
	$self->debug("mod $mod, port $port, ",2);
	$self->debug("modport $modport, portindex $portindex\n",3);
    }
}


#
# Read a set of values for all given ports.
#
# usage: getFields(self,ports,oids)
#        ports: Reference to a list of ports, in any allowable port format
#        oids: A list of OIDs to reteive values for
#
# On sucess, returns a two-dimensional list indexed by port,oid
#
sub getFields($$$) {
    my $self = shift;
    my ($ports,$oids) = @_;

    my @ifindicies = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@$ports);
    my @oids = @$oids;


    #
    # Put together an SNMP::VarList for all the values we want to get
    #
    my @vars = ();
    foreach my $ifindex (@ifindicies) {
	foreach my $oid (@oids) {
	    push @vars, ["$oid","$ifindex"];
	}
    }

    #
    # If we try to ask for too many things at once, we get back really bogus
    # errors. So, we limit ourselves to an arbitrary number that, by
    # experimentation, works.
    #
    my $maxvars = 16;
    my @results = ();
    while (@vars) {
	my $varList = new SNMP::VarList(splice(@vars,0,$maxvars));
	my $rv = $self->{SESS}->get($varList);
	push @results, @$varList;
    }
	    
    #
    # Build up the two-dimensional list for returning
    #
    my @return = ();
    foreach my $i (0 .. $#ifindicies) {
	foreach my $j (0 .. $#oids) {
	    my $val = shift @results;
	    $return[$i][$j] = $$val[2];
	}
    }

    return @return;
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
