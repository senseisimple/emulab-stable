<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Must provide the EID!
# 
if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("The project ID was not provided!", 1);
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("The experiment ID was not provided!", 1);
}

# Canceled operation redirects back to showexp page. See below.
if ($canceled) {
    header("Location: showexp.php3?pid=$pid&eid=$eid");
    return;
}

#
# Standard Testbed Header, after cancel above.
#
PAGEHEADER("Replay Control");

$exp_eid = $eid;
$exp_pid = $pid;

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The experiment $exp_eid is not a valid experiment ".
	      "in project $exp_pid.", 1);
}
$row           = mysql_fetch_array($query_result);
$exp_gid       = $row[gid];

#
# Verify permissions.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_MODIFY)) {
    USERERROR("You do not have permission for $exp_eid!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font><br>\n";

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we 
# redirect the browser back to the experiment page (see above).
#
if (!$confirmed) {
    echo "<center><h2><br>
          Are you sure you want to replay all events in experiment '$exp_eid?'
          </h2>\n";
    
    SHOWEXP($exp_pid, $exp_eid, 1);

    echo "<form action='replayexp.php3?pid=$exp_pid&eid=$exp_eid'
                method=post>";

    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";

    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# We run a wrapper script that does all the work of restarting the events
#
echo "<center>";
echo "<h2>Starting event replay. Please wait a moment ...
      </h2></center>";

flush();

#
# Avoid SIGPROF in child.
# 
set_time_limit(0);

$retval = SUEXEC($uid, "$exp_pid,$unix_gid",
		  "webeventsys_control replay $exp_pid $exp_eid",
		 SUEXEC_ACTION_DIE);

echo "Events for your experiment are now being replayed.\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
