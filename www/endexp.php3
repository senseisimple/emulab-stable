<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminate Experiment");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
	"SELECT * FROM experiments WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("You are not a member of Project $exp_pid for ".
            "Experiment: $exp_eid.", 1);
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
    
    echo "<form action=\"endexp.php3\" method=\"post\">";
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
echo "<h3>Terminating the experiment. This may take a few minutes ...
          </center><br><br>
	  Please do <em>not</em> click the 'Stop' button. This will cause
	  the experiment teardown to terminate prematurely, which can cause
  	  problems for future (other) experiments.
      </h3>";

flush();

#
# Run the scripts. We use a script wrapper to deal with changing
# to the proper directory and to keep some of these details out
# of this. 
#
$output = array();
$retval = 0;
$result = exec("$TBSUEXEC_PATH $uid $gid tbstopit $exp_pid $exp_eid",
 	       $output, $retval);

if ($retval && $retval != 69) {
    echo "<br><br><h2>
          Termination Failure($retval): Output as follows:
          </h2>
          <br>
          <XMP>\n";
          for ($i = 0; $i < count($output); $i++) {
              echo "$output[$i]\n";
          }
    echo "</XMP>\n";

    die("");
}

#
# From the database too!
#
$query_result = mysql_db_query($TBDBNAME,
	"DELETE FROM experiments WHERE eid='$exp_eid' and pid=\"$exp_pid\"");
if (! $query_result) {
    $err = mysql_error();
    TBERROR("Database Error deleting experiment $exp_eid ".
            "in project $exp_pid: $err\n", 1);
}

echo "<center><br>";
echo "<h2>Experiment '$exp_eid' in project '$exp_pid' Terminated!<br>";
if ($retval == 69) {
    echo "Since there was no experiment directory to work from, the EID has
          been removed<br>
          but you will need to make sure the nodes/vlans are released
          yourself.\n";
}
echo "</h2>";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
