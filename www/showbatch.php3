<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Batch Mode Experiment Information");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

$isadmin = ISADMIN($uid);

#
# Verify page arguments.
# 
if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    USERERROR("You must provide an Experiment ID.", 1);
}

if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    USERERROR("You must provide a Project ID.", 1);
}

$exp_eid = $eid;
$exp_pid = $pid;

#
# Check to make sure thats this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM batch_experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid batch mode experiment ".
            "in project $exp_pid.", 1);
}
$exprow = mysql_fetch_array($query_result);

#
# Verify that this uid is a member of the project for the experiment
# being displayed.
#
if (!$isadmin) {
    $query_result =
	DBQueryFatal("SELECT pid FROM group_membership WHERE ".
		     "uid='$uid' and pid='$exp_pid'");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $exp_pid for ".
                  "Experiment: $exp_eid.", 1);
    }
}

$created   = $exprow[created];
$expires   = $exprow[expires];
$longname  = $exprow[name];
$creator   = $exprow[creator_uid];
$status    = $exprow[status];
$attempts  = $exprow[attempts];

#
# Generate the table.
# 
echo "<table align=center border=1>\n";

echo "<tr>
          <td>Name: </td>
          <td class=\"left\">$exp_eid</td>
      </tr>\n";

echo "<tr>
          <td>Long Name: </td>
          <td class=\"left\">$longname</td>
      </tr>\n";

echo "<tr>
          <td>Project: </td>
          <td class=left>
             <A href='showproject.php3?pid=$exp_pid'>$exp_pid</A></td>
      </tr>\n";

echo "<tr>
          <td>Experiment Head: </td>
          <td class=\"left\">
              <A href='showuser.php3?target_uid=$creator'>
                 $creator</td>
      </tr>\n";

echo "<tr>
          <td>Created: </td>
          <td class=\"left\">$created</td>
      </tr>\n";

echo "<tr>
          <td>Expires: </td>
          <td class=\"left\">$expires</td>
      </tr>\n";

echo "<tr>
          <td>Status: </td>
          <td class=\"left\">$status</td>
      </tr>\n";

echo "<tr>
          <td>Start Attempts: </td>
          <td class=\"left\">$attempts</td>
      </tr>\n";

echo "</table>\n";

# Terminate option.
echo "<p><center>
       Do you want to terminate this batch experiment?
       <A href='endbatch.php3?exp_pideid=$exp_pid\$\$$exp_eid'>Yes</a>
      </center>\n";    

#
# Dump experiment record if its currently running or configuring.
#
if (strcmp($status, "running") == 0 ||
    strcmp($status, "configuring") == 0) {
    echo "<center>
          <h1>Experiment Information</h1>
          </center>\n";
    SHOWEXP($exp_pid, $exp_eid);
    SHOWNODES($exp_pid, $exp_eid);
}
    
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
