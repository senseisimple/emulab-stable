<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
#PAGEHEADER("Watch Experiment Log");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

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

#
# Check to make sure this is a valid PID/EID tuple.

if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $pid/$eid is not a valid experiment!", 1);
}

#
# Verify permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view the log for $pid/$eid!", 1);
}

#
# Check for a logfile. This file is transient, so it could be gone by
# the time we get to reading it.
#
if (! TBExptLogFile($pid, $eid)) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
ignore_user_abort(0);

$retval = 0;
$result = system("$TBSUEXEC_PATH $uid $pid spewlogfile $pid $eid", $retval);

if ($retval) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

?>
