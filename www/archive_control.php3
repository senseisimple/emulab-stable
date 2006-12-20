<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide an Experiment ID.", 1);
}
if (!TBvalid_pid($pid)) {
    PAGEARGERROR("Invalid project ID.");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid experiment ID.");
}

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission to view experiment $eid!", 1);
}

$exptidx = TBExptIndex($pid, $eid);
if ($exptidx < 0) {
    TBERROR("Could not get experiment index for $pid/$eid!", 1);
}

if (!TBExptGroup($pid, $eid, $gid)) {
    TBERROR("Could not get experiment gid for $pid/$eid!", 1);
}

#
# Not many actions to consider.
#
if (isset($commit) && $commit != "") {
	SUEXEC($uid, "$pid,$gid",
	       "webarchive_control commit $pid $eid",
	       SUEXEC_ACTION_DIE);
}

$newurl = preg_replace("/archive_control/",
		       "archive_view", $_SERVER['REQUEST_URI']);

header("Location: $newurl");
?>
