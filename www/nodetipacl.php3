<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("xmlrpc.php3");

#
# This script generates an "acl" file.
#

#
# Only known and logged in users can get acls..
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

#
# Admin users can look at any node, but normal users can only control
# nodes in their own experiments.
#
# XXX is MODIFYINFO the correct one to check? (probably)
#
if (! $isadmin) {
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_READINFO)) {
        USERERROR("You do not have permission to tip to node $node_id!", 1);
    }
}

#
# Ask outer emulab for the stuff we need. It does it own perm checks
#
if ($ELABINELAB) {
    $arghash = array();
    $arghash["node"] = $node_id;

    $results = XMLRPC($uid, "nobody", "elabinelab.console", $arghash);

    if (!$results ||
	! (isset($results{'server'})  && isset($results{'portnum'}) &&
	   isset($results{'keydata'}) && isset($results{'certsha'}))) {
	TBERROR("Did not get everything we needed from RPC call", 1);
    }

    $server  = $results['server'];
    $portnum = $results['portnum'];
    $keydata = $results['keydata'];
    $keylen  = strlen($keydata);
    $certhash= strtolower($results{'certsha'});
}
else {

    $query_result = DBQueryFatal("SELECT server, portnum, keylen, keydata " . 
				 "FROM tiplines WHERE node_id='$node_id'" );

    if (mysql_num_rows($query_result) == 0) {
	USERERROR("The node $node_id does not exist, ".
		  "or does not have a tipline!", 1);
    }
    $row = mysql_fetch_array($query_result);
    $server  = $row[server];
    $portnum = $row[portnum];
    $keylen  = $row[keylen];
    $keydata = $row[keydata];

    #
    # Read in the fingerprint of the capture certificate
    #
    $capfile = "$TBETC_DIR/capture.fingerprint";
    $lines = file($capfile,"r");
    if (!$lines) {
	TBERROR("Unable to open $capfile!",1);
    }

    $fingerline = rtrim($lines[0]);
    if (!preg_match("/Fingerprint=([\w:]+)$/",$fingerline,$matches)) {
	TBERROR("Unable to find fingerprint in string $fingerline!",1);
    }
    $certhash = str_replace(":","",strtolower($matches[1]));
}

$filename = $node_id . ".tbacl"; 

header("Content-Type: text/x-testbed-acl");
header("Content-Disposition: inline; filename=$filename;");
header("Content-Description: ACL key file for a testbed node serial port");

# XXX, should handle multiple tip lines gracefully somehow, 
# but not important for now.

echo "host:   $server\n";	
echo "port:   $portnum\n";
echo "keylen: $keylen\n";
echo "key:    $keydata\n";
echo "ssl-server-cert: $certhash\n";
?>

