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
# Standard Testbed Header.
#
PAGEHEADER("Experiment Archive ($exptidx)");

#
# Verify page arguments.
#
if (! isset($exptidx) || $exptidx == "") {
    USERERROR("Must supply an experiment to view!", 1);
}
if (!TBvalid_integer($exptidx)) {
    USERERROR("Invalid characters in $exptidx!", 1);
}

#
# We get an index. Must map that to a pid/gid to do a group level permission
# check, since it might not be an current experiment.
#
unset($pid);
unset($eid);
unset($gid);
if (TBExptidx2PidEid($exptidx, $pid, $eid, $gid) < 0) {
    USERERROR("No such experiment index $exptidx!", 1);
}

if (!$isadmin &&
    !TBProjAccessCheck($uid, $pid, $gid, $TB_PROJECT_READINFO)) {
    USERERROR("You do not have permission to view tags for ".
	      "archive in $pid/$gid ($exptidx)!", 1);
}
$current = TBCurrentExperiment($exptidx);

$url = preg_replace("/archive_view/", "cvsweb/cvsweb",
		    $_SERVER['REQUEST_URI']);

# This is how you get forms to align side by side across the page.
if ($current) {
    $style = 'style="float:left; width:33%;"';
}
else {
    $style = "";
}

echo "<center>\n";
echo "<font size=+1>
        This is the Subversion archive for experiment $exptidx.<br></font>";

if ($current) {
    echo "<form action='${TBBASE}/archive_tag.php3' $style method=get>\n";
    echo "<b><input type=submit name=tag value='Tag Archive'></b>";
    echo "<input type=hidden name=exptidx value='$exptidx'>";
    echo "</form>";
}

echo "<form action='${TBBASE}/archive_tags.php3' $style method=get>";
echo "<b><input type=submit name=tag value='Show Tags'></b>";
echo "<input type=hidden name=exptidx value='$exptidx'>";
echo "</form>";

if ($current) {
    echo "<form action='${TBBASE}/archive_missing.php3' $style method=get>";
    echo "<b><input type=submit name=missing value='Show Missing Files'></b>";
    echo "<input type=hidden name=exptidx value='$exptidx'>";
    echo "</form>";
} 

echo "<iframe width=100% height=800 scrolling=yes src='$url' border=2>".
     "</iframe>\n";
echo "</center><br>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
