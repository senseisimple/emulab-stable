<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Cancel Batch Mode Experiment");

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
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * FROM batch_experiments WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid batch mode experiment ".
            "in project $exp_pid.", 1);
}
$row = mysql_fetch_array($query_result);

#
# Verify that this uid is a member of the project for the experiment
# being displayed, or is an admin type.
#
if (! $isadmin) {
    $query_result =
	mysql_db_query($TBDBNAME,
		       "SELECT pid FROM proj_memb ".
		       "WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
    
    if (mysql_num_rows($query_result) == 0) {
	USERERROR("You are not a member of Project $exp_pid for ".
		  "Experiment: $exp_eid.", 1);
    }
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
# We need the unix gid for the project for running the scripts below.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT unix_gid from projects where pid=\"$exp_pid\"");
if (($row = mysql_fetch_row($query_result)) == 0) {
    TBERROR("Database Error: Getting GID for project $exp_pid.", 1);
}
$gid = $row[0];

#
# We run a wrapper script that does all the work of terminating the
# experiment. 
#
#   tbstopit <pid> <eid>
#
echo "<center><br>";
echo "<h3>Starting Batch Mode Experiment Cancelation. Please wait a moment ...
          </center><br><br>
      </h3>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $gid webkillbatchexp $exp_pid $exp_eid",
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

    die("");
}

echo "<center><h2><br>";
#
# Exit status 0 means cancelation was immediate.
# Exit status 1 means the experiment was running, and will terminate later.
#
if ($retval) {
	echo "Cancelation has started<br><br>
              You will be notified via email when the process has completed,
    	      and you can reuse the experiment name.<br><br>
              This might take a few minutes. Please be patient.\n";
}
else {
	echo "Batchmode Experiment $exp_eid in project $exp_pid has
              been canceled!\n";
}
echo "</center></h2>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
