<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Cancel a Batch Mode Experiment");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Must provide the EID!
# 
if (!isset($exp_pideid) ||
    strcmp($exp_pideid, "") == 0) {
  USERERROR("The experiment ID was not provided!", 1);
}

#
# First get the project (PID) from the form parameter, which came in
# as <pid>$$<eid>.
#
$exp_eid = strstr($exp_pideid, "\$\$");
$exp_eid = substr($exp_eid, 2);
$exp_pid = substr($exp_pideid, 0, strpos($exp_pideid, "\$\$", 0));

#
# Check to make sure thats this is a valid PID/EID tuple.
#
if (! TBValidBatch($exp_pid, $exp_eid)) {
    USERERROR("The experiment $exp_eid is not a valid batch mode experiment ".
	      "in project $exp_pid.", 1);
}

#
# Verify Permission.that this uid is a member of the project for the experiment
# being displayed, or is an admin type.
#
if (! $isadmin &&
    ! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_DESTROY)) {
    USERERROR("You do not have permission to end batch experiments in ".
	      "project/group $exp_pid/$exp_gid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Batch Mode Experiment, Cancelation Canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you <b>REALLY</b>
          sure you want to cancel Batch Mode Experiment '$exp_eid?'
          </h2>\n";
    
    echo "<form action=\"endbatch.php3\" method=\"post\">";
    echo "<input type=hidden name=exp_pideid value=\"$exp_pideid\">\n";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# We need the gid.
#
$query_result =
    DBQueryFatal("SELECT gid FROM batch_experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
$row = mysql_fetch_array($query_result);
$exp_gid = $row[gid];

#
# and the unix stuff.
# 
TBGroupUnixInfo($exp_pid, $exp_gid, $unix_gid, $unix_name);

#
# We run a wrapper script that does all the work of terminating the
# experiment. 
#
echo "<center><br>";
echo "<h3>Starting Batch Mode Experiment Cancelation. Please wait a moment ...
      </h3></center>\n";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $unix_gid ".
	       "webkillbatchexp $exp_pid $exp_eid",
 	       $output, $retval);

if ($retval && $retval != 1) {
    echo "<br><br><h2>
          Cancelation Failure($retval): Output as follows:
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

echo "<br><br><h2>\n";

#
# Exit status 0 means cancelation was immediate.
# Exit status 1 means the experiment was running, and will terminate later.
#
if ($retval) {
	echo "Cancelation has started<br><br>
              You will be notified via email when the process has completed,
    	      and you can reuse the experiment name.
              This typically takes less than 5 minutes.
              If you do not receive email notification within a reasonable
              amount of time, please contact $TBMAILADDR.\n";
}
else {
	echo "Batchmode Experiment $exp_eid in project $exp_pid has
              been canceled!\n";
}
echo "</h2>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
