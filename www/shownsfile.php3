<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show me the NS File");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($exp_pideid) ||
    strcmp($exp_pideid, "") == 0) {
    USERERROR("You must provide an experiment ID.", 1);
}

#
# First get the project (PID) from the form parameter, which came in
# as <pid>$$<eid>.
#
$exp_eid = strstr($exp_pideid, "$$");
$exp_eid = substr($exp_eid, 2);
$exp_pid = substr($exp_pideid, 0, strpos($exp_pideid, "$$", 0));

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
if (!$isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $exp_pid for ".
                  "Experiment: $exp_eid.", 1);
    }
}

$query_result = mysql_db_query($TBDBNAME,
       "SELECT nsfile from nsfiles where pid='$exp_pid' and eid='$exp_eid'");
if (!$query_result ||
    mysql_num_rows($query_result) == 0) {
    USERERROR("There is no stored NS file for $exp_pid/$exp_eid", 1);
}
$row    = mysql_fetch_array($query_result);
$nsfile = $row[nsfile];

echo "<XMP>$nsfile</XMP>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
