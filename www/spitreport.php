<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

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
    PAGEARGERROR("Invalid characters in Project ID!");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid characters in Experiment ID!");
}

if (! TBValidExperiment($pid, $eid)) {
    USERERROR("The experiment $eid is not a valid experiment ".
	      "in project $pid.", 1);
}

if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("Not enough permission to view experiment $pid/$eid", 1);
}    

$output = array();
$retval = 0;

if ($showevents) {
    $flags = "-v";
}
else {
    # Show event summary and firewall info.
    $flags = "-b -e -f";
}

$result =
    exec("$TBSUEXEC_PATH $uid $TBADMINGROUP webtbreport $flags $pid $eid",
	 $output, $retval);

header("Content-Type: text/plain");
for ($i = 0; $i < count($output); $i++) {
    echo "$output[$i]\n";
}
echo "\n";
