<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminate a Testbed Experiment");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
$row = mysql_fetch_array($query_result);
$exp_gid = $row[gid];

#
# Look for termination in progress and exit with error. Other state
# errors are caught by the backend scripts. We do not set the termination
# time here. The backend duplicates the check and handles setting it.
#
$expt_locked = $row[expt_locked];
if ($expt_locked) {
    USERERROR("It appears that experiment $exp_eid went into transition at ".
	      "$expt_locked.<br>".
	      "You must wait until the experiment is no longer in transition.".
	      "<br><br>".
	      "When the transition has completed, a notification will be ".
	      "sent via email to the user that initiated it.", 1);
}

#
# Verify permissions.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_DESTROY)) {
    USERERROR("You do not have permission to end experiment $exp_eid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Experiment termination canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to terminate Experiment '$exp_eid?'
          </h2>\n";
    
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

#
# We run a wrapper script that does all the work of terminating the
# experiment. 
#
#   tbstopit <pid> <eid>
#
echo "<center><br>";
echo "<h2>Starting experiment termination. Please wait a moment ...
      </h2></center>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $unix_gid webendexp $exp_pid $exp_eid",
 	       $output, $retval);

if ($retval < 0) {
    echo "<br><br><h2>
          Termination Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
              echo "$output[$i]\n";
          }
    echo "</XMP>\n";

    PAGEFOOTER();
    die("");
}

#
# Exit status 1 means termination/cancelation was immediate.
# Exit status 0 means the experiment is terminating, or will be.
#
echo "<br><br><h3>\n";
if ($retval) {
    echo "Experiment `$exp_eid' in project `$exp_pid' has been terminated!
          <br><br>
          You may now reuse the experiment name.\n";         
}
else {
    echo "Experiment `$exp_eid' in project `$exp_pid' is terminating!<br><br>
          You will be notified via email when the experiment has been torn
	  down, and you can reuse the experiment name.
          This typically takes less than two minutes, depending on the
          number of nodes in the experiment.
          If you do not receive email notification within a reasonable amount
          of time, please contact $TBMAILADDR.\n";
}
echo "</h3>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
