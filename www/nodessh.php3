<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates an "tbc" file, to be passed to ./ssh-mime.pl
# on the remote node, when set up as a proper mime type.
#

#
# Only known and logged in users.
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

$query_result =
    DBQueryFatal("select n.jailflag,n.jailip,n.sshdport, ".
		 "       r.vname,r.pid,r.eid, ".
		 "       t.isvirtnode,t.isremotenode ".
		 " from nodes as n ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "left join node_types as t on t.type=n.type ".
		 "where n.node_id='$node_id'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("The node $node_id does not exist!", 1);
}

$row = mysql_fetch_array($query_result);
$jailflag = $row[jailflag];
$jailip   = $row[jailip];
$sshdport = $row[sshdport];
$vname    = $row[vname];
$pid      = $row[pid];
$eid      = $row[eid];
$isvirt   = $row[isvirtnode];
$isremote = $row[isremotenode];

if (!isset($pid)) {
    USERERROR("$node_id is not allocated to an experiment!", 1);
}

$filename = $node_id . ".tbc"; 
header("Content-Type: text/x-testbed-ssh");
header("Content-Disposition: attachment; filename=$filename;");
header("Content-Description: SSH description file for a testbed node");

echo "hostname: $vname.$eid.$pid.$OURDOMAIN\n";
echo "login:    $uid\n";

if ($isvirt) {
    if ($isremote) {
	#
	# Remote nodes run sshd on another port since they so not
	# have per-jail IPs. Of course, might not even be jailed!
	#
	if ($jailflag) {
	    echo "port: $sshdport\n";
	}
    }
    else {
	#
	# Local virt nodes are on the private network, so have to
	# bounce through ops node to get there. They run sshd on
	# on the standard port, but on a private IP.
	#
	echo "gateway: $USERNODE\n";
    }
}

?>

