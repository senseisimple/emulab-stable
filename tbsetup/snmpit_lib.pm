#!/usr/local/bin/perl -w
#
# Module of subroutines useful to snmpit and its modules
#

package snmpit_lib;

use Exporter;
@ISA = ("Exporter");
@EXPORT = qw( macport portnum Dev vlanmemb vlanid
		getTestSwitches getVlanPorts getExperimentVlans getDeviceNames
	    	getDeviceType mapPortsToDevices getSwitchStack tbsort );

use English;
use libdb;
use strict;

my $debug = 0;

my %Devices=();
# Devices maps device names to device IPs

my %Interfaces=();
# Interfaces maps pcX:Y<==>MAC

my %Ports=();
# Ports maps pcX:Y<==>switch:port

my %vlanmembers=();
# vlanmembers maps id -> members

my %vlanids=();
# vlanids maps pid:eid <==> id

#
# Initialize the 
#
sub init($) {
    $debug = shift || $debug;
    &ReadTranslationTable;
    return 0;
}

#
# Map between interfaces and mac addresses
#
sub macport {
    my $val = shift || "";
    return $Interfaces{$val};
}

#
# Map between interfaces and port numbers
#
sub portnum {
    my $val = shift || "";
    return $Ports{$val};
}

#
# Map between interfaces and the devices they are attached to
#
sub Dev {
    my $val = shift || "";
    return $Devices{$val};
}

#
# This function fills in %Interfaces and %Ports
# They hold pcX:Y<==>MAC and pcX:Y<==>switch:port respectively
#
sub ReadTranslationTable {
    my $name="";
    my $mac="";
    my $switchport="";

    print "FILLING %Interfaces\n" if $debug;
    my $result = DBQueryFatal("select * from interfaces;");
    while ( @_ = $result->fetchrow_array()) {
	$name = "$_[0]:$_[1]";
	if ($_[2] != 1) {$name .=$_[2]; }
	$mac = "$_[3]";
	if ($name =~ /(sh\d+)(-\d)(:\d)?/ ) { $name = "$1$3"; }
	$Interfaces{$name} = $mac;
	$Interfaces{$mac} = $name;
	print "Interfaces: $mac <==> $name\n" if $debug > 1;
    }

    print "FILLING %Devices\n" if $debug;
    $result = DBQueryFatal("select i.node_id,i.IP,n.type from interfaces as i ".
	    "left join nodes as n on n.node_id=i.node_id ".
	    "left join node_types as nt on n.type=nt.type ".
	    "where n.role!='testnode' and i.card=nt.control_net");
    while ( my ($name,$ip,$type) = $result->fetchrow_array()) {
	$Devices{$name} = $ip;
	$Devices{$ip} = $name;
	print "Devices: $name ($type) <==> $ip\n" if $debug > 1;
    }

    print "FILLING %Ports\n" if $debug;
    $result = DBQueryFatal("select node_id1,card1,port1,node_id2,card2,port2 ".
	    "from wires;");
    while ( @_ = $result->fetchrow_array()) {
	$name = "$_[0]:$_[1]";
	print "Name='$name'\t" if $debug > 2;
	print "Dev='$_[3]'\t" if $debug > 2;
	$switchport = join(":",($_[3],$_[4]));
	$switchport .=".$_[5]";
	print "switchport='$switchport'\n" if $debug > 2;
	$Ports{$name} = $switchport;
	$Ports{$switchport} = $name;
	print "Ports: '$name' <==> '$switchport'\n" if $debug > 1;
    }

}

#
# Returns an array of ports (in node:card form) used by the given VLANs
#
sub getVlanPorts (@) {
    my @vlans = @_;
    # Silently exit if they passed us no VLANs
    if (!@vlans) {
	return ();
    }

    my $result = DBQueryFatal("SELECT members FROM vlans WHERE " .
	join(' OR ', map("id='$_'",@vlans))); # Join "id='foo'" with ORs
    my @ports;
    while (my @row = $result->fetchrow()) {
	my $members = $row[0];
	# $members is a space-seprated list
	foreach my $port (split /\s+/,$members) {
	    # Due to the inconsistent nature of our tables (curses!), we
	    # have to do some conversion here
	    $port =~ /^(.+):(.+)/;
	    my ($node,$iface) = ($1,$2);
	    if (!defined($node) || !defined($iface)) {
		warn "WARNING: Bad node in VLAN: $port - skipping\n";
		next;
	    }
	    my $result = DBQueryFatal("SELECT card FROM interfaces " .
	    	"WHERE node_id='$node' AND iface='$iface'");
	    if (!$result->num_rows()) {
		warn "WARNING: Bad node/iface pair in VLAN: $port - skipping\n";
		next;
	    }

	    my $card = ($result->fetchrow())[0];

	    # OK, finally have the info we need
	    push @ports, $node . ":" . $card;
	}
    }
    return @ports;
}

#
# Returns an array of all VLAN id's used by a given experiment
#
sub getExperimentVlans ($$) {
    my ($pid, $eid) = @_;

    my $result =
	DBQueryFatal("SELECT id FROM vlans WHERE pid='$pid' AND eid='$eid'");
    my @vlans = (); 
    while (my @row = $result->fetchrow()) {
	push @vlans, $row[0];
    }

    return @vlans;
}

#
# Usage: getDeviceNames(@ports)
#
# Returns an array of the names of all devices used in the given ports
#
sub getDeviceNames(@) {
    my @ports = @_;
    my %devices = ();
    print "getDeviceNames: Passed in " . join(",",@ports) . "\n" if $debug;
    foreach my $port (@ports) {
	$port =~ /^(.+):(.+)$/;
	my ($node,$card) = ($1,$2);
	if (!defined($node) || !defined($card)) { # Oops, $card can be 0
	    die "Bad port given: $port\n";
	}
	my $result = DBQueryFatal("SELECT node_id2 FROM wires " .
	    "WHERE node_id1='$node' AND card1=$card");
	if (!$result->num_rows()) {
	    warn "No database entry found for port $port - Skipping\n";
	    next;
	}
	# This is a loop, on the off chance chance that a single port on a
	# node can be connected to multiple ports on the switch.
	while (my @row = $result->fetchrow()) {
	    my $device = $row[0];
	    $devices{$device} = 1;
	}
    }
    return (sort {tbsort($a,$b)} keys %devices);
}

#
# Returns a hash, keyed by device, of all ports in the given list that are
# on that device
#
sub mapPortsToDevices(@) {
    my @ports = @_;
    my %map = ();
    foreach my $port (@ports) {
	my ($device) = getDeviceNames($port);
	if (defined($device)) { # getDeviceNames does the job of warning users
	    push @{$map{$device}},$port;
	}
    }
    return %map;
}

#
# Returns the device type for the given node_id
#
sub getDeviceType ($) {

    my ($node) = @_;

    my $result =
	DBQueryFatal("SELECT type FROM nodes WHERE node_id='$node'");

    my @row = $result->fetchrow();
    # Sanity check - make sure the node exists
    if (!@row) {
	die "No such node: $node\n";
    }

    return $row[0];
}

#
# Returns an array with then names of all switches identified as test switches
#
sub getTestSwitches () {
    my $result =
	DBQueryFatal("SELECT node_id FROM nodes WHERE role='testswitch'");
    my @switches = (); 
    while (my @row = $result->fetchrow()) {
	push @switches, $row[0];
    }

    # Sanity check - make sure we actually found some
    if (!@switches) {
	die "Failed to find testswitches\n";
    }

    return @switches;
}

#
# Returns the stack_id that a switch belongs to
#
sub getSwitchStack($) {
    my $switch = shift;
    my $result = DBQueryFatal("SELECT stack_id FROM switch_stacks WHERE " .
    		"node_id='$switch'");
    if (!$result->numrows()) {
	print STDERR "No stack_id found for switch $switch\n";
	return undef;
    } else {
	my ($stack_id) = ($result->fetchrow());
	return $stack_id;
    }
}

#
# Used to sort a set of nodes in testbed order (ie. pc2 < pc10)
#
# usage: tbsort($a,$b)
#        returns -1 if $a < $b
#        returns  0 if $a == $b
#        returns  1 if $a > $b
#
sub tbsort { 
    my ($a,$b) = @_;
    $a =~ /^([a-z]*)([0-9]*):?([0-9]*)/;
    my $a_let = ($1 || "");
    my $a_num = ($2 || 0);
    my $a_num2 = ($3 || 0);
    $b =~ /^([a-z]*)([0-9]*):?([0-9]*)/;
    my $b_let = ($1 || "");
    my $b_num = ($2 || 0);
    my $b_num2 = ($3 || 0);
    if ($a_let eq $b_let) {
	if ($a_num == $b_num) {
	    return $a_num2 <=> $b_num2;
	} else {
	    return $a_num <=> $b_num;
	}
    } else {
	return $a_let cmp $b_let;
    }
    return 0;
}
# End with true
1;

