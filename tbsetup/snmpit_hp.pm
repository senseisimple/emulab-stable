#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2000-2009 University of Utah and the Flux Group.
# Copyright (c) 2004-2009 Regents, University of California.
# All rights reserved.
#

#
# snmpit module for HP procurve level 2 switches
#

package snmpit_hp;
use strict;

$| = 1; # Turn off line buffering on output

use English;
use SNMP;
use snmpit_lib;

use libtestbed;


#
# These are the commands that can be passed to the portControl function
# below
#
my %cmdOIDs =
(
    "enable" => ["ifAdminStatus","up"],
    "disable"=> ["ifAdminStatus","down"],
    "1000mbit"=> ["hpSwitchPortFastEtherMode","auto-1000Mbits"],
    "100mbit"=> ["hpSwitchPortFastEtherMode","auto-100Mbits"],
    "10mbit" => ["hpSwitchPortFastEtherMode","auto-10Mbits"],
    "auto"   => ["hpSwitchPortFastEtherMode","auto-neg"],
    "full"   => ["hpSwitchPortFastEtherMode","full"],
    "half"   => ["hpSwitchPortFastEtherMode","half"],
);

#
# some long OIDs that get used frequently.
#
my $normOID = "dot1qVlanStaticUntaggedPorts";
my $forbidOID = "dot1qVlanForbiddenEgressPorts";
my $egressOID = "dot1qVlanStaticEgressPorts";
my $aftOID = "dot1qPortAcceptableFrameTypes";
my $createOID = "dot1qVlanStaticRowStatus";

#
# Openflow OIDs, only number format now.
#
my $ofOID = 'iso.org.dod.internet.private.enterprises.11.2.14.11.5.1.7.1.35';
my $ofEnableOID     = $ofOID.'.1.1.2';
my $ofControllerOID = $ofOID.'.1.1.3';
my $ofListenerOID   = $ofOID.'.1.1.4';
my $ofSupportOID    = $ofOID.'.2.1.0';

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
    # set up hashes for internal use
    #
    $self->{IFINDEX} = {};
    $self->{TRUNKINDEX} = {};
    $self->{TRUNKS} = {};

    # other global variables
    $self->{DOALLPORTS} = 0;
    $self->{DOALLPORTS} = 1;
    $self->{SKIPIGMP} = 1;

    if ($self->{DEBUG}) {
	print "snmpit_hp initializing $self->{NAME}, " .
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
		       "$mibpath/IF-MIB.txt", "$mibpath/BRIDGE-MIB.txt", 
		       "$mibpath/HP-ICF-OID.txt");
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
    my $test_case = $self->get1("sysObjectID", 0);
    if (!defined($test_case)) {
	warn "WARNING: Unable to retrieve via SNMP from $self->{NAME}\n";
	return undef;
    }
    $self->{HPTYPE} = SNMP::translateObj($test_case);

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

sub set($$;$$) {
    my ($self, $varbind, $id, $retries) = @_;
    if (!defined($id)) { $id = $self->{NAME} . ":set "; }
    if (!defined($retries)) { $retries = 2; }
    my $sess = $self->{SESS};
    my $closure = sub () {
	my $RetVal = $sess->set($varbind);
	my $status = $RetVal;
	if (!defined($RetVal)) {
	    $status = "(undefined)";
	    if ($sess->{ErrorNum}) {
		my $bad = "$id had error number " . $sess->{ErrorNum} .
			  " and had error string " . $sess->{ErrorStr} . "\n";
		print $bad;
	    }
	}
	return $RetVal;
    };
    my $RetVal = $self->hammer($closure, $id, $retries);
    return $RetVal;
}

sub mirvPortSet($) {
    my ($bitfield) = @_;
    my $unpacked = unpack("B*",$bitfield);
    return [split //, $unpacked];
}

sub testPortSet($$) {
    my ($bitfield, $index) = @_;
    return @{mirvPortSet($bitfield)}[$index];
}

#
# opPortSet($op, $bitfield, @indices)
# set or clear bits in a port set,  $op > 0 means set, otherwise clear
#
sub opPortSet($$@) {
    my ($op, $bitfield, @indices) = @_;
    my @bits = mirvPortSet($bitfield);

    foreach my $index (@indices) { $bits[$index] = $op > 0 ? 1 : 0; }

    return pack("B*", join('',@bits));
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

sub bitSetToList($)
{
    my ($arrayref) = @_;
    my @ports = ();
    my $max = scalar (@$arrayref);

    for (my $port = 0; $port < $max; $port++)
	{ if (@$arrayref[$port]) { push @ports, (1 + $port); } }
    return @ports;
}

sub portSetToList($$) {
    my ($self, $portset) = @_;
    return bitSetToList(mirvPortSet($portset));
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
	$self->debug("Unsupported port control command '$cmd' ignored.\n");
	return 0;
    }
}

#
# HP's refuse to create vlans with display names that can
# be interpreted as vlan numbers
#
sub convertVlanName($) {
	my $id = shift;
	my $new;
	if ( $id =~ /^_(\d+)$/) {
	    $new = $1;
	    return ((($new > 0) && ($new  < 4095)) ? $new : $id);
	}
	if ( $id =~ /^(\d+)$/) {
	    $new = $1;
	    return ((($new > 0) && ($new  < 4095)) ? "_$new" : $id);
	}
	return $id;
}

#
# Try to pull a VLAN number out of a long OID string
#
sub parseVlanNumberFromOID($) {
    my ($oid) = @_;
    # OID must be a dotted string
    my (@elts) = split /\./, $oid;
    if (scalar(@elts) < 2) {
        return undef;
    }
    # Second-to-last element must be the right text string or numeric ID
    if ($elts[$#elts-1] eq "dot1qVlanStaticName" || $elts[$#elts-1] eq "1") {
        # Last element must be numeric
        if ($elts[$#elts] =~ /\d+/) {
            return $elts[$#elts];
        } else {
            return undef;
        }
    } else {
        return undef;
    }
}

sub checkLACP($$) {
   my ($self, $port) = @_;
   if (my $j = $self->{TRUNKINDEX}{$port})
       { $port = $j + $self->{TRUNKOFFSET}; }
   return $port;
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
	if ($input == $PORT_FORMAT_IFINDEX) {
	    return map $self->checkLACP($_), @ports;
	}
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

    my $rv = $self->get1("dot1qVlanStaticRowStatus", $vlan_number);
    if (!defined($rv) || !$rv || $rv ne "active") {
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
    $self->debug("$id\n");

    if ($count > 0) { @mapping{@vlan_ids} = undef; }

    #
    # Find all VLAN names. Do one to get the first field...
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32, ["dot1qVlanStaticName"]);
    foreach my $rowref (@$rows) {
	($name,$vlan_number,$vlan_name) = @$rowref;
	$self->debug("$id: Got $name $vlan_number $vlan_name\n",2);
        # Hack to get around some strange behavior
        if ((!defined($vlan_number) || $vlan_number eq "") &&
                defined(parseVlanNumberFromOID($name))) {
            $vlan_number = parseVlanNumberFromOID($name);
            $self->debug("Changed vlan_number to $vlan_number\n",3);
        }
	$vlan_name = convertVlanName($vlan_name);
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
sub createVlan($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $vlan_number = shift;
    my $id = $self->{NAME} . ":createVlan";

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
    my $hpvlan_id = convertVlanName("$vlan_id");
    my $VlanName = 'dot1qVlanStaticName'; # vlan # is index
    $self->debug("createVlan: name $vlan_id number $vlan_number \n");
    #
    # Perform the actual creation. Yes, this next line MUST happen all in
    # one set command....
    #
    my $closure = sub () {
    	my $RetVal = $self->set
	       ([[$VlanName,$vlan,$hpvlan_id,"OCTETSTR"],
	     [$createOID,$vlan, "createAndGo","INTEGER"]], "$id: creation");
	if (defined($RetVal)) { return $RetVal; }
	#
	# Sometimes we loose responses, or on the second time around
	# it might refuse to create a vlan that's already there, so wait
	# a bit to see if it exists (also so as to not get too
	# agressive with the switch which can cause to crash with
	# certain sets, e.g. IGMP stuff)
	#
	sleep (2);
	$RetVal = $self->get1($createOID, $vlan);
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
    my $IgmpEnable = 'hpSwitchIgmpState';
    $RetVal = $self->get1($IgmpEnable, $vlan);

    $closure = sub () {
	my $check = $self->set([[$IgmpEnable,$vlan,"enable","INTEGER"]]);
	if (!defined($check) || ($check ne "enable"))
		{ sleep (19); return undef;}
	return $check;
    };
    $RetVal = $self->hammer($closure, "$id: setting snooping");
    if (!defined($RetVal)) { return 0; }

    $closure = sub () {
	my $check = $self->get1($IgmpEnable, $vlan);
	if (!defined($check) || ($check ne "enable"))
		{ sleep (4); return undef ;}
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
		my @oldList = $self->portSetToList($SetVal);
		my @newList = $self->portSetToList($check);
		print "SetCheckPortSet result for vlan # $vlan_number " 
		. "differs from what was set -  old is @oldList\n " 
		. "new is @newList \n";
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
# gets the forbidden, untagged, and egress lists for a vlan
# sends back as a 3 element array of lists.  (Thats the order
# the packet traces for the HP had them in).
#
sub getVlanLists($$) {
    my ($self, $vlan) = @_;
    my $ret = [0, 0, 0];
    @$ret[0] = mirvPortSet($self->get1($forbidOID, $vlan));
    @$ret[1] = mirvPortSet($self->get1($normOID, $vlan));
    @$ret[2] = mirvPortSet($self->get1($egressOID, $vlan));
    return $ret;
}

#
# sets the forbidden, untagged, and egress lists for a vlan
# sends back as a 3 element array of lists.
# (Thats the order we saw in a tcpdump of vlan creation.)
# (or in some cases 6 elements for 2 vlans).
#
sub setVlanLists($@) {
    my ($self, @args) = @_;
    my $oids = [$forbidOID, $normOID, $egressOID];
    my $j = 0; my $todo = [ 0 ];
    while (@args) {
	my $vlan = shift @args;
	my $arrayref = shift @args;
	foreach my $i (0, 1, 2) {
	    @$todo[$j++] = [ @$oids[$i], $vlan,
			    pack("B*",join('', @{@$arrayref[$i]})), "OCTETSTR"];
	}
    }
    $j = $self->set($todo);
    if (!defined($j)) { print "vlists failed\n";}
    return $j;
}

#
# Put the given ports in the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: setPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
# hp specific: if the vlan_number < 0 *remove* the ports from the vlan
# No good place to write down the following crazy convention:
#
# A normal member of a vlan has its PVID set to that vlan, 
# has dot1QPortAceptableFrameType = admitAll. and only has 1 egress
# port among all vlans, namely on its PVID.  (I think ;-)
#
# If we want the port to be like dual mode on a foundry or cisco, we would
# merely add a the egress port on a second vlan.  There would be no way
# of telling the intent to permit this port to go into dual mode.
# since this is the common case, if we want to tell that when we directly
# move a normal port from one vlan to another that it should loose its
# membership in the first (and shake up all switches in the stack)
# we will adopt the convention that we will do this, unless the port
# is marked as being forbidden to join vlan 1.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $id = $self->{NAME} . "::setPortVlan";
    $self->debug("$id: $vlan_number ");
    my %vlansToPorts; # i.e. bumpedVlansToListOfPorts
    my @newTaggedPorts = ();
    my ($errors, $portIndex, $pvid) = (0,0,0);
	   
    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #

    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX, @ports);
    $self->debug("ports: " . join(",",@ports) . "\n");
    $self->debug("as ifIndexes: " . join(",",@portlist) . "\n");

    #
    # Need to determine status and remove ports from default_vlan
    # before adding to other.  This is a read_modify_write so lock out
    # other instances of snmpit_hp.

    $self->lock();
    my $defaultInfo = $self->getVlanLists(1);
    foreach $portIndex (@portlist) {
	# check for three easy cases
	# a. known dual port.
	# b. the ports is not allocated
	# c. the port is a trunk

	# This is a dual port, so it doesn't have to leave its PVID.
	if (@{@$defaultInfo[0]}[$portIndex - 1]) {
	    push @newTaggedPorts, $portIndex;
	    next;
	}
	# Unallocated untrunked port.
	if (@{@$defaultInfo[1]}[$portIndex - 1]) {
	    $pvid = 1;
	} else {
	    my $tagOnly = $self->get1($aftOID,$portIndex);
	    if ($tagOnly eq "admitOnlyVlanTagged") {
		# case c: Trunk Port
		push @newTaggedPorts, $portIndex;
		next;
	    } else {
		# case d: untrunked port leaving another vlan.
		# a little more work - get its PVID and assume that
		# is the vlan which it is leaviing. and toss it
		# into the bumpedVlan hash.
		$pvid = $self->get1("dot1qPvid", $portIndex);
	    }
	}
	if (defined($pvid) && ($pvid != $vlan_number)) {
	    if (!exists($vlansToPorts{$pvid})) {
		@{$vlansToPorts{$pvid}} = ();
	    }
	    push @{$vlansToPorts{$pvid}}, $portIndex;
	}
    }
    @{$self->{DISPLACED_VLANS}} = grep {$_ != 1;} keys %vlansToPorts;
 
    my $newInfo = $self->getVlanLists($vlan_number);
    foreach my $vlan (keys %vlansToPorts) {
	my $oldInfo = $vlan == 1 ? $defaultInfo : $self->getVlanLists($vlan) ;
	foreach $portIndex (@{$vlansToPorts{$vlan}}) {
	    @{@$oldInfo[1]}[$portIndex-1] = 0;
	    @{@$newInfo[1]}[$portIndex-1] = 1;
	    @{@$oldInfo[2]}[$portIndex-1] = 0;
	    @{@$newInfo[2]}[$portIndex-1] = 1;
	}
	$self->setVlanLists($vlan, $oldInfo, $vlan_number, $newInfo);
    }

    # Now add tagged ports separately, just to be safe.

    if (@newTaggedPorts) {
	foreach $portIndex (@newTaggedPorts)
		{ @{@$newInfo[2]}[$portIndex-1] = 1; }
	$self->setVlanLists($vlan_number, $newInfo);
    }
    $self->unlock();

    #
    # We need to make sure the ports get enabled.
    #

    $self->debug("Enabling "  . join(',',@ports) . "...\n");
    if ( my $rv = $self->portControl("enable",@ports) ) {
	print STDERR "Port enable had $rv failures.\n";
	$errors += $rv;
    }

    if ($self->{SKIPIGMP}) { return $errors; }

#   Old nortel code for inspiration.
#    $self->lock();
#    my $igmp = "rcVlanIgmpVer1SnoopMRouterPorts";
#    my $IgnoredVal = $self->{SESS}->get("$igmp.$vlan_number");
#    $errors += $self->snmpSetCheckPortSet($igmp, $vlan_number, $SetVal);
#    $self->unlock();

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

    $self->debug($self->{NAME} . "::delPortVlan $vlan_number ");
	   
    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #

    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX, @ports);
    $self->debug("ports: " . join(",",@ports) . "\n");
    $self->debug("as ifIndexes: " . join(",",@portlist) . "\n");

    $self->lock();
    my $vlist = $self->getVlanLists($vlan_number);
    foreach my $port (@portlist) {
	@{@$vlist[1]}[$port - 1] = 0;
	@{@$vlist[2]}[$port - 1] = 0;
    }
    my $result = $self->setVlanLists($vlan_number, $vlist);
    $self->unlock();
    return defined($result) ? 0 : 1;
}

#
# Disables all ports in the given VLANS. Each VLAN is given as a VLAN
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

    foreach my $vlan_number (@vlan_numbers) {
	#
	# Do two passes: 1st untrunk any dual-mode ports on this vlan.
	# 2nd, remove all ports.
	#
	my $dualPorts = mirvPortSet($self->get1($forbidOID, 1)); # array
	my @portlist = $self->portSetToList
			($self->get1($normOID, $vlan_number));
	foreach my $portIndex (@portlist) {
		if (@$dualPorts[$portIndex-1])
		    { $self->disablePortTrunking($portIndex);}
	}
	$self->lock();
	my $defaultLists = $self->getVlanLists(1);
	my $vLists = $self->getVlanLists($vlan_number);
	@portlist = bitSetToList(@$vLists[2]);
	$self->debug("$id $vlan_number: @portlist\n");

	foreach my $portIndex (@portlist) {
	    #
	    # If this port is not listed as a tagged member of the vlan,
	    # then it could have only gotten here either as a trunk
	    # or dual-mode port from another vlan, so don't disable it.
	    #
	    if (@{@$vLists[1]}[$portIndex - 1]) {
		@{@$defaultLists[1]}[$portIndex - 1] = 1;
		@{@$defaultLists[2]}[$portIndex - 1] = 1;
		$self->debug("disabling port $portIndex  "
				. "from vlan $vlan_number \n" );
		$self->set(["ifAdminStatus",$portIndex,"down","INTEGER"],$id);
	    }
	    @{@$vLists[1]}[$portIndex - 1] = 0;
	    @{@$vLists[2]}[$portIndex - 1] = 0;
	}
	$errors += $self->setVlanLists($vlan_number, $vLists, 1, $defaultLists);
	$self->unlock();
    }
    return $errors;
}

#
# Removes and disables some ports in a given VLAN. The VLAN is given as a VLAN
# 802.1Q tag value.  Ports are known to be regular ports and not trunked.
#
# usage: removeSomePortsFromVlan(self,vlan,@ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub removeSomePortsFromVlan($$@) {
    my ($self, $vlan_number, @ports) = @_;
    my ($errors, $changes, $id, %porthash) =
	(0, 0, $self->{NAME} . "::removeSomePortsFromVlan");

    @ports = $self->convertPortFormat($PORT_FORMAT_IFINDEX,@ports);
    @porthash{@ports} = @ports;

    $self->lock();
    my $defaultLists = $self->getVlanLists(1);
    my $vLists = $self->getVlanLists($vlan_number);
    my @portlist = bitSetToList(@$vLists[2]);
    $self->debug("$id $vlan_number: @portlist\n",2);

    foreach my $portIndex (@portlist) {
	next unless $porthash{$portIndex};
	if (@{@$vLists[1]}[$portIndex - 1]) {
	    # otherwise, port is tagged, or dual; maybe should complain.

	    @{@$defaultLists[1]}[$portIndex - 1] = 1;
	    @{@$defaultLists[2]}[$portIndex - 1] = 1;
	    $self->debug("disabling port $portIndex  "
			    . "from vlan $vlan_number \n" );
	    $self->set(["ifAdminStatus",$portIndex,"down","INTEGER"],$id);
	}
	@{@$vLists[1]}[$portIndex - 1] = 0;
	@{@$vLists[2]}[$portIndex - 1] = 0;
	$changes++;
    }
    $errors += $self->setVlanLists($vlan_number, $vLists, 1, $defaultLists)
	    if ($changes > 0);
    $self->unlock();
    return $errors;
}

#
# Remove the given VLANs from this switch. Removes all ports from the VLAN,
# It's necessary to call removePortsFromVlan() first on HP's. The VLAN is
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
    my $name = $self->{NAME};

    foreach my $vlan_number (@vlan_numbers) {
	#
	# Perform the actual removal
	#
	print "  Removing VLAN # $vlan_number ... ";
	my $RetVal = $self->set([[$createOID,$vlan_number,"destroy","INTEGER"]]);
	if ($RetVal) {
	    print "Removed VLAN $vlan_number on switch $name.\n";
	} else {
	    print STDERR "Removing VLAN $vlan_number failed on switch $name.\n";
	}
	# The next call would buy time to let switch consolidate itself
	# Nortels have bizarre failures when quickly creating and destroying
	# vlans with IGMP snooping enabled.
	# $self->{SESS}->bulkwalk(0,32,[$createOID]);
    }
    return ($errors == 0) ? 1 : 0;
}

#
# XXX: Major cleanup
#
sub UpdateField($$$@) {
    my ($self, $OID, $val, @ports)= @_;
    my $id = $self->{NAME} . "::UpdateField OID $OID value $val";
    $self->debug("$id: ports @ports\n");

    my $result = 0;
    my $oidval = $val;
    my ($Status, $portname, $row);


    foreach $portname (@ports) {
	($row) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$portname);
	$self->debug("checking row $row for $val ...\n");
	$Status = $self->get1($OID,$row);
	if (!defined($Status)) {
	    print STDERR "id: Port $portname No answer from device\n";
	    next;
	}
	$self->debug("Port $portname, row $row was $Status\n");
	if (!($OID =~ /^ifAdmin/)) {
	    #
	    # Procurves use the same mib variable to set both
	    # speed and duplex concurrently; only certain
	    # combinations are permitted.  (We won't support
	    # auto-10-100MBits. And at least the 5400 series
	    # doesn't seem to support full-duplex-1000Mbits.)
	    #
	    my @state = split "-", $Status;
	    if (($val eq "half") || ($val eq "full")) {
		if ($state[0] eq "auto") {
		    if (($state[1] eq "neg") || ($state[1] eq "10")) {
			# can't autospeed with specific duplex.
			$oidval = ($val eq "half") ?
			   "half-duplex-100Mbits" : "full-duplex-100Mbits";
		    } elsif ($state[1] eq "1000Mbits") {
			$oidval = $Status;
		    } else {
			$oidval = $val . "-duplex-" . $state[1] ;
		    }
		} else {
			$oidval = $val . "-duplex-" . $state[2] ;
		}
	    } else {
		if (($val eq "auto-neg") || ($val eq "auto-1000Mbits") ||
			($state[1] ne "duplex")) {
		    $oidval = $val;
		} else {
		    my @valarr = split "-", $val;
		    $oidval = $state[0] . "-duplex-" . "$valarr[1]";
		}
	    }
	}
	if ($Status ne $oidval) {
	    $self->debug("Setting $portname (r $row) to $oidval...");
	    $Status = $self->set([[$OID,$row,$oidval,"INTEGER"]]);
	    $result =  (defined($Status)) ? 0 : -1;
	    $self->debug($result ? "failed.\n" : "succeeded.\n");
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

    my $portset = $self->get1($egressOID,$vlan_number);
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
    $self->debug($self->{NAME} . "::listVlans()\n",1);
    my $maxport = $self->{MAXPORT};

    #
    # Walk the tree to find the VLAN names
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32,"dot1qVlanStaticName");
    foreach $rowref (@$rows) {
	($oid, $vlan_number, $vlan_name) = @$rowref;
	$self->debug("Got $oid $vlan_number $vlan_name\n",3);
        # Hack to get around some strange behavior
        if ((!defined($vlan_number) || $vlan_number eq "") &&
                defined(parseVlanNumberFromOID($oid))) {
            $vlan_number = parseVlanNumberFromOID($oid);
            $self->debug("Changed vlan_number to $vlan_number\n",3);
        }
	if ($vlan_number eq "1") { next;}
	$vlan_name = convertVlanName($vlan_name);
	if (!$Names{$vlan_number}) {
	    $Names{$vlan_number} = $vlan_name;
	    @{$Members{$vlan_number}} = ();
	}
    }

    #
    #  Walk the tree for the VLAN members
    #
    ($rows) = $self->{SESS}->bulkwalk(0,32,"dot1qVlanStaticEgressPorts");
    foreach $rowref (@$rows) {
	($oid,$vlan_number,$value) = @$rowref;
	if ($vlan_number == 1) { next;}
	@portlist = $self->portSetToList($value);
	$self->debug("Got $oid $vlan_number @portlist\n",3);

	foreach $ifIndex (@portlist) {
	    ($node) = $self->convertPortFormat($PORT_FORMAT_NODEPORT,$ifIndex);
	    if (!$node) {
		($modport) = $self->convertPortFormat
				    ($PORT_FORMAT_MODPORT,$ifIndex);
		$modport =~ s/\./\//;
		$node = $self->{NAME} . ".$modport";
	    }
	    push @{$Members{$vlan_number}}, $node;
	    if (!$Names{$vlan_number}) {
		$self->debug("listVlans: WARNING: port $node in non-existant " .
		    "VLAN $vlan_number\n", 1);
	    }
	}
    }

    #
    # Build a list from the name and membership lists
    #
    my @list = ();
    foreach my $vlan_id (sort keys %Names) {
	push @list, [$Names{$vlan_id},$vlan_id,$Members{$vlan_id}];
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
	if (($ifIndex <= $self->{MAXPORT}) && ($varname =~ /AdminStatus/)) { 
	    $Able{$ifIndex} = ($status =~/up/ ? "yes" : "no");
	}
	$self->{SESS}->getnext($ifTable);
    } while ( $varname =~ /^ifAdminStatus$/) ;

    #
    # Get the port configuration, including speed, duplex, and whether or not
    # it is autoconfiguring
    #
    foreach $ifIndex (keys %Able) {
	if ($status = $self->{SESS}->get(["ifOperStatus",$ifIndex])) {
	    $Link{$ifIndex} = $status;
	}
	# HP combines speed and duplex and it has to be teased apart lexically.
	if ($status = $self->get1("hpSwitchPortFastEtherMode",$ifIndex)) {
	    my @parse = split("-duplex-", $status);
	    if (2 == scalar(@parse)) {
		$duplex{$ifIndex} = $parse[0];
		$speed{$ifIndex} = $parse[1];
	    } else {
		@parse = split("auto-",$status);
		$duplex{$ifIndex} = "auto";
		$speed{$ifIndex} = $parse[1];
		if ($speed{$ifIndex} eq "neg") {
		    $speed{$ifIndex} = "auto";
		}
	    }
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
		$modport =~ s/\./\//;
		$port = $self->{NAME} . ":$modport";
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
	    if ($port > $self->{MAXPORT}) {
		# do nothing.  There are entries for vlan interfaces, etc.
	    } elsif ($varname =~ /InOctets/) {
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
	my $modport = $self->{IFINDEX}{$id};
	$port = portnum($self->{NAME} . ":" . $modport);
	if (!$port && $self->{DOALLPORTS}) {
		$modport =~ s/\./\//;
		$port = $self->{NAME} . ".$modport";
	}
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
    $self->debug($self->{NAME} . "::resetVlanIfOnTrunk m $modport "
		    . "vlan $vlan ifIndex $ifIndex\n",1);
    my $vlan_ports = $self->get1($egressOID, $vlan);
    if (testPortSet($vlan_ports, $ifIndex - 1)) {
	$self->setVlansOnTrunk($modport,0,$vlan);
	$self->setVlansOnTrunk($modport,1,$vlan);
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
# N.B. by Sklower - cisco's use this to put vlans on multiwire trunks;
# it gets called from _stack.pm
#
# HP's also require a different ifindex for putting a vlan on a multiwire
# trunk from the individual ifindex from any constituent port.
#
# although Rob Ricci's vision is that this would only get called when putting
# vlans on multi-wire interswitch trunks and the check would happen in
# _stack, it is 1.) possible to use snmpit -i Switch <mod>/<port> to do
# maintenance functions of vlans and so you should check for each port
# any way, and 2.) the check is cheap and can be done in convertPortFormat.
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
    my ($RetVal);
    my $errors = 0;
    my $id = $self->{NAME} . "::setVlansOnTrunk";

    my @dualPorts = $self->portSetToList($self->get1($forbidOID,1));

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

    #
    # Make sure ostensible trunk is either trunk or dual mode
    #
    my $tagOnly = $self->get1($aftOID,$ifIndex);
    if (($tagOnly ne "admitOnlyVlanTagged") &&
			 $self->not_in($ifIndex, @dualPorts)) {
	warn "$id: port $modport is normal port, " .
		"refusing to add vlan(s) @vlan_numbers\n";
	return 0;
    }

    foreach my $vlan_number (@vlan_numbers) {
	$RetVal = ($value == 1) ? $self->setPortVlan($vlan_number,$modport) :
				    $self->delPortVlan($vlan_number,$modport);
	if ($RetVal) {
	    $errors++;
	    warn "$id:couldn't " .  (($value == 1) ? "add" : "remove") .
		    " port $modport on vlan $vlan_number\n" ;
	}
    }
    return !$errors;
}

#
# Enable trunking on a port
#
# usage: enablePortTrunking2(self, modport, nativevlan, equaltrunking)
#        modport: module.port of the trunk to operate on
#        nativevlan: VLAN number of the native VLAN for this trunk
#	 equaltrunk: don't do dual mode; tag PVID also.
#        Returns 1 on success, 0 otherwise
#
sub enablePortTrunking2($$$$) {
    my ($self,$port,$native_vlan,$equaltrunking) = @_;

    if ($equaltrunking) {
	if (!$native_vlan) { $native_vlan = 1; }
    } elsif (!defined($native_vlan) || ($native_vlan <= 1)) {
	warn "Error: innappropriate or missing PVID for trunk\n";
	return 0;
    }
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    #
    # portSetVlan will clear out all other vlans and set the PVID
    #
    my $rv = $self->setPortVlan($native_vlan, $port);
    if ($rv) {
	warn "ERROR: Unable to add Trunk $port to PVID $native_vlan\n";
	return 0;
    } 
    #
    # Set port type apropriately.
    #
    if ($equaltrunking) {
	my $portType = [$aftOID,$ifIndex,"admitOnlyVlanTagged","INTEGER"];
	$rv = $self->{SESS}->set($portType);
	if (!defined($rv)) {
	    warn "enablePortTrunking: Unable to set port type\n";
	    return 0;
	}
    } else {
	$self->lock();
	my $defLists = $self->getVlanLists(1);
	@{@$defLists[0]}[$ifIndex - 1] = 1;  # add to forbid list of default 
	$rv = $self->setVlanLists(1, $defLists);
	$self->unlock();
	if (!defined($rv)) {
	    warn "enablePortTrunking: Unable to set port type\n";
	    return 0;
	}
    }
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
    my ($self, $port) = @_;

    my ($portIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    my $native_vlan = $self->get1("dot1qPvid",$portIndex);
    if (!defined($native_vlan) || ($native_vlan == 0)) {
	$native_vlan = 1;
    }
    #
    # we have to walk all of the blasted vlans to find out
    # which ones have this port as a member of and remove it.
    #
    my ($rows) = $self->{SESS}->bulkwalk(0,32, [$egressOID]);
    foreach my $rowref (@$rows) {
	my ($name,$vlan_number,$portset) = @$rowref;
	if (testPortSet($portset, $portIndex - 1) &&
		   ($vlan_number != $native_vlan))
	    { $self->delPortVlan($vlan_number, $portIndex); }
    }

    $self->lock();
    #
    # clear annotation that this is a dual port
    #
    my $defLists = $self->getVlanLists(1);
    if (@{@$defLists[0]}[$portIndex - 1]) {
	@{@$defLists[0]}[$portIndex - 1] = 0;
	$self->setVlanLists(1, $defLists);
    }
    #
    # make port have untagged exit
    #
    if ($native_vlan != 1) {
	$defLists = $self->getVlanLists($native_vlan);
    }
    if (@{@$defLists[1]}[$portIndex - 1] == 0) {
	@{@$defLists[1]}[$portIndex - 1] = 1;
	@{@$defLists[2]}[$portIndex - 1] = 1;
	$self->setVlanLists($native_vlan, $defLists);
    }
    $self->unlock();

    my $portType = [$aftOID,$portIndex,"admitAll","INTEGER"];
    my $rv = $self->{SESS}->set($portType);
    if (!defined($rv)) {
	warn "ERROR: Unable to set port type to access\n";
	return 0;
    }
    return 1;
}

my %blade_sizes = ( 
   hpSwitchJ8697A => 24, # hp5406zl
   hpSwitchJ8698A => 24, # hp5412zl
   hpSwitchJ8770A => 24, # hp4204
   hpSwitchJ8773A => 24 # hp4208
);

#
# Reads the IfIndex table from the switch, for SNMP functions that use 
# IfIndex rather than the module.port style. Fills out the objects IFINDEX
# members,
#
# usage: readifIndex(self)
#        returns nothing but sets the instance variable IFINDEX.
#
# TODO: XXXXXXXXXXXXXXXXXXXXXXX - the 288 is a crock; 
# for some reason doing an swalk of ifType returns 161 instead of
# "ieee8023adLag"; we should walk ifType look for the least ifIndex of
# that type and walk hpSwitchPortTrunkType looking for the least lacpTrk
# to figure out the offset.

sub readifIndex($) {
    my $self = shift;
    my ($maxport, $maxtrunk, $name, $ifindex, $iidoid, $port, $mod, $j) = (0,0);
    $self->debug($self->{NAME} . "::readifIndex:\n", 2);

    my $bladesize = $blade_sizes{$self->{HPTYPE}};

    my ($rows) = snmpitBulkwalkFatal($self->{SESS}, ["hpSwitchPortTrunkGroup"]);
    my $t_off = $self->{TRUNKOFFSET} = 288;

    foreach my $rowref (@$rows) {
	($name,$ifindex,$iidoid) = @$rowref;
	$self->debug("got $name, $ifindex, iidoid $iidoid\n", 2);
	$self->{TRUNKINDEX}{$ifindex} = $iidoid;
	if ($iidoid) { push @{$self->{TRUNKS}{$iidoid}}, $ifindex; }
	if ($ifindex > $maxport) { $maxport = $ifindex;}
	if ($iidoid > $maxtrunk) { $maxtrunk = $iidoid;}
    }
    while (($ifindex, $iidoid) = each %{$self->{TRUNKINDEX}}) {
	if (defined($bladesize)) {
	    $j = $ifindex - 1;
	    $port = 1 + ($j % $bladesize);
	    $mod = 1 + int ($j / $bladesize);
	} else
	    { $mod = 1; $port = $ifindex; }
	my $modport = "$mod.$port";
	my $portindex = $iidoid ? ($t_off + $iidoid) : $ifindex ;
	$self->{IFINDEX}{$modport} = $portindex;
	$self->{IFINDEX}{$ifindex} = $modport;
    }
    foreach $j (keys %{$self->{TRUNKS}}) {
	$ifindex = $j + $t_off;
	$port = "1." . $ifindex;
	$self->{IFINDEX}{$ifindex} = $port;
	$self->{IFINDEX}{$port} = $ifindex;
	$self->{TRUNKINDEX}{$ifindex} = 0; # simplifies convertPortIndex
    }
    $self->{MAXPORT} = $maxport;
    $self->{MAXTRUNK} = $maxtrunk;
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

my $lock_held = 0;

sub lock($) {
    my $self = shift;
    my $token = "snmpit_" . $self->{NAME};
    if ($lock_held == 0) {
	my $old_umask = umask(0);
	die if (TBScriptLock($token,0,1800) != TBSCRIPTLOCK_OKAY());
	umask($old_umask);
    }
    $lock_held = 1;
}

sub unlock($) {
	if ($lock_held == 1) { TBScriptUnlock();}
	$lock_held = 0;
}

#
# Enable Openflow
#
sub enableOpenflow($$) {
    my $self = shift;
    my $vlan = shift;
    my $RetVal;
    
    $RetVal = $self->set([$ofEnableOID, $vlan, 1, "INTEGER"]);
    if (!defined($RetVal)) {
	warn "ERROR: Unable to enable Openflow on VLAN $vlan\n";
	return 0;
    }
    return 1;
}

#
# Disable Openflow
#
sub disableOpenflow($$) {
    my $self = shift;
    my $vlan = shift;
    my $RetVal;
    
    $RetVal = $self->set([$ofEnableOID, $vlan, 2, "INTEGER"]);
    if (!defined($RetVal)) {
	warn "ERROR: Unable to disable Openflow on VLAN $vlan\n";
	return 0;
    }
    return 1;
}

#
# Set controller
#
sub setController($$$) {
    my $self = shift;
    my $vlan = shift;
    my $controller = shift;
    my $RetVal;
    
    $RetVal = $self->set([$ofControllerOID, $vlan, $controller, "OCTETSTR"]);
    if (!defined($RetVal)) {
	warn "ERROR: Unable to set controller on VLAN $vlan\n";
	return 0;
    }
    return 1;
}

#
# Set listener
#
sub setListener($$$) {
    my $self = shift;
    my $vlan = shift;
    my $listener = shift;
    my $RetVal;
    
    $RetVal = $self->set([$ofListenerOID, $vlan, $listener, "OCTETSTR"]);
    if (!defined($RetVal)) {
	warn "ERROR: Unable to set listener on VLAN $vlan\n";
	return 0;
    }
    return 1;
}

#
# Check if Openflow is supported on this switch
#
sub isOpenflowSupported($) {
    my $self = shift;
    my $ret;

    #
    # Still don't know how to detect if Openflow is supported.
    # the ofOID is a directory. Maybe walking from it?
    #
    $ret = $self->get1($ofSupportOID, 1); # not really
    if (defined($ret)) {
	return 1;
    } else {
	return 0;
    }
}

# End with true
1;
