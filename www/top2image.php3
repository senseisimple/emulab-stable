<?php
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

#
# Spit out the image with a content header.
#
if ($fp = popen("$TBSUEXEC_PATH $uid $gid webvistopology $pid $eid", "r")) {
    header("Content-type: image/png");
    fpassthru($fp);
}

#
# No Footer!
# 
?>
