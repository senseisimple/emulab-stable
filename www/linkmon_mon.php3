<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Check to make sure a valid experiment.
#
if (isset($pid) && strcmp($pid, "") &&
    isset($eid) && strcmp($eid, "")) {
    if (! TBvalid_eid($eid)) {
	PAGEARGERROR("$eid contains invalid characters!");
    }
    if (! TBvalid_pid($pid)) {
	PAGEARGERROR("$pid contains invalid characters!");
    }
    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("$pid/$eid is not a valid experiment!", 1);
    }
    if (TBExptState($pid, $eid) != $TB_EXPTSTATE_ACTIVE) {
	USERERROR("$pid/$eid is not currently swapped in!", 1);
    }
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_MODIFY)) {
	USERERROR("You do not have permission to run linktest on $pid/$eid!",
		  1);
    }
}
else {
    PAGEARGERROR("Must specify pid and eid!");
}

#
# Verify the object name (a link/lan and maybe the vnode on that link/lan)
#
if (!isset($linklan) || strcmp($linklan, "") == 0) {
    USERERROR("You must provide a link name.", 1);
}
if (! TBvalid_linklanname($linklan)) {
    PAGEARGERROR("$linklan contains invalid characters!");
}

#
# Must also supply the node on the link or lan we care about.
# 
if (!isset($vnode) || strcmp($vnode, "") == 0) {
    USERERROR("You must provide a node name.", 1);
}
if (! TBvalid_vnode_id($vnode)) {
    PAGEARGERROR("$vnode contains invalid characters!");
}

#
# Look to see if an update frequency provide (in seconds, convert to ms).
# Default to 1 second.
#
$freq = 1000;

if (isset($updatefreq) && $updatefreq != "" &&
    TBvalid_tinyint($updatefreq)) {
    $freq = $updatefreq * 1000;
}

#
# Verify Permission.
#
if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to view experiment $pid/$eid!", 1);
}

#
# Okay, grab the appropriate line from the traces table.
# 
$query_result =
    DBQueryFatal("select * from traces ".
		 "where pid='$pid' and eid='$eid' and ".
		 "      linkvname='$linklan' and vnode='$vnode'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("No trace is being performed on $linkname/$vnode", 1);
}
$tracerow = mysql_fetch_array($query_result);
$nodeid   = $tracerow['node_id'];
$idx      = $tracerow['idx'];

#
# Wow, what a kludge. The port number is the index + 4442. See rc.trace.
#
$portnum  = 4442 + $idx;

#
# Clean up when the remote user disconnects
#
$socket = 0;

function SPEWCLEANUP()
{
    global $socket;

    if (!$socket || !connection_aborted()) {
	exit();
    }
    fclose($socket);
    exit();
}
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

# Avoid PHP error reporting in sockopen that confuse the headers.
error_reporting(0);

$socket = fsockopen($nodeid, $portnum);
if (!$socket) {
    USERERROR("Error opening $nodeid/$portnum - $errstr",1);
}

#
# The protocol is that the other end spits out two lines, and then we
# tell it the delay between updates. 
# 
# First line is number of links.
#
if (! ($str = fgets($socket, 1024))) {
    TBERROR("Error getting first line of output", 1);
}
if (! preg_match("/^(\d*)$/", $str, $matches)) {
    TBERROR("Error parsing first line of output", 1);
}
$numlinks = $matches[1];

#
# Now suck up that many lines.
#
while ($numlinks) {
    fgets($socket, 1024);
    $numlinks--;
}

# And write the update frequency to it.
fwrite($socket, "$freq\n");

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

while (! feof($socket)) {
    $buffer = fgets($socket, 1024);
    echo $buffer;
    flush();
}
fclose($socket);

?>
