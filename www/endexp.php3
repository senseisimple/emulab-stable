<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Terminate Experiment");

#
# Only known and logged in users can end experiments.
#
$uid = "";
if ( ereg("php3\?([[:alnum:]]+)",$REQUEST_URI,$Vals) ) {
    $uid=$Vals[1];
    addslashes($uid);
} else {
    unset($uid);
}
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
# Stop now if the Confirm box was not Depressed
#
if (!isset($confirm) ||
    strcmp($confirm, "yes")) {
  USERERROR("The \"Confirm\" button was not depressed! If you really ".
            "want to end experiment '$exp_eid' in project '$exp_pid', ".
            "please go back and try again.", 1);
}

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
echo "<h3>Terminating the experiment. This may take a few minutes ...</h3>";
echo "</center>";

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
