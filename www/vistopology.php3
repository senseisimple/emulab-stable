<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Graphic Visualization of Topology");

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
if (! TBValidExperiment($exp_pid, $exp_eid)) {
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
# Dump the NS file so that the user can cross reference the visualization
# against the NS file that created it.
# 
$query_result =
    DBQueryFatal("SELECT nsfile from nsfiles where pid='$pid' and eid='$eid'");

if (mysql_num_rows($query_result)) {
    $row    = mysql_fetch_array($query_result);
    $nsfile = $row[nsfile];

    echo "<XMP>$nsfile</XMP>\n";
    flush();
}

#
# Spit out an image that refers to a php script. That script will run and
# send back the GIF image contents.
#
echo "<br>
      <center>
       <img src='top2gif.php3?pid=$exp_pid&eid=$exp_eid' align=center>
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
