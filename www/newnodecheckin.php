<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");
require("newnode-defs.php3");

#
# Note - this script is not meant to be called by humans! It returns no useful
# information whatsoever, and expects the client to fill in all fields
# properly.
# Since this script does not cause any action to actually happen, so it's save
# to leave 'in the open' - the worst someone can do is annoy the testbed admins
# with it!
#

#
# Grab the IP address that this node has right now, so that we can contact it
# later if we need to, say, reboot it.
#
$tmpIP = getenv("REMOTE_ADDR");

#
# Find all interfaces
#
$interfaces = array();
foreach ($HTTP_GET_VARS as $key => $value) {
    if (preg_match("/iface(name|mac)(\d+)/",$key,$matches)) {
        $vartype = $matches[1];
    	$ifacenum = $matches[2];
    	if ($vartype == "name") {
	    if (preg_match("/^([a-z]+)(\d+)$/i",$value,$matches)) {
		$interfaces[$ifacenum]["type"] = $matches[1];
	        $interfaces[$ifacenum]["card"] = $ifacenum;
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
# Use one of the interfaces to see if this node seems to have already checked
# in once
#
if (count($interfaces)) {
    $testmac = $interfaces[0]["mac"];

    #
    # First, make sure it isn't a 'real boy' - we should let the operators know
    # about this, because there may be some problem.
    #
    $query_result = DBQueryFatal("select n.node_id from " .
	"nodes as n left join interfaces as i " .
	"on n.node_id=i.node_id " .
	"where i.mac='$testmac'");
    if  (mysql_num_rows($query_result)) {
        $row = mysql_fetch_array($query_result);
	$node_id = $row["node_id"];
        echo "Node is already a real node, named $node_id\n";
	TBMAIL($TBMAIL_OPS,"Node Checkin Error","A node attempted to check " .
	    "in as a new node, but it is already\n in the database as " .
	    "$node_id!");
	exit;
    }


    #
    # Next, try the new nodes
    #
    $query_result = DBQueryFatal("select n.new_node_id, n.node_id from " .
	"new_nodes as n left join new_interfaces as i " .
	"on n.new_node_id=i.new_node_id " .
	"where i.mac='$testmac'");

    if  (mysql_num_rows($query_result)) {
        $row = mysql_fetch_array($query_result);
	$id = $row["new_node_id"];
	$node_id = $row["node_id"];
        echo "Node has already checked in as ID $id, name $node_id\n";

	#
	# Keep the temp. IP address around in case it's gotten a new one
	#
	DBQueryFatal("update new_nodes set temporary_IP='$tmpIP' " .
	    "where new_node_id=$id");

	exit;
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
	"IP='$IP', temporary_IP='$tmpIP', dmesg='$messages', created=now()");

$query_result = DBQueryFatal("select last_insert_id()");
$row = mysql_fetch_array($query_result);
$new_node_id = $row[0];

foreach ($interfaces as $interface) {
	$card = $interface["card"];
	$mac = $interface["mac"];
	$type = $interface["type"];
	DBQueryFatal("insert into new_interfaces set " .
	    "new_node_id=$new_node_id, card=$card, mac='$mac', " .
	    "interface_type='$type'");
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

?>
