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
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$eid' and pid='$pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $eid is not a valid experiment ".
            "in project $pid.", 1);
}

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
if (!$isadmin) {
    $query_result =
	DBQueryFatal("SELECT pid FROM group_membership ".
		     "WHERE uid='$uid' and pid='$pid'");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $pid!", 1);
    }
}

$query_result =
    DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("There is no stored NS file for $pid/$eid", 1);
}
$row    = mysql_fetch_array($query_result);
$nsfile = $row[nsfile];

echo "<XMP>$nsfile</XMP>\n";
flush();

echo "<br>
      <center><h3>
      Here is the physical mapping for this experiment
      </h3></center>\n";

$output = array();
$retval = 0;

$result = exec("$TBSUEXEC_PATH nobody flux webreport $pid $eid",
 	       $output, $retval);

echo "<XMP>\n";
for ($i = 1; $i < count($output); $i++) {
    echo "$output[$i]\n";
}
echo "</XMP>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
