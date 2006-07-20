<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003, 2006 University of Utah and the Flux Group.
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
if (!TBvalid_pid($pid)) {
    PAGEARGERROR("Invalid project ID.");
}
if (!TBvalid_eid($eid)) {
    PAGEARGERROR("Invalid experiment ID.");
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
# See if any vis data. If not, then the renderer has not finished yet.
#
$query_result =
    DBQueryFatal("select vname from vis_nodes ".
		 "where pid='$pid' and eid='$eid' limit 1");

if (!$query_result || !mysql_num_rows($query_result)) {
    # No Data. Spit back a stub image.
    header("Content-type: image/gif");
    readfile("coming-soon-thumb.gif");
    return;
}

#
# See if we have a copy of the image in the desired zoom/detail level
# cached in the DB. If so, that is what we return.
#
$query_result =
    DBQueryFatal("select image from vis_graphs ".
		 "where pid='$pid' and eid='$eid' and ".
		 "      zoom='$zoom' and detail='$detail'");

if (mysql_num_rows($query_result)) {
    $row   = mysql_fetch_array($query_result);
    $image = $row['image'];
    
    header("Content-type: image/png");
    echo $image;
    return;
}

#
# Run in the project group.
#
$gid = $pid;
$arguments = "";

# note that we already ensured that $detail and $thumb are numeric above.
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
else {
    # No Data. Spit back a stub image.
    header("Content-type: image/gif");
    readfile("coming-soon-thumb.gif");
}

#
# No Footer!
# 
?>



