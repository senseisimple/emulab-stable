<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
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
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
	
	SHOWNODE($node_id);
    }
    else {
	echo "<center><h2><br>
              Are you <b>REALLY</b>
                sure you want to reboot all nodes in experiment $pid/$eid?
              </h2>\n";

	SHOWEXP($pid, $eid);
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

#
# Pass it off to the script.
#
$retval = 0;
header("Content-Type: text/plain");
ignore_user_abort(1);

if ($nodemode) {
    $result = system("$TBSUEXEC_PATH $uid nobody webnodereboot $node_id",
		     $retval);
}
else {
    $result = system("$TBSUEXEC_PATH $uid nobody webnodereboot -e $pid,$eid",
		     $retval);
}

if ($retval) {
    USERERROR("Reboot failed!", 1);
}

#
# And send an audit message.
#
TBUserInfo($uid, $uid_name, $uid_email);

if ($nodemode) {
    $message = "$node_id was rebooted via the web interface by $uid\n";
    $subject = "Node Reboot: $node_id";
}
else {
    $message = "Nodes in $pid/$eid were rebooted via the web interface ".
	       "by $uid\n";
    $subject = "Nodes Rebooted: $pid/eid";
}

TBMAIL($TBMAIL_AUDIT,
       $subject, $message,
       "From: $uid_name <$uid_email>\n".
       "Errors-To: $TBMAIL_WWW");

#
# Spit out a redirect so that the history does not include a post
# in it. 
#
#if ($nodemode)
#    header("Location: shownode.php3?node_id=$node_id");
#else
#    header("Location: showexp.php3?exp_pid=$pid&exp_eid=$eid");

?>
