<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates an "acl" file.
#

#
# Only known and logged in users can get acls..
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_READINFO)) {
        USERERROR("You do not have permission to tip to node $node_id!", 1);
    }
}

$query_result = DBQueryFatal("SELECT server, portnum, keylen, keydata " . 
			     "FROM tiplines WHERE node_id='$node_id'" );

if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id does not exist, or seem to have a tipline!", 1);
}

$filename = $node_id . ".tbacl"; 

header("Content-Type: text/x-testbed-acl");
header("Content-Disposition: attachment; filename=$filename;");
header("Content-Description: ACL key file for a testbed node serial port");

# XXX, should handle multiple tip lines gracefully somehow, 
# but not important for now.

$row = mysql_fetch_array($query_result);
$server  = $row[server];
$portnum = $row[portnum];
$keylen  = $row[keylen];
$keydata = $row[keydata];

# XXX fix me!!!
# $certhash = "7161bb44818e7be5a5bcd58506163e1583e6aa1c";
$certhash = "0bc864551de711a3d46ac173dbd67cde75c36734";

echo "host:   $server\n";	
echo "port:   $portnum\n";
echo "keylen: $keylen\n";
echo "key:    $keydata\n";
echo "ssl-server-cert: $certhash\n";
?>

