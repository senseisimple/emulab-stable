<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
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
if (!isset($node) || strcmp($node, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

if (!TBvalid_node_id($node)) {
    USERERROR("Invalid node ID.", 1);
}

if (!TBValidNodeName($node)) {
    USERERROR("Invalid node ID.", 1);
}

if (!TBNodeAccessCheck($uid, $node, $TB_NODEACCESS_READINFO)) {
    USERERROR("Not enough permission.", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

#for ($lpc = 0; $lpc < 30; $lpc++) {
    #	sleep(1);
    #	$on_off = $lpc % 2;
    #	echo "$on_off";
    #	flush();
    #}

#
# Silly, I can't get php to get the buffering behavior I want with a socket, so
# we'll open a pipe to a perl process
#
$socket = fsockopen("$node", 1812);
if (!$socket) {
    USERERROR("Error opening $node - $errstr",1);
}

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

#
# Just loop forver reading from the socket
#
do {
    # Bad rob! No biscuit!
    $onoff = fread($socket,6);
    echo "$onoff";
    flush();
} while ($onoff != "");
fclose($socket);

?>
