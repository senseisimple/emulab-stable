<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Batch Mode Experiment Information");

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
	"SELECT * FROM batch_experiments WHERE ".
        "eid=\"$exp_eid\" and pid=\"$exp_pid\"");
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
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT pid FROM proj_memb WHERE uid=\"$uid\" and pid=\"$exp_pid\"");
    if (mysql_num_rows($query_result) == 0) {
        USERERROR("You are not a member of Project $exp_pid for ".
                  "Experiment: $exp_eid.", 1);
    }
}

$created   = $exprow[created];
$expires   = $exprow[expires];
$longname  = $exprow[name];
$creator   = $exprow[creator_uid];
$numpcs    = $exprow[numpcs];
$numsharks = $exprow[numsharks];
$status    = $exprow[status];
$attempts  = $exprow[attempts];

#
# Generate the table.
# 
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

echo  "<tr>
          <td>Estimated #PCs: </td>
          <td class=\"left\">$numpcs</td>
      </tr>\n";

echo  "<tr>
          <td>Estimated #Sharks: </td>
          <td class=\"left\">$numsharks</td>
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
# Dump experiment record if its currently running.
#
if (strcmp($status, "running") == 0) {
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
