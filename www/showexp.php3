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
$exp_eid = $eid;
$exp_pid = $pid;

#
# Check to make sure this is a valid PID/EID tuple.
#
$query_result =
    DBQueryFatal("SELECT * FROM experiments WHERE ".
		 "eid='$exp_eid' and pid='$exp_pid'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The experiment $exp_eid is not a valid experiment ".
            "in project $exp_pid.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $exp_pid, $exp_eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $exp_eid!", 1);
}

#
# Dump experiment record.
# 
SHOWEXP($exp_pid, $exp_eid);

# Terminate option.
echo "<p><center>
       Do you want to terminate this experiment?
       <A href='endexp.php3?pid=$exp_pid&eid=$exp_eid'>Yes</a>
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
