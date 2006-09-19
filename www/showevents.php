<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Watch Event Log");

#
# Only known and logged in users.
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
    USERERROR("You do not have permission to view events for $pid/$eid!", 1);
}

echo "<font size=+2>Experiment <b>".
     "<a href='showproject.php3?pid=$pid'>$pid</a>/".
     "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b></font>\n";
echo "<br>\n";

echo "<div><iframe id='outputframe' ".
      "width=100% height=600 scrolling=auto border=4></iframe></div>\n";
    
echo "<script type='text/javascript' language='javascript'>\n";
echo "SetupOutputArea('outputframe');\n"; 
echo "</script>\n";

echo "<iframe id='inputframe' name='inputframe' width=0 height=0
              src='spewevents.php?pid=$pid&eid=$eid'
              onload='StopOutput();'
              border=0 style='width:0px; height:0px; border: 0px'>
      </iframe>\n";

echo "<script type='text/javascript' language='javascript'>\n";
echo "StartOutput('inputframe', 'outputframe');\n"; 
echo "</script>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
