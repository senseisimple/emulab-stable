<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

function SPITERROR($code, $msg)
{
    header("HTTP/1.0 $code $msg");
    exit();
}

#
# Must be SSL, even though we do not require an account login.
#
if (!isset($SSL_PROTOCOL)) {
    SPITERROR(400, "Must use https:// to access this page!");
}

#
# Verify page arguments.
# 
if (!isset($nodeid) ||
    strcmp($nodeid, "") == 0) {
    SPITERROR(400, "You must provide a node ID.");
}
if (!isset($file) ||
    strcmp($file, "") == 0) {
    SPITERROR(400, "You must provide an filename.");
}
if (!isset($key) ||
    strcmp($key, "") == 0) {
    SPITERROR(400, "You must provide an key.");
}

#
# Make sure a reserved node.
#
if (! TBNodeIDtoExpt($nodeid, $pid, $eid, $gid)) {
    SPITERROR(400, "$nodeid is not reserved to an experiment!");
}
TBExpLeader($pid, $eid, $creator);

#
# We need the secret key. 
#
$query_result =
    DBQueryFatal("select keyhash from experiments ".
		 "where pid='$pid' and eid='$eid'");

if (mysql_num_rows($query_result) == 0) {
    SPITERROR(403, "No key defined for this experiment!");
}
$row = mysql_fetch_array($query_result);

if (!isset($row["keyhash"]) || !$row["keyhash"]) {
    SPITERROR(403, "No key defined for this experiment!");
}
if (strcmp($row["keyhash"], $key)) {
    SPITERROR(403, "Wrong Key!");
}

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    pclose($fp);
    exit();
}
ignore_user_abort(1);
register_shutdown_function("SPEWCLEANUP");

#
# Run once with just the verify option to see if the file exists.
# Then do it for real, spitting out the data. Sure, the user could
# delete the file in the meantime, but thats his problem. 
#
$retval = SUEXEC($creator, $gid, "spewrpmtar -v $nodeid $file",
		 SUEXEC_ACTION_IGNORE);

if ($retval) {
    SPITERROR(404, "Could not find $file!");
} 

#
# Okay, now do it for real. 
# 
if ($fp = popen("$TBSUEXEC_PATH $creator $gid ".
		"spewrpmtar $nodeid $file", "r")) {
    header("Content-Type: application/octet-stream");
    header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
    header("Cache-Control: no-cache, must-revalidate");
    header("Pragma: no-cache");

    fpassthru($fp);
    $fp = 0;
    flush();
}
else {
    SPITERROR(404, "Could not find $file!");
}

?>
