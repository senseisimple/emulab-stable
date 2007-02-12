<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
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
$reqargs = RequiredPageArguments("experiment", PAGEARG_EXPERIMENT,
				 "linklan",    PAGEARG_STRING,
				 "vnode",      PAGEARG_STRING);
$optargs = OptionalPageArguments("action",     PAGEARG_STRING,
				 "updatefreq", PAGEARG_INTEGER);

# Need these below.
$pid = $experiment->pid();
$eid = $experiment->eid();

if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
    USERERROR("You do not have permission to run linktest on $pid/$eid!", 1);
}

#
# Must be active. The backend can deal with changing the base experiment
# when the experiment is swapped out, but we need to generate a form based
# on virt_lans instead of delays/linkdelays. Thats harder to do. 
#
if ($experiment->state() != $TB_EXPTSTATE_ACTIVE) {
    USERERROR("Experiment $eid must be active to monitor its links!", 1);
}

if (! TBvalid_linklanname($linklan)) {
    PAGEARGERROR("$linklan contains invalid characters!");
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
