<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005-2011 University of Utah and the Flux Group.
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
# Verify page arguments.
#
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "linklan",    PAGEARG_STRING,
				 "action",     PAGEARG_STRING);
$optargs = OptionalPageArguments("vnode",      PAGEARG_STRING);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();
$unix_gid = $experiment->UnixGID();
$project  = $experiment->Project();
$unix_pid = $project->unix_gid();

if (!$experiment->AccessCheck($this_user, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to run linktest on $pid/$eid!", 1);
}

#
# Must be active. The backend can deal with changing the base experiment
# when the experiment is swapped out, but we need to generate a form based
# on virt_lans instead of delays/linkdelays. Thats harder to do. 
#
if ($experiment->state() != $TB_EXPTSTATE_ACTIVE) {
    USERERROR("Experiment $eid must be active to monitor its links!", 1);
}

if (! TBvalid_linklanname($linklan)) {
    PAGEARGERROR("$linklan contains invalid characters!");
}

#
# Optional vnode on the link.
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
$cmd = "weblinkmon_ctl " .
       ((isset($vnode) ? "-s $vnode " : "")) .
       "$pid $eid $linklan $action";

$retval = SUEXEC($uid, "$unix_pid,$unix_gid", $cmd, SUEXEC_ACTION_IGNORE);
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
header("Location: ". CreateURL("linkmon_list", $experiment,
			       "linklan", $linklan, "action", $action,
			       "vnode", (isset($vnode) ? $vnode : "")));

?>
