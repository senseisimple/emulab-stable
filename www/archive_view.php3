<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Only known and logged in users can look at experiments.
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
if (!TBvalid_pid($pid)) {
    PAGEARGERROR("Invalid project ID.");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid experiment ID.");
}

#
# Standard Testbed Header now that we have the pid/eid okay.
#
PAGEHEADER("Experiment Archive ($pid/$eid)");

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $eid!", 1);
}

$exptidx = TBExptIndex($pid, $eid);
if ($exptidx < 0) {
    TBERROR("Could not get experiment index for $pid/$eid!", 1);
}
$url = "cvsweb/cvsweb.php3/${exptidx}?exptidx=$exptidx";

echo "<center>\n";
echo "This is the Subversion archive for your experiment.<br>";
echo "<form action='archive_control.php3' method=get>\n";
echo "<b><input type=submit name=commit value='Force Commit'></b>\n";
echo "<input type=hidden name=pid value='$pid'>";
echo "<input type=hidden name=eid value='$eid'>";
echo "</form>\n";
if (isset($commit)) {
    echo "<b>Archive sucessfully committed.</b><br>";
}
echo "</center>\n";

echo "<iframe width=100% height=800 scrolling=yes src='$url' border=2>".
     "</iframe>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
