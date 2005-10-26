<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Experiment Activity Log");

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

if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $pid/$eid is not a valid experiment!", 1);
}

#
# Verify permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view the log for $pid/$eid!", 1);
}

#
# Check for a logfile. This file is transient, so it could be gone by
# the time we get to reading it.
#
if (! TBExptLogFile($pid, $eid)) {
    USERERROR("Experiment $pid/$eid is no longer in transition!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br /><br />\n";

echo "<center>
      <iframe src=spewlogfile.php3?pid=$pid&eid=$eid
              width=90% height=500 scrolling=auto frameborder=1>
      Your user agent does not support frames or is currently configured
      not to display frames. However, you may visit
      <A href=spewlogfile.php3?pid=$pid&eid=$eid>the log file directly.</A>
      </iframe></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
