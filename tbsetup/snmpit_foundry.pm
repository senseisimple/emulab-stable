#!/usr/bin/perl -w

#
# EMULAB-LGPL
# Copyright (c) 2004, Regents, University of California.
# Modified from an Netbed/Emulab module, Copyright (c) 2000-2003, University of
# Utah
#

#
# snmpit module for Foundry level 2 switches
#

package snmpit_foundry;
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
#    "1000mbit"=> ["snSwPortInfoSpeed","s1G"], # doesn't work with FreeBSD
    "1000mbit"=> ["snSwPortInfoSpeed","sAutoSense"],
    "100mbit"=> ["snSwPortInfoSpeed","s100M"],
    "10mbit" => ["snSwPortInfoSpeed","s10M"],
    "auto"   => ["snSwPortInfoSpeed","sAutoSense"],
    "full"   => ["snSwPortInfoChnMode","fullDuplex"],
    "half"   => ["snSwPortInfoChnMode","halfDuplex"],
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
    $self->{NEW_NAMES} = {};
    $self->{NEW_NUMBERS} = {};
    $self->{IFINDEX} = {};
    $self->{PORTINDEX} = {};

    if ($self->{DEBUG}) {
	print "snmpit_foundry module initializing... debug level $self->{DEBUG}\n"
	;   
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
		       "$mibpath/FOUNDRY-SN-ROOT-MIB.txt");
    $SNMP::save_descriptions = 1; # must be set prior to mib initialization
    SNMP::initMib();		  # parses default list of Mib modules 
    $SNMP::use_enums = 1;	  # use enum values instead of only ints

    warn ("Opening SNMP session to $self->{NAME}...") if ($self->{DEBUG});

    $self->{SESS} = new SNMP::Session(DestHost => $self->{NAME},
	Community => $self->{COMMUNITY});

    if (!$self->{SESS}) {
	#
	# Bomb out if the session could not be established
	#
	warn "WARNING: Unable to connect via SNMP to $self->{NAME}\n";
	return undef;
    }
    #
    # Sometimes the SNMP session gets created when there is no connectivity
    # to the device so let's try something simple
    #
    my $test_case = $self->{SESS}->get("system.sysDescr.0");
    if (!defined($test_case)) {
	warn "WARNING: Unable to retrieve via SNMP from $self->{NAME}\n";
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

# Attempt to repeat an action until it succeeds

sub hammer($$$;$) {
    my ($self, $closure, $id, $retries) = @_;

    if (!defined($retries)) { $retries = 8; }
    for my $i (1 .. $retries) {
	my $result = $closure->();
	if (defined($result) || ($retries == 1)) { return $result; }
	if (defined($id)) { warn $id . " ... will try again\n"; }
	sleep 1;
    }
    if (defined($id)) { warn $id . " .. giving up\n"; }
    return undef;
}

# shorthand

sub get1($$$) {
    my ($self, $obj, $instance) = @_;
    my $id = "nortel::snmpitGetOnePersistently of $obj, $instance";
    my $closure = sub () {
	my $RetVal = snmpitGet($self->{SESS}, [$obj, $instance], 1);
	if (!defined($RetVal)) { sleep 2;}
	return $RetVal;
    };
    my $RetVal = $self->hammer($closure, $id, 10);
    if (!defined($RetVal)) {
	warn "$id failed - $snmpit_lib::snmpitErrorString\n";
    }
    return $RetVal;
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
	$self->debug("Not converting, input format = output format\n",2);
	return @ports;
    }

    if ($input == $PORT_FORMAT_IFINDEX) {
	if ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting ifindex to modport\n",2);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting ifindex to nodeport\n",2);
	    return map portnum($self->{NAME}.":".$self->{IFINDEX}{$_}), @ports;
	}
    } elsif ($input == $PORT_FORMAT_MODPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting modport to ifindex\n",2);
	    return map $self->{IFINDEX}{$_}, @ports;
	} elsif ($output == $PORT_FORMAT_NODEPORT) {
	    $self->debug("Converting modport to nodeport\n",2);
	    return map portnum($self->{NAME} . ":$_"), @ports;
	}
    } elsif ($input == $PORT_FORMAT_NODEPORT) {
	if ($output == $PORT_FORMAT_IFINDEX) {
	    $self->debug("Converting nodeport to ifindex\n",2);
	    return map $self->{IFINDEX}{(split /:/,portnum($_))[1]}, @ports;
	} elsif ($output == $PORT_FORMAT_MODPORT) {
	    $self->debug("Converting nodeport to modport\n",2);
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

    my $obj = "snVLanByPortCfgVLanName.$vlan_number";
    my $rv = $self->{SESS}->get($obj);
    if (!defined($rv) || !$rv || ($rv eq "NOSUCHINSTANCE")) {
	sleep 1;
	$rv = $self->{SESS}->get($obj);
	if (defined($rv) && $rv && ($rv ne "NOSUCHINSTANCE")) {
	    return 1;
	} else { return 0; }
    }
    return 1;
}

#
# Given VLAN indentifiers from the database, finds the 802.1Q VLAN
# number for them. If no VLAN id is given, returns mappings for the entire
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
    my ($name, $id, $vlan_number, $vlan_name);

    if (!@vlan_ids) {
	while ( ($id, $name) = each %{$self->{NEW_NAMES}} ) {
		$mapping{$id} = $name;
	}
    } else  {
	@mapping{@vlan_ids} = undef;
	foreach $id (@vlan_ids) {
	    if (defined($self->{NEW_NAMES}->{$id})) {
		$mapping{$id} = $self->{NEW_NAMES}->{$id};
	    }
	}
    }
    #
    # Find all VLAN names. Do one to get the first field...
    #
    my $field = ["snVLanByPortCfgVLanName",0];
    $self->{SESS}->getnext($field);
    do {
	($name,$vlan_number,$vlan_name) = @{$field};
	$self->debug("foundry::findVlans: $name $vlan_number $vlan_name\n",2);

	#
	# We only want the names - we ignore everything else
	#
	if ($name =~ /snVLanByPortCfgVLanName/) {
	    if (!@vlan_ids || exists $mapping{$vlan_name}) {
		$self->debug("Putting in mapping from $vlan_name to " .
		    "$vlan_number\n",2);
		$mapping{$vlan_name} = $vlan_number;
	    }
	}

	$self->{SESS}->getnext($field);
    } while ( $name =~ /^snVLanByPortCfgVLanName/) ;

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

    $self->debug("Entering device findVlan of $vlan_id\n",2);
    my $max_tries = $no_retry ? 1 : 5;
    my $vlan_number = undef;

 BLOCK: {
    $vlan_number = $self->{NEW_NAMES}->{$vlan_id};
    if (defined($vlan_number)) { last BLOCK; }

    #
    # We try this a few time, with 1 second sleeps, since it can take
    # a while for VLAN information to propagate
    #
    foreach my $try (1 .. $max_tries) {

	my %mapping = $self->findVlans($vlan_id);
	$vlan_number = $mapping{$vlan_id};
	if (defined($vlan_number)) { last BLOCK; }

	#
	# Wait before we try again
	#
	if ($try != $max_tries) {
	    $self->debug("VLAN find failed, trying again\n",2);
	    sleep 1;
	}
    }
 }
    $self->debug("Leaving device findVlan of $vlan_id \n",2);
    return $vlan_number;
}

#   
# Create a VLAN on this switch, with the given identifier (which comes from
# the database) and given 802.1Q tag number.
#
# usage: createVlan($self, $vlan_id, $vlan_number)
#        returns the new VLAN number on success
#        returns 0 on failure
#
# This routine is a bit of a lie for Foundry switches, because you
# can't create a VLAN without adding a member, so it is merely going to
# register the fact in the pending hashes, so that when the addPorts()
# call gets made, it will retrieve the appropriate tag number.
#
sub createVlan($$$) {
    my $self = shift;
    my $vlan_id = shift;
    my $vlan_number = shift;

    if (!defined($vlan_number)) {
	warn "foundry::createVlan called without supplying vlan_number";
	return 0;
    }
    $self->debug("foundry::createVlan id $vlan_id num $vlan_number\n",1);
    my $check_number = $self->{NEW_NAMES}->{$vlan_id};
    if (!defined($check_number))
	{ $check_number = $self->findVlan($vlan_id,1); }
    if (defined($check_number)) {
	if ($check_number != $vlan_number) {
		warn "attempingting to recreate vlan id $vlan_id which has ".
		     "existing vlan number $check_number with the new number ".
		     "$vlan_number\n";
		     return 0;
	}
     } else {
	$self->{NEW_NAMES}->{$vlan_id} = $vlan_number;
	$self->{NEW_NUMBERS}->{$vlan_number} = $vlan_id;
     }
     return $vlan_number;
}

#
# Put the given ports in the given VLAN. The VLAN is given as an 802.1Q 
# tag number.
#
# usage: setPortVlan($self, $vlan_number, @ports)
#	 returns 0 on sucess.
#	 returns the number of failed ports on failure.
#
sub setPortVlan($$@) {
    my $self = shift;
    my $vlan_number = shift;
    my @ports = @_;

    my $errors = 0;
    my %BumpedVlans = ();
           
    #
    # Run the port list through portmap to find the ports on the switch that
    # we are concerned with
    #

    my @portlist = $self->convertPortFormat($PORT_FORMAT_IFINDEX, @ports);
    $self->debug("foundry::setPortVLan ports: " . join(",",@ports) . "\n");
    $self->debug("as ifIndexes: " . join(",",@portlist) . "\n");

    foreach my $port (@portlist) {
	#
	# Do the acutal SNMP command
	#

	$self->debug("Putting port $port in VLAN $vlan_number\n");
	my $obj = "snVLanByPortMemberRowStatus.$vlan_number.$port";
	my $RetVal = $self->{SESS}->set( $obj, 4);

	if (!defined($RetVal) || !$RetVal) {
	    # might fail if the port is untagged and a member of another
	    # VLAN (e.g. Control, when setting up a firewalled experiment)
	    # so try to clear it out, and then try again.
	    my $portIndex = $self->{PORTINDEX}->{$port};
	    $RetVal = $self->{SESS}->get(["snSwPortInfoTagMode",$portIndex]);
	    $self->debug("TagMode for portIndex $portIndex is $RetVal\n");
	    if (defined($RetVal) && ($RetVal eq "untagged")) {
		$self->debug("2nd chance at $port\n");
		$RetVal = $self->{SESS}->get(["snSwPortVlanId",$portIndex]);
		if (defined($RetVal)) {
		    $BumpedVlans{$RetVal} = 1;
		    my $ok = $self->setVlansOnTrunk($port,0,$RetVal);
		    $self->debug("sVOT( $port, 0, $RetVal) returned $ok\n");
		    $RetVal = $self->{SESS}->set( $obj, 4);
		    if (defined ($RetVal) && $RetVal ) {
			    next;
		    }
		}
	    }
	    print STDERR "VLAN change for index $port failed\n";
	    $errors++;
	}
    }

    #
    # We need to make sure the ports get enabled.
    #

    $self->debug("Enabling "  . join(',',@ports) . "...\n");
    if ( my $rv = $self->portControl("enable",@ports) ) {
	print STDERR "Port enable had $rv failures.\n";
	$errors += $rv;
    }

    #
    # If this is a new vlan, need to set name and turn off STP.
    #
    $self->newNameNoStp($vlan_number, \$errors);

    #
    # When removing things from the control vlan for a firewall,
    # need to tell stack to shake things up to flush FDB on neighboring
    # switches.
    #
    my @bumpedlist = keys ( %BumpedVlans );
    if (@bumpedlist) {
	@{$self->{DISPLACED_VLANS}} = @bumpedlist;
    }

    return $errors;
}

#
# Set name and disable STP for a new vlan
#
sub newNameNoStp($$$)
{
    my ($self, $vlan_number, $eref) = @_;
    my $vlan_id = $self->{NEW_NUMBERS}->{$vlan_number};
    my $errors = 0;

    if (defined($vlan_id)) {
	print "  Creating VLAN $vlan_id as VLAN #$vlan_number on " .
		"$self->{NAME} ... ";
	my $obj = "snVLanByPortCfgVLanName";
 	my $RetVal = $self->{SESS}->
		set([$obj, $vlan_number, "$vlan_id", "OCTETSTR"]);
	if (!defined($RetVal) || !$RetVal) {
	    print STDERR "can't set name for vlan $vlan_number\n";
	    $$errors++;
	}
	$obj = "snVLanByPortCfgStpMode";
	$RetVal = $self->{SESS}->set( [$obj, $vlan_number,0,"INTEGER"]);
	if (!$RetVal) {
	    print STDERR "can't defeat STP on vlan $vlan_number\n";
	    $$errors++;
	}
	print "",($errors == 0 ? "Succeeded":"Failed"), ".\n";
	if ($errors == 0) {
	    delete $self->{NEW_NUMBERS}->{$vlan_number};
	    delete $self->{NEW_NAMES}->{$vlan_id};
	} else  { $$eref += $errors; }
    }
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

    foreach my $vlan_number (@vlan_numbers) {
	#
	# Do one to get the first field...
	#
	$self->debug("foundry::removePortsFromVlan number $vlan_number\n");
	my $field = ["snVLanByPortMemberVLanId",0];
	my ($name,$index,$value,$modport,$portIndex);
	do {
	    $self->{SESS}->getnext($field);
	    ($name,$index,$value) = @{$field};
	    $self->debug("removePortsFromVlan: Got $name $index $value\n",2);
	    if ($name eq "snVLanByPortMemberVLanId" &&
				($value == $vlan_number)) {
		#
		# This table is indexed by vlan.port
		#
		$index =~ /(\d+)\.(\d+)/;
		my ($vlan,$ifIndex) = ($1,$2);
		if ($vlan != $vlan_number)
		    { die "Something seriously hosed on $self->{NAME}\n"; }
		$portIndex = $self->{PORTINDEX}{$ifIndex};
		$modport = $self->{IFINDEX}{$ifIndex};
		#
		# disable this port, unless it is tagged.
		#
		$value = $self->{SESS}->get(["snSwPortInfoTagMode",$portIndex]);
		$self->debug("disabling $modport ( $portIndex ) "
				. "from vlan $vlan ( $value )\n" );
		if (defined($value) && ($value eq "untagged")) {
		    $value = $self->{SESS}->set
		    (["ifAdminStatus",$ifIndex,"down","INTEGER"]);
		}
	    }
	} while ( $name =~ /^snVLanByPortMemberVLanId/) ;
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
    my $DeleteOID = "snVLanByPortCfgRowStatus";
    my $name = $self->{NAME};



    foreach my $vlan_number (@vlan_numbers) {
	#
	# Perform the actual removal
	#
	my $RetVal = undef;
	print "  Removing VLAN # $vlan_number ... ";
	$RetVal = $self->{SESS}->set([[$DeleteOID,$vlan_number,"delete","INTEGER"]]);
	if ($RetVal) {
	    print "Removed VLAN $vlan_number on switch $name.\n";
	} else {
	    print STDERR "Removing VLAN $vlan_number failed on switch $name.\n";
	}
    }
    return ($errors == 0) ? 1 : 0;
}

#
# XXX: Major cleanup
#
sub UpdateField($$$@) {
    my $self = shift;
    my ($OID,$val,@ports)= @_;

    $self->debug("foundry::UpdateField: OID $OID value $val ports @ports\n");

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

    my $portset = $self->get1("snVLanByPortPortList",$vlan_number);
    if (defined($portset)) {
	my @ports = unpack "C*", $portset;
	$self->debug("foundry::vlanHasPorts got @ports \n");
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
    my $vlan_name;

    #
    # First, we grab the numbers of all the VLANs
    #
    my ($varname,$index,$value);
    my $field = ["snVLanByPortVLanId",0];
    $self->{SESS}->getnext($field);
    do {
	($varname,$index,$value) = @{$field};
	$self->debug("listVlans(numbers): Got $varname $index $value\n",2);
	if ($varname =~ /snVLanByPortVLanId/) {
	    $Numbers{$index} = $value;
	}
	$self->{SESS}->getnext($field);
    } while ( $varname eq "snVLanByPortVLanId");
    #
    # Then, we grab the names of all the VLANs
    #
    $field = ["snVLanByPortVLanName",0];
    $self->{SESS}->getnext($field);
    do {
	($varname,$index,$value) = @{$field};
	$self->debug("listVlans(names): Got $varname $index $value\n",2);
	if ($varname =~ /snVLanByPortVLanName/) {
	    $Names{$index} = $value;
	}
	$self->{SESS}->getnext($field);
	$vlan_name = $Names{$index};
    } while ( $varname eq "snVLanByPortVLanName");

    #
    # Next, we find membership, by port
    #
    $field = ["snVLanByPortPortList",0];
    $self->{SESS}->getnext($field);
    my $prefix = ($self->{NAME} || "port" );
    do {
	($varname,$index,$value) = @{$field};
	if ($varname =~ /snVLanByPortPortList/) {
	    my @portlist = unpack "C*", $value ;

	    my @memberlist = ();
	    $vlan_name = $Names{$index};
	    if (!defined($vlan_name)) { $vlan_name = "random" }
	    $self->debug("Vlan $vlan_name has portlist @portlist\n",2);

	    while ((scalar @portlist) != 0) {

	    # Find out the real node name. We just call it portX if
	    # it doesn't seem to have one.
	
		my $port = pop @portlist;
		my $slot = pop @portlist;
		my $portname = $prefix . ":" . $slot . "." . $port ;
		my $node = portnum($portname);
		if (defined($node)) {
		    $self->debug("portnum returns $node for  $portname\n",2);
		    push @{$Members{$index}}, $node;
		} else {
		    $self->debug("no portnum for $portname in $vlan_name\n");
		}
            }
	   #@memberlist = $Members{$index};
	   #$self->debug("constructed list @memberlist\n",2);
	}
	$self->{SESS}->getnext($field);
    } while ( $varname eq "snVLanByPortPortList" );

    #
    # Build a list from the name and membership lists
    #
    my @list = ();
    my $vlan_id;

    foreach $vlan_id ( sort keys %Numbers ) {
	my $number = $Numbers{$vlan_id};


	if (defined($number) && !($number ==  1)) {
		# Check for members to avoid pushing an undefined value into
		# the array (which causes warnings elsewhere)

		$self->debug("number $number for index $vlan_id \n",2);
	
		if ($Members{$vlan_id}) {
		    push @list, [$Names{$vlan_id},$number,$Members{$vlan_id}];
		} else {
		    push @list, [$Names{$vlan_id},$number,[]];
		}
	} else  { $self->debug("skipping index $vlan_id\n",2); }
    }

    #$self->debug("devListVlans:".join("\n",(map {join ",", @$_} @list))."\n");
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
    my ($varname, $modport, $ifIndex, $portIndex, $status);
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
	if ($status = $self->{SESS}->get(["snSwPortInfoChnMode",$portIndex])) {
	    $status =~ s/Duplex//;
	    $duplex{$ifIndex} = $status;
	}
	if ($status = $self->{SESS}->get(["snSwPortInfoSpeed",$portIndex])) {
	    $status =~ s/^s//;
	    $status =~ s/M$//;
	    $status =~ s/G/000/;
	    $status =~ s/Sense//;
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
	my $port = portnum($self->{NAME} . ":$modport");

	#
	# Skip ports that don't seem to have anything interesting attached
	#
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
sub resetVlanIfOnTrunk($$$) {
    my ($self, $modport, $vlan) = @_;
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    my $obj = "snVLanByPortMemberRowStatus.$vlan.$ifIndex";
    my $RetVal = $self->{SESS}->get($obj);

    if ($self->{DEBUG} > 0) {
	print $self->{NAME} . "::resetVlanIfOnTrunk got $RetVal for $obj\n";
    }
    if (defined($RetVal) && ($RetVal eq "valid")) {
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
    my $self = shift;
    my ($modport, $value, @vlan_numbers) = @_;
    my ($portindex,$ifIndex,$RetVal);
    my $errors = 0;

    #
    # Some error checking
    #
    if (($value != 1) && ($value != 0)) {
	die "Invalid value $value passed to setVlansOnTrunk\n";
    }
    if (grep(/^1$/,@vlan_numbers)) {
	die "VLAN 1 passed to setVlansOnTrunk\n";
    }
    $self->debug("foundry::setVlansOnTrunk" .
		"m $modport v $value nums @vlan_numbers\n");
    ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    $portindex = $self->{PORTINDEX}{$ifIndex};
    # Make sure port is tagged
    if ($value == 1) { 
	$RetVal = $self->{SESS}->set(
		[["snSwPortInfoTagMode",$portindex,"tagged","INTEGER"]]);
	if (!defined($RetVal) || !$RetVal) {
	    print STDERR "couldn't tag port $modport\n";
	    $errors++;
	}
    }
    foreach my $vlan_number (@vlan_numbers) {
	my $action = ($value == 1) ? "create" : "delete" ;
	$RetVal = $self->{SESS}->set(
		"snVLanByPortMemberRowStatus.$vlan_number.$ifIndex",$action);
	if (!$RetVal) {
		$errors++;
		print STDERR "couldn't add/remove port $modport" .
				"on vlan $vlan_number\n";
	}
	#
	# If this is a new vlan, need to set name and turn off STP.
	#
	$self->newNameNoStp($vlan_number, \$errors);
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
    my $self = shift;
    my ($modport) = @_;
    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$modport);
    my $portIndex = $self->{PORTINDEX}{$ifIndex};

    my $tag_obj = ["snSwPortInfoTagMode",$portIndex];
    my ($RetVal, $errors, $member_obj, @vlanlist);
    $errors = 0;
    $self->debug("foundry::clearAllVlansOnTurn modport $modport "
		    . " ifIndex $ifIndex portIndex $portIndex\n");
    my $tag_state  = $self->{SESS}->get( $tag_obj);
    $self->debug("tag state is $tag_state ");
    if (defined($tag_state) && ($tag_state eq "tagged") ) {
	$self->{SESS}->set(["snSwPortVlanId",$portIndex,0,"INTEGER"]);
	my %vlaninfo = $self->findVlans();
	@vlanlist = values %vlaninfo;
    } else {
	@vlanlist = ( $self->{SESS}->get([["snSwPortVlanId",$portIndex]]) );
    }
    foreach my $number (@vlanlist) {
	if ($number == 1) { next ; }
	$member_obj = "snVLanByPortMemberRowStatus.$number.$ifIndex";
	$RetVal = $self->{SESS}->get($member_obj);
	if (!defined($RetVal)) { next;}
	$self->debug("got $RetVal for $member_obj\n",1);
	if ($RetVal eq "valid") {
	    $self->debug("removing port $ifIndex from VLAN $number\n",1);
	    my $RetVal = $self->{SESS}->set( $member_obj, 3);

	    if (!defined($RetVal) || ! $RetVal ) {
		$errors++;
		print STDERR "Couldn't remove $modport from VLAN $number\n";
	    }
	}
    }
    return ($errors == 0) ;
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

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    my $portIndex = $self->{PORTINDEX}{$ifIndex};

    #
    # Clear out the list of allowed VLANs for this trunk port, so that when it
    # comes up, there is not some race condition
    #
    my $rv = $self->clearAllVlansOnTrunk($port);
    if (!$rv) {
	warn "ERROR: Unable to clear VLANs on trunk\n";
	return 0;
    } 

    #
    # Add this port to the VLAN as a tagged port
    #
    $rv = $self->setVlansOnTrunk($port, 1, ( $native_vlan) );
    if (!$rv) {
	warn "ERROR: Unable to add port $port to VLAN $native_vlan\n";
	return 0;
    } 

    #
    # Set the native VLAN for this trunk
    #
    my $nativeVlan = ["snSwPortVlanId",$portIndex,$native_vlan,"INTEGER"];
    $rv = snmpitSet($self->{SESS},$nativeVlan);
    if (!$rv) {
	warn "ERROR: Unable to set native VLAN on trunk\n";
	return 0;
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
    my $self = shift;
    my ($port) = @_;

    my ($ifIndex) = $self->convertPortFormat($PORT_FORMAT_IFINDEX,$port);
    my $portIndex = $self->{PORTINDEX}{$ifIndex};
    my $vlan_obj = ["snSwPortVlanId",$portIndex];
    my $native_vlan = snmpitGet($self->{SESS},$vlan_obj);

    #
    # Clear out the list of allowed VLANs for this trunk port
    #
    my $rv = $self->clearAllVlansOnTrunk($port);
    if (!$rv) {
	warn "ERROR: Unable to clear VLANs on trunk\n";
	return 0;
    } 
    $rv = $self->{SESS}->set(
	[["snSwPortInfoTagMode",$portIndex,"untagged","INTEGER"]]);
    if (defined($rv)) {
	$self->debug("foundry::disablePortTrunking TagMode set to $rv\n");
    }
    if ($native_vlan > 1) {
	$self->setPortVlan($native_vlan, $port);
    }
    return 1;
}

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
    $self->debug("readifIndex:\n",2);

    my $field = ["snIfIndexLookupInterfaceId",0];
    $self->{SESS}->getnext($field);
    do {
	($name,$ifindex,$iidoid) = @{$field};
	$self->debug("got $name, $ifindex, iidoid $iidoid ",2);
	if ($name =~ /snIfIndexLookupInterfaceId/) {
	    my @iid = split /\./, $iidoid ;
	    my $port = pop @iid;
	    my $mod = pop @iid;
	    my $modport = "$mod.$port";
	    my $portindex = $port + (256 * $mod);
	    $self->{IFINDEX}{$modport} = $ifindex;
	    $self->{IFINDEX}{$ifindex} = $modport;
	    $self->{PORTINDEX}{$modport} = $portindex;
	    $self->{PORTINDEX}{$ifindex} = $portindex;
	    $self->debug("mod $mod, port $port, ",2);
	    $self->debug("modport $modport, portindex $portindex",2);
	}
	$self->debug("\n",2);
	$self->{SESS}->getnext($field);
    } while ($name =~ /snIfIndexLookupInterfaceId/);
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
