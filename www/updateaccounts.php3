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
PAGEHEADER("Update Accounts on Experimental Nodes");

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
# errors are caught by the backend scripts. 
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
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_UPDATEACCOUNTS)) {
    USERERROR("You do not have permission to update accounts!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Account Update canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "Confirming this operation will cause the password file on each
          node in experiment '$exp_eid' to be updated. Typically, you would
          initiate this operation if a new project member has been approved and
          you want his/her account added immediately (without having to reboot
          all your nodes or swap the experiment in/out).\n";

    echo "<br><br>
          <center>
          Are you <b>sure</b> you want to update Accounts
          on all of the nodes in experiment '$exp_eid?'\n";
    
    echo "<form action='updateaccounts.php3?pid=$pid&eid=$eid' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    echo "<br>
          This operation will consume a small number of CPU cycles on
          each node. If this would disturb an experiment in progress, you
          should cancel this operation until later.
          Please note that accounts are automatically updated whenever a node
          reboots, or when an experiment is swapped back in.\n";

    PAGEFOOTER();
    return;
}

#
# We need the unix gid for the project for running the scripts below.
# Note usage of default group in project.
#
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# Start up a script to do this.
#
echo "<center><br>";
echo "<h2>Starting account update. Please wait a moment ...
      </h2></center>";

flush();

SUEXEC($uid, $unix_gid, "webnodeupdate -b $pid $eid", 1);

echo "<p>
      You will be notified via email when the update has completed on
      all of the nodes in your experiment. Since this involves rebuilding
      the accounts database on each node, it should be just a few minutes
      before you receive notification.\n";

#
# Back links for convenience.
#
echo "<p>
      <a href='showexp.php3?pid=$pid&eid=$eid'>
         Back to experiment information page</a>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
