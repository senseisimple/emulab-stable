<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid experiment.
#
if (isset($pid) && strcmp($pid, "") &&
    isset($eid) && strcmp($eid, "")) {
    if (! TBvalid_eid($eid)) {
	PAGEARGERROR("$eid contains invalid characters!");
    }
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("$pid contains invalid characters!");
    }
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("$pid/$eid is not a valid experiment!", 1);
    }
    if (TBExptState($pid, $eid) != $TB_EXPTSTATE_ACTIVE) {
	USERERROR("$pid/$eid is not currently swapped in!", 1);
    }
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to run linktest on $pid/$eid!",
		  1);
    }
}
else {
    PAGEARGERROR("Must specify pid and eid!");
}

#
# Verify the object name (a link/lan and maybe the vnode on that link/lan)
#
if (!isset($linklan) || strcmp($linklan, "") == 0) {
    USERERROR("You must provide a link name.", 1);
}
if (! TBvalid_linklanname($linklan)) {
    PAGEARGERROR("$linklan contains invalid characters!");
}

#
# Must also supply the node on the link or lan we care about.
#
if (isset($vnode) && $vnode != "") {
    if (! TBvalid_node_id($vnode)) {
	PAGEARGERROR("$vnode contains invalid characters!");
    }
}
else {
    unset($vnode);
}

#
# And we need the action to apply. 
# 
if (!isset($action) || strcmp(action, "") == 0) {
    USERERROR("You must provide an action to apply", 1);
}
if ($action != "pause" && $action != "restart" &&
    $action != "kill" && $action != "snapshot") {
    USERERROR("Invalid action to apply", 1);
}

$query_result =
    DBQueryFatal("select * from traces ".
		 "where eid='$eid' AND pid='$pid' and ".
		 "      linkvname='$linklan' ".
		 (isset($vnode) ? "and vnode='$vnode'" : ""));

if (mysql_num_rows($query_result) == 0) {
    USERERROR("No such link being traced/monitored in $eid/$pid!", 1);
}

#
# Build up a command; basically the backend is going to do nothing except
# call tevc, but we indirect through a script.
#
if (!TBExptGroup($pid, $eid, $gid)) {
    TBERROR("Could not get gid for $pid/$eid", 1);
}
TBGroupUnixInfo($pid, $gid, $unix_gid, $unix_name);

$cmd = "weblinkmon_ctl " .
       ((isset($vnode) ? "-s $vnode " : "")) .
       "$pid $eid $linklan $action";

$retval = SUEXEC($uid, "$pid,$unix_gid", $cmd, SUEXEC_ACTION_IGNORE);
if ($retval) {
    # Ug, I know this hardwired return value is bad! 
    if ($retval == 2) {
        # A usage error, reported to user. At some point the
        # approach is to do all the argument checking first.
	SUEXECERROR(SUEXEC_ACTION_USERERROR);
    }
    else {
         # All other errors reported to tbops since they are bad!
	SUEXECERROR(SUEXEC_ACTION_DIE);
    }
}

#
# Otherwise, return the user to the list page.
#
header("Location: linkmon_list.php3".
       "?pid=$pid&eid=$eid&linklan=$linklan&action=$action&vnode=$vnode");

?>
