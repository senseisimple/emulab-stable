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
# Try to figure out which switch ports it's plugged into
#
$mac_list = array();
foreach ($interfaces as $interface) {
	$mac_list[$interface["type"]] = "";
}
find_switch_macs($mac_list);

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
	if ($mac_list[$mac]["switch"]) {
	    DBQueryFatal("insert into new_interfaces set " .
		"new_node_id=$new_node_id, iface='$iface', mac='$mac', " .
		"interface_type='$type', ".
		"switch_id='$mac_list[$mac][switch]', " .
		"switch_card='$mac_list[$mac][card]', " .
		"switch_port='$mac_list[$mac][port]'");
	} else {
	    DBQueryFatal("insert into new_interfaces set " .
		"new_node_id=$new_node_id, iface='$iface', mac='$mac', " .
		"interface_type='$type'");
	}
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
