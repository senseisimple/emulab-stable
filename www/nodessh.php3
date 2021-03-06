<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# This script generates an "tbc" file, to be passed to ./ssh-mime.pl
# on the remote node, when set up as a proper mime type.
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify form arguments.
#
$reqargs = RequiredPageArguments("node", PAGEARG_NODE);

# Need these below
$node_id = $node->node_id();

$query_result =
    DBQueryFatal("select n.jailflag,n.jailip,n.sshdport, ".
		 "       r.vname,r.pid,r.eid, ".
		 "       t.isvirtnode,t.isremotenode,t.isplabdslice, ".
		 "       t.issubnode,t.isfednode,t.class ".
		 " from nodes as n ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "left join node_types as t on t.type=n.type ".
		 "left join os_info as oi on n.def_boot_osid=oi.osid ".
		 "where n.node_id='$node_id'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("The node $node_id does not exist!", 1);
}

$row = mysql_fetch_array($query_result);
$jailflag = $row["jailflag"];
$jailip   = $row["jailip"];
$sshdport = $row["sshdport"];
$vname    = $row["vname"];
$pid      = $row["pid"];
$eid      = $row["eid"];
$isvirt   = $row["isvirtnode"];
$isremote = $row["isremotenode"];
$isplab   = $row["isplabdslice"];
$issubnode= $row["issubnode"];
$class    = $row["class"];
$isfednode= $row["isfednode"];

#
# XXX hack to determine if target node is on a routable network
#
$unroutable = ($ELABINELAB || !strncmp($CONTROL_NETWORK, "192.168.", 8));

#
# If we need a proxy host, determine whether it is ops or boss.
# Normally it is ops, unless ops is a VM and it is in an inner elab.
# In that case, there is no external DNS alias ("myops.eid.pid.emulab.net")
# created for the inner ops so it cannot be the proxy.
#
if ($ELABINELAB && $OPS_VM) {
    $PROXYNODE = $BOSSNODE;
} else {
    $PROXYNODE = $USERNODE;
}

if (!isset($pid)) {
    USERERROR("$node_id is not allocated to an experiment!", 1);
}

$filename = $node_id . ".tbssh"; 
header("Content-Type: text/x-testbed-ssh");
header("Content-Disposition: inline; filename=$filename;");
header("Content-Description: SSH description file for a testbed node");

if ($NONAMEDSETUP) {
    echo "hostname: $node_id.$OURDOMAIN\n";
} else {
    echo "hostname: $vname.$eid.$pid.$OURDOMAIN\n";
}
echo "login:    $uid\n";

if ($isvirt) {
    if ($isremote) {
	#
	# Remote nodes run sshd on another port since they so not
	# have per-jail IPs. Of course, might not even be jailed!
	#
	if ($jailflag || $isplab || $isfednode) {
	    echo "port: $sshdport\n";
	}
    }
    else {
	#
	# Local virt nodes are on the private network, so have to
	# bounce through ops node to get there. They run sshd on
	# on the standard port, but on a private IP.
	#
	echo "gateway: $PROXYNODE\n";
    }
}
elseif ($unroutable) {
    #
    # If nodes are unroutable, gateway via the user node
    #
    echo "gateway: $PROXYNODE\n";
}
elseif ($issubnode && $class == 'ixp') {
    #
    # IXP hack: pass <node-id>-gw as the gateway address
    #
    echo "gateway: $node_id-gw.$OURDOMAIN\n";
}

?>
