<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
#PAGEHEADER("Watch Experiment Log");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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

for ($lpc = 0; $lpc < 30; $lpc++) {
	sleep(1);
	$on_off = $lpc % 2;
	echo "$on_off";
	flush();
}

?>
