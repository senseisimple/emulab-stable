<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");

#
# Note - this script is not meant to be called by humans! It returns no useful
# information whatsoever, and expects the client to fill in all fields
# properly.
# Since this script does not cause any action to actually happen, so it's save
# to leave 'in the open' - the worst someone can do is annoy the testbed admins
# with it!
#

#
# Find all interfaces
#
foreach ($HTTP_GET_VARS as $key => $value) {
    if (preg_match("/iface(name|mac)(\d+)/",$key,$matches)) {
        $vartype = $matches[1];
    	$ifacenum = $matches[2];
    	if ($vartype == "name") {
	    if (preg_match("/^([a-z]+)(\d+)$/i",$value,$matches)) {
		$interfaces[$ifacenum]["type"] = $matches[1];
	        $interfaces[$ifacenum]["iface"] = "eth$ifacenum";
	    } else {
		echo "Bad interface name $value!";
		continue;
	    }
	} else {
	    $interfaces[$ifacenum]["mac"] = $value;
	}
    }
}


#
# Attempt to come up with a node_id and an IP address for it
#
$node_num = find_free_id("pc");
$hostname = "pc$node_num";
$IP = guess_IP("pc",$node_num);

#
# Make an educated guess as to what type it belongs to
#
$type = guess_node_type($cpuspeed,$disksize);

#
# Stash this information in the database
#
DBQueryFatal("insert into new_nodes set node_id='$hostname', type='$type', " .
	"IP='$IP', dmesg='$messages', created=now()");

$query_result = DBQueryFatal("select last_insert_id()");
$row = mysql_fetch_array($query_result);
$new_node_id = $row[0];

foreach ($interfaces as $interface) {
	$iface = $interface["iface"];
	$mac = $interface["mac"];
	$type = $interface["type"];
	DBQueryFatal("insert into new_interfaces set new_node_id=$new_node_id, "
	    . "iface='$iface', mac='$mac', interface_type='$type'");
}

#
# Send mail to testbed-ops about the new node
#
TBMAIL($TBMAIL_OPS,"New Node","A new node, $hostname, has checked in");

function find_free_id($prefix) {

    #
    # Note, we lock the new_nodes table while we look to avoid races
    #
    DBQueryFatal("lock tables nodes read, new_nodes write");
    $node_number = 0;

    #
    # Just go through the nodes and new_nodes tables looking for one that
    # hasn't been used yet - put in a silly little guard to prevent an
    # infinite loop in case of bugs.
    #
    while ($node_number < 1000) {
	$node_number++;
    	$potential_name = $prefix . $node_number;
    	$query_result = DBQueryFatal("select node_id from nodes " .
		"where node_id='$potential_name'");
	if (mysql_num_rows($query_result)) {
		continue;
	}
    	$query_result = DBQueryFatal("select node_id from new_nodes " .
		"where node_id='$potential_name'");
	if (mysql_num_rows($query_result)) {
		continue;
	}
	break;
    }
    DBQueryFatal("unlock tables");

    return $node_number;

}

function guess_IP ($prefix, $number) {

    $hostname = $prefix . $number;

    #
    # First, let's see if they've already added to to DNS - the PHP
    # gethostbyname has a really dumb way to return failure - it just
    # gives you the hostname back.
    #
    $IP = gethostbyname($hostname);
    if (strcmp($IP,$hostname)) {
    	return $IP;
    }

    #
    # Okay, no such luck. We'll go backwards through the host list until
    # we find the previous node with an established IP, then add our offset
    # onto that
    #
    $i = $number - 1;
    $IP = "";
    while ($i > 0) {
        $query_result = DBQueryFatal("select IP from interfaces as i " .
		"left join nodes as n on i.node_id = n.node_id left join " .
		"node_types as nt on n.type = nt.type " .
		"where n.node_id='$prefix$i' and i.card = nt.control_net");
        if (mysql_num_rows($query_result)) {
	    $row = mysql_fetch_array($query_result);
	    $IP = $row[IP];
	    break;
	}
	$i--;
    }

    if ($i <= 0) {
    	return 0;
    }

    #
    # Parse the IP we got into the last octet and everything else. Note that we
    # don't really do the IP address calcuation correctly - just an
    # approximation of the right thing.
    #
    list($oct1,$oct2,$oct3,$oct4) = explode(".",$IP);
    $oct4 += $number - $i;

    # We don't yet wrap - it may not be OK to do
    if ($oct4 > 254) {
        return 0;
    }

    return "$oct1.$oct2.$oct3.$oct4";
    
}

function guess_node_type($proc,$disk) {

    #
    # Allow the reported speed to differ from the one in the database
    #
    $fudge_factor = .05;

    #
    # Convert disk size from megabytes to gigabtypes
    #
    $disk /= 1000.0;

    #
    # Go through node types and try to find a single one that matches
    # this node's processor speed. This is a totally bogus way to do this,
    # but it's the best we got for now.
    #
    $node_type = "";
    $query_result = DBQueryFatal("select type, speed, HD from node_types " .
	"where !isvirtnode and !isremotenode");
    while ($row = mysql_fetch_array($query_result)) {
        $speed = $row["speed"];
	$type = $row["type"];
	$HD = $row["HD"];
	echo "Checking $speed vs $proc, $HD vs $disk\n";
	if (($proc > ($speed * (1.0 - $fudge_factor))) &&
	    ($proc < ($speed * (1.0 + $fudge_factor))) &&
	    ($disk > ($HD * (1.0 - $fudge_factor))) &&
	    ($disk < ($HD * (1.0 + $fudge_factor)))) {
	    if ($node_type != "") {
	        # We found two potential matches, choose neither
		echo "Found a second match ($type), bailing\n";
		return "";
	    } else {
		echo "Found a first match ($type)\n";
	        $node_type = $type;
	    }
	}
    }

    echo "Returning $node_type\n";
    return $node_type;
}

?>
