<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003, 2005, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
require("defs.php3");
require("newnode-defs.php3");
include("xmlrpc.php3");

#
# Note - this script is not meant to be called by humans! It returns no useful
# information whatsoever, and expects the client to fill in all fields
# properly.
# Since this script does not cause any action to actually happen, so its save
# to leave 'in the open' - the worst someone can do is annoy the testbed admins
# with it!
#

$reqargs = RequiredPageArguments("cpuspeed",    PAGEARG_STRING,
				 "diskdev",     PAGEARG_STRING,
				 "disksize",    PAGEARG_STRING,
				 "role",        PAGEARG_STRING,
				 "messages",    PAGEARG_STRING);

$optargs = OptionalPageArguments("node_id",     PAGEARG_STRING,
				 "identifier",  PAGEARG_STRING,
				 "use_temp_IP", PAGEARG_STRING,
				 "type",        PAGEARG_STRING);

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
    # First, make sure it is not a 'real boy' - we should let the operators 
    # know about this, because there may be some problem.
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
        echo "Node has already checked in\n";
	echo "Node ID is $id\n";

	#
	# Keep the temp. IP address around in case its gotten a new one
	#
	DBQueryFatal("update new_nodes set temporary_IP='$tmpIP' " .
	    "where new_node_id=$id");

	exit;
    }
}


#
# Attempt to come up with a node_id and an IP address for it - unless one was
# provided by the client.
#
if (!isset($node_id)) {
    $name_info = find_free_id("pc");
    $node_prefix = $name_info[0];
    $node_num = $name_info[1];
    $hostname = $node_prefix . $node_num;
} else {
    $hostname = $node_id;
}

if (isset($use_temp_IP)) {
    $IP = $tmpIP;
} else {
    $IP = guess_IP($node_prefix,$node_num);
}

#
# Handle the node type
#
if (isset($type) && $type != "") {
    #
    # If they gave us a type, lets see if that type exists or not
    #
    if (!preg_match("/^[-\w]*$/", $type)) {
	echo "Illegal characters in type: '$type'\n";
	exit;
    }
    if (TBValidNodeType($type)) {
	#
	# Great, it already exists, nothin' else to do
	#
    } else {
	#
	# Okay, it doesn't exist. We'll create it.
	#
	make_node_type($type,$cpuspeed,$disksize);
    }
} else {
    #
    # Make an educated guess as to what type it belongs to
    #
    $type = guess_node_type($cpuspeed,$disksize);
}

#
# Default the role to 'testnode' if the node didn't supply a role
#
$role = (isset($role) ? addslashes($role) : "testnode");

#
# Stash this information in the database
#
if (isset($identifier) && $identifier != "") {
    $identifier = "'" . addslashes($identifier) . "'";
} else {
    $identifier = "NULL";
}
$messages = (isset($messages) ? addslashes($messages) : "");
$hostname = (isset($hostname) ? addslashes($hostname) : "");

DBQueryFatal("insert into new_nodes set node_id='$hostname', type='$type', " .
	"IP='$IP', temporary_IP='$tmpIP', dmesg='$messages', created=now(), " .
	"identifier=$identifier, role='$role'");

$query_result = DBQueryFatal("select last_insert_id()");
$row = mysql_fetch_array($query_result);
$new_node_id = $row[0];

echo "Node ID is $new_node_id\n";

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

function check_node_exists($node_id) {
    $node_id = addslashes($node_id);
    
    #
    # Just check to see if this node already exists in one of the
    # two tables - return 1 if it does, 0 if not
    #
    $query_result = DBQueryFatal("select node_id from nodes " .
	    "where node_id='$node_id'");
    if (mysql_num_rows($query_result)) {
	return 1;
    }
    $query_result = DBQueryFatal("select node_id from new_nodes " .
	    "where node_id='$node_id'");
    if (mysql_num_rows($query_result)) {
	return 1;
    }

    return 0;
}

function find_free_id($prefix) {
    global $ELABINELAB, $TBADMINGROUP, $interfaces;

    #
    # When inside an inner emulab, we have to ask the outer emulab for
    # our nodeid; we cannot just pick one out of a hat, at least not yet.
    #
    if ($ELABINELAB) {
	$arghash = array();
	$arghash["mac"] = $interfaces[0]["mac"];

	$results = XMLRPC("nobody", $TBADMINGROUP,
			  "elabinelab.newnode_info", $arghash);

	if (!$results || ! isset($results{'nodeid'})) {
	    echo "Could not get nodeid from XMLRPC server; quitting.\n";
	    exit;
	}
	elseif (preg_match("/^(.*[^\d])(\d+)$/",
			   $results{'nodeid'}, $matches)) {
	    $base   = $matches[1];
	    $number = $matches[2];
	    return array($base, intval($number));
	}
	else {
	    $nodeid = $results{'nodeid'};
	    
	    echo "Improper nodeid ($nodeid) from XMLRPC server; quitting.\n";
	    exit;
	}
    }

    #
    # First, check to see if there's a recent entry in new_nodes we can name
    # this node after
    #
    $query_result = DBQueryFatal("select node_id from new_nodes " .
        "order by created desc limit 1");
    if (mysql_num_rows($query_result)) {
        $row = mysql_fetch_array($query_result);
	$old_node_id = $row[0];
	#
	# Try to figure out if this is in some format we can increment
	#
	if (preg_match("/^(.*[^\d])(\d+)$/",$old_node_id,$matches)) {
	    echo "Matches pcXXX format";
	    # pcXXX format
	    $base = $matches[1];
	    $number = $matches[2];
	    $potential_name = $base . ($number + 1);
	    if (!check_node_exists($potential_name)) {
		return array($base,($number +1));
	    }
	} elseif (preg_match("/^(.*)-([a-zA-Z])$/",$old_node_id,$matches)) {
	    # Something like WAIL's (type-rack-A) format
	    $base = $matches[1];
	    $lastchar = $matches[2];
	    $newchar = chr(ord($lastchar) + 1);
	    $potential_name = $base . '-' . $newchar;
	    if (!check_node_exists($potential_name)) {
		return array($base . '-', $newchar);
	    }
	}
    }

    #
    # Okay, that didn't work.
    # Just go through the nodes and new_nodes tables looking for one that
    # hasn't been used yet - put in a silly little guard to prevent an
    # infinite loop in case of bugs.
    #
    $node_number = 0;
    while ($node_number < 10000) {
	$node_number++;
    	$potential_name = $prefix . $node_number;
	if (!check_node_exists($potential_name)) {
	    break;
	}
    }

    return array($prefix, $node_number);

}

?>
