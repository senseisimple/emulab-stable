<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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
# Check to make sure a valid nodeid, *or* a valid experiment.
#
if (isset($pid) && strcmp($pid, "") &&
    isset($eid) && strcmp($eid, "")) {
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("$pid/$eid is not a valid experiment!", 1);
    }

    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to reboot nodes in ".
		  "experiment $exp_eid!", 1);
    }
    $nodemode = 0;
}
elseif (isset($node_id) && strcmp($node_id, "")) {
    if (! TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }

    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_REBOOT)) {
        USERERROR("You do not have permission to reboot $node_id!", 1);
    }
    $nodemode = 1;
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
if ($canceled) {
    PAGEHEADER("Reboot Nodes");
	
    echo "<center><h3><br>
          Operation canceled!
          </h3></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    PAGEHEADER("Reboot Nodes");

    if ($nodemode) {
	echo "<center><h2><br>
              Are you <b>REALLY</b>
                sure you want to reboot node '$node_id?'
              </h2>\n";
	
	SHOWNODE($node_id, SHOWNODE_SHORT);
    }
    else {
        echo "<font size=+2>Experiment <b>".
             "<a href='showproject.php3?pid=$pid'>$pid</a>/".
             "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";

	echo "<center><font size=+2><br>
              Are you <b>REALLY</b>
                sure you want to reboot all nodes?
              </font>\n";

	SHOWEXP($pid, $eid, 1);
    }

    
    echo "<form action=boot.php3 target=_blank method=post>";
    if ($nodemode) {
	echo "<input type=hidden name=node_id value=$node_id>\n";
    }
    else {
	echo "<input type=hidden name=pid value=$pid>\n";
	echo "<input type=hidden name=eid value=$eid>\n";
    }
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

if ($nodemode) {
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

if ($nodemode) {
    $fp = popen("$TBSUEXEC_PATH $uid nobody webnodereboot -w $node_id",
		"r");
}
else {
    $fp = popen("$TBSUEXEC_PATH $uid nobody webnodereboot -w -e $pid,$eid",
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
