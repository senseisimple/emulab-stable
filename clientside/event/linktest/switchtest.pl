#!/usr/bin/perl
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2010 University of Utah and the Flux Group.
# All rights reserved.
#

use lib "/usr/testbed/lib";
use libdb;

if (scalar(@ARGV) != 2
    || $ARGV[0] !~ /[a-zA-Z0-9]+/
    || $ARGV[1] !~ /[a-zA-Z0-9]+/) {
    print "USAGE: switchtest <proj> <exp>\n";
    exit(1);
}

$proj = $ARGV[0];
$exp = $ARGV[1];

%valid = ();
%should_enable = ();

$foundNodes = 0;

# Get the set of node, iface values which should be enabled in an exp
$result = DBQueryFatal("select m.node_id,a.attrvalue from lans as l left join lan_members as m on l.lanid=m.lanid left join lan_member_attributes as a on l.lanid=a.lanid and m.memberid = a.memberid where a.attrkey='iface' and l.pid='$proj' and l.eid='$exp'");
while (($node, $iface) = $result->fetchrow_array) {
    $valid{"$node:$iface"} = 1;
}

# Get the set of node, card, iface, roles associated with all nodes in an exp
$result = DBQueryFatal("select m.node_id,i.card,i.iface,i.role from lans as l left join lan_members as m on l.lanid=m.lanid left join interfaces as i on m.node_id=i.node_id where l.pid='$proj' and l.eid='$exp'");
while (($node, $card, $iface, $role) =$result->fetchrow_array) {
    $foundNodes = 1;
    $should = 0;
    if (exists($valid{"$node:$iface"}) || $role eq "ctrl") {
	$should = 1;
    }
    $should_enable{"$node:$card"} = $should;
}

# Command line to get all port info (enabled yes/no, up/down status)
$portinfo = `/usr/testbed/sbin/wap /usr/testbed/bin/snmpit -s | tail -n +2`;
@portlist = split(/\n/, $portinfo);
foreach $port (@portlist) {
    if ($port =~ /^([a-zA-Z:0-9]+)\s+([a-z]+)\s+([a-z]+)/) {
	$name = $1;
	$enabledText = $2;
	$upText = $3;
	$enabled = ($enabledText eq "yes");
	$up = ($upText eq "up");
#	if ($enabled && ! $up) {
#	    print STDERR "WARNING: $name is enabled but not up\n";
#	}
	if (exists($should_enable{$name})) {
	    if ($should_enable{$name}) {
		if (! $enabled || ! $up) {
		    print STDERR "ERROR: $name should be enabled. ".
			"Switch says enabled=$enabledText, status=$upText\n";
		}
	    } else {
		if ($enabled || $up) {
		    print STDERR "ERROR: $name should be disabled. ".
			"Switch says enabled=$enabledText, status=$upText\n";
		}
	    }
	}
    }
}

if (! $foundNodes) {
    print STDERR "WARNING: No active nodes found in experiment $exp ".
	"in project $proj\n";
}
