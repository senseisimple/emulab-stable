<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. 
#

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

# if they dont exist, or are non-numeric, use defaults.
# note: one can use is_numeric in php4 instead of ereg.
if (!isset($zoom) || !ereg("^[0-9]{1,50}.?[0-9]{0,50}$", $zoom)) { $zoom = 1; }
if (!isset($detail) || !ereg("^[0-9]{1,50}$", $detail)) { $detail = 0; }
if (!isset($thumb) || !ereg("^[0-9]{1,50}$", $detail)) { $thumb = 0; }

if ($zoom > 8.0) { $zoom = 8.0; }
if ($zoom <= 0.0) { $zoom = 1.0; }

if ($thumb > 1024) { $thumb = 1024; }

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
# XXX If an admin type, then use an appropriate gid so that we can get
# get to the top file. This needs more thought.
#
$gid = "nobody";

if (ISADMIN($uid)) {
    $gid = $exp_pid;
}

$arguments = "";

# note that we've already ensured that $detail and $thumb are numeric above.
if ($detail != 0) { $arguments .= " -d $detail"; }
if ($thumb != 0)  { $arguments .= " -t $thumb";  }

#
# Spit out the image with a content header.
#

if ($fp = popen("$TBSUEXEC_PATH $uid $gid webvistopology " .
		"$arguments -z $zoom $pid $eid", "r")) {
    header("Content-type: image/png");
    fpassthru($fp);
}

#
# No Footer!
# 
?>



