<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "node", PAGEARG_NODE,
				 "canceled", PAGEARG_BOOLEAN,
				 "confirmed", PAGEARG_BOOLEAN);

if (isset($experiment)) {
    if (! $experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to reboot nodes in this ".
		  "experiment", 1);
    }
    $pid = $experiment->pid();
    $eid = $experiment->eid();
}
elseif (isset($node)) {
    if (! $node->AccessCheck($this_user, $TB_NODEACCESS_REBOOT)) {
        USERERROR("You do not have permission to reboot this node", 1);
    }
    $node_id = $node->node_id();
}
else {
    USERERROR("Must specify a node or an experiment!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    PAGEHEADER("Reboot Nodes");
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    PAGEHEADER("Reboot Nodes");

    if (isset($node)) {
	echo "<center><h2><br>
              Are you <b>REALLY</b>
                sure you want to reboot node '$node_id?'
              </h2>\n";
	
	$node->Show(SHOWNODE_SHORT);
    }
    else {
	echo $experiment->PageHeader();

	echo "<center><font size=+2><br>
              Are you <b>REALLY</b>
                sure you want to reboot all nodes?
              </font>\n";

	$experiment->Show(1);
    }

    $url = CreateURL("boot", ((isset($node) ? $node : $experiment)));
    
    echo "<form action='$url' target=_blank method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if (isset($node)) {
    $message = "$node_id was rebooted via the web interface by $uid\n";
    $subject = "Node Reboot: $node_id";
}
else {
    $message = "Nodes in $pid/$eid were rebooted via the web interface ".
	       "by $uid\n";
    $subject = "Nodes Rebooted: $pid/$eid";
}

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (connection_aborted() && $fp) {
	pclose($fp);
    }
    exit();
}
register_shutdown_function("SPEWCLEANUP");
ignore_user_abort(1);

if (isset($node)) {
    $fp = popen("$TBSUEXEC_PATH $uid nobody webnode_reboot -w $node_id",
		"r");
}
else {
    $fp = popen("$TBSUEXEC_PATH $uid nobody webnode_reboot -w -e $pid,$eid",
		"r");
}
if (! $fp) {
    USERERROR("Reboot failed!", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

echo date("D M d G:i:s T");
echo "\n";
while (!feof($fp)) {
    $string = fgets($fp, 1024);
    echo "$string";
    flush();
}
echo date("D M d G:i:s T");
echo "\n";
pclose($fp);
$fp = 0;

?>
