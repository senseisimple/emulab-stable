<?php
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
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
        USERERROR("You do not have permission to tip to node $node_id!", 1);
    }
}

$query_result = DBQueryFatal("SELECT server, portnum, keylen, keydata FROM tiplines WHERE node_id='$node_id'" );

if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id does not exist, or appear to have a tipline!", 1);
}

$filename = $node_id . ".acl"; 

header("Content-Type: text/testbed-acl");
header("Content-Disposition: attachment; filename=$filename;");
header("Content-Description: an ACL file which will allow access to a testbed node serial port");

# XXX, should handle multiple tip lines gracefully somehow, but not important for now.

$row = mysql_fetch_array($query_result);
$server  = $row[server];
$portnum = $row[portnum];
$keylen  = $row[keylen];
$keydata = $row[keydata];

# XXX fix me!!!
$certhash = "fc3749f14c074589342cd1fef8bb33169e7b3724";

echo "host:   $server\n";	
echo "port:   $portnum\n";
echo "keylen: $keylen\n";
echo "key:    $keydata\n";
echo "ssl-server-cert: $certhash\n";

#
# Spit out acl with a content header.
#

#if ($fp = popen("$TBSUEXEC_PATH $uid $gid webvistopology $detailstring -z $zoom $pid $eid", "r")) {
#    header("Content-type: text/testbed-acl");
#    fpassthru($fp);
#}

#
# No Footer!
# 
?>