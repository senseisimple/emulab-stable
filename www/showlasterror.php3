<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Last Error");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify page arguments.
# 
if (!TBvalid_pid($pid)) {
    PAGEARGERROR("Invalid project ID.");
}

if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid experiment ID.");
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
} else {
  $row     = mysql_fetch_array($query_result);
  $exptidx = $row[idx];
}

$expstate = TBExptState($pid, $eid);

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
    DBQueryFatal("SELECT e.cause,e.confidence,e.mesg,cause_desc FROM experiment_stats as s,errors as e, causes as c ".
	         "WHERE s.exptidx = $exptidx and e.cause = c.cause and s.last_error = e.session and rank = 0");

if (mysql_num_rows($query_result) != 0) {
  $row = mysql_fetch_array($query_result);
  echo "<h2>$row[cause_desc]</h2>\n";
  echo "<pre>\n";
  echo htmlspecialchars($row[mesg])."\n";
  echo "\n";
  echo "Cause: $row[cause]\n";
  echo "Confidence: $row[confidence]\n";
  echo "</pre>\n";
} else {
  echo "Nothing\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
