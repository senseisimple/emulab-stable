<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

# This will not return if its a sajax request.
include("showlogfile_sup.php3");

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

$exp_eid = $eid;
$exp_pid = $pid;

# Canceled operation redirects back to showexp page. See below.
if ($canceled) {
    header("Location: showexp.php3?pid=$pid&eid=$eid");
    return;
}

#
# Standard Testbed Header, after checking for cancel above.
#
PAGEHEADER("Terminate Experiment");

#
# Check to make sure thats this is a valid PID/EID, while getting the
# experiment GID.
#
if (! TBExptGroup($exp_pid, $exp_eid, $exp_gid)) {
    USERERROR("The experiment $exp_eid is not a valid experiment ".
	      "in project $exp_pid.", 1);
}

$query_result =
    DBQueryFatal("select lockdown FROM experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
$row       = mysql_fetch_array($query_result);
$lockdown  = $row["lockdown"];

#
# Verify permissions.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_DESTROY)) {
    USERERROR("You do not have permission to end experiment $exp_eid!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$exp_pid'>$exp_pid</a>/".
     "<a href='showexp.php3?pid=$exp_pid&eid=$exp_eid'>$exp_eid</a>".
     "</b></font><br>\n";

# A locked down experiment means just that!
if ($lockdown) {
    echo "<br><br>\n";
    USERERROR("Cannot proceed; the experiment is locked down!", 1);
}
   
#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case redirect the
# browser back up a level.
#
if (!$confirmed) {
    echo "<center><br><font size=+2>
          Are you <b>REALLY</b>
          sure you want to terminate Experiment '$exp_eid?'
          </font>\n";
    echo "<br>(This will <b>completely</b> destroy all trace of the
           experiment)<br><br>\n";

    SHOWEXP($exp_pid, $exp_eid, 1);
    
    echo "<form action='endexp.php3?pid=$exp_pid&eid=$exp_eid' method=post>";
    echo "<input type=hidden name=exp_pideid value=\"$exp_pideid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

flush();

#
# Run the backend script.
#
$retval = SUEXEC($uid, "$exp_pid,$unix_gid", "webendexp $exp_pid $exp_eid",
		 SUEXEC_ACTION_IGNORE);

#
# Fatal Error. Report to the user, even though there is not much he can
# do with the error. Also reports to tbops.
# 
if ($retval < 0) {
    SUEXECERROR(SUEXEC_ACTION_DIE);
    #
    # Never returns ...
    #
    die("");
}

#
# Exit status >0 means the operation could not proceed.
# Exit status =0 means the experiment is terminating in the background.
#
echo "<br>\n";
if ($retval) {
    echo "<h3>Experiment termination could not proceed</h3>";
    echo "<blockquote><pre>$suexec_output<pre></blockquote>";
}
else {
    echo "<b>Your experiment is terminating!</b> 
          You will be notified via email when the experiment has been torn
	  down, and you can reuse the experiment name.
          This typically takes less than two minutes, depending on the
          number of nodes in the experiment.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";
    echo "<br><br>\n";
    STARTLOG($exp_pid, $exp_eid);
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
