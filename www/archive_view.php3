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

# This is how you get forms to align side by side across the page.
$style = 'style="float:left; width:33%;"';

echo "<center>\n";
echo "<font size=+1>
        This is the Subversion archive for your experiment.<br></font>";
echo "<form action='archive_tag.php3' $style method=get>\n";
echo "<b><input type=submit name=tag value='Tag Archive'></b>";
echo "<input type=hidden name=pid value='$pid'>";
echo "<input type=hidden name=eid value='$eid'>";
echo "</form>";
echo "<form action='archive_tags.php3' $style method=get>";
echo "<b><input type=submit name=tag value='Show Tags'></b>";
echo "<input type=hidden name=which value='$exptidx'>";
echo "</form>";
echo "<form action='archive_missing.php3' $style method=get>";
echo "<b><input type=submit name=missing value='Show Missing Files'></b>";
echo "<input type=hidden name=pid value='$pid'>";
echo "<input type=hidden name=eid value='$eid'>";
echo "</form>";
echo "</center>\n";

echo "<iframe width=100% height=800 scrolling=yes src='$url' border=2>".
     "</iframe>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
