<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Show Experiment Information");

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

#
# Dump experiment record.
# 
echo "<center>
      <h1>Experiment Information</h1>
      </center>\n";
SHOWEXP($exp_pid, $exp_eid);

# Terminate option.
echo "<p><center>
       Do you want to terminate this experiment?
       <A href='endexp.php3?exp_pideid=$exp_pid\$\$$exp_eid'>Yes</a>
      </center>\n";    

#
# Dump the node information.
#
SHOWNODES($exp_pid, $exp_eid);
    
#
# Lets dump the project information too.
#
echo "<center>
      <h3>Project Information</h3>
      </center>\n";
SHOWPROJECT($exp_pid, $uid);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
