<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in users can watch LEDs
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node",    PAGEARG_NODE,
				 "service", PAGEARG_STRING,
				 "init",    PAGEARG_STRING);

if (!$node->AccessCheck($this_user, $TB_NODEACCESS_READINFO)) {
    USERERROR("Not enough permission.", 1);
}

$port = "";
$initPacket = "";

if ($service == "telemetry") {
    $port = 2531;
    $initPacket = base64_decode($init);
}
else {
    USERERROR("Unknown service: $service.", 1);
}

if ($node->NodeStatus() != "up") {
    USERERROR("Node is down.", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

$socket = fsockopen("$node_id", $port);

#
# Clean up when the remote user disconnects
#
function SPEWCLEANUP()
{
    global $socket;

    if (!$socket || !connection_aborted()) {
	exit();
    }
    fclose($socket);
    exit();
}
# ignore_user_abort(1);
register_shutdown_function("SPEWCLEANUP");

if ($initPacket != "") {
     fwrite($socket, $initPacket);
     fflush($socket);
}

do {
    $bits = fread($socket, 512);
    echo "$bits";
    flush();
} while($bits != "");

fclose($socket);

?>
