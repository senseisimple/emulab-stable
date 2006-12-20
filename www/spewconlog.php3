<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# No PAGEHEADER since we spit out a Location header later. See below.
# 

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid nodeid.
#
if (isset($node_id) && strcmp($node_id, "")) {
    if (! TBvalid_node_id($node_id)) {
	PAGEARGERROR("Illegal characters in node_id!");
    }
    
    if (! TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }

    if (!$isadmin &&
	!TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_READINFO)) {
        USERERROR("You do not have permission to view the console log ".
		  "for $node_id!", 1);
    }
}
else {
    PAGEARGERROR("Must specify a node ID!");
}

#
# Look for linecount argument
#
if (isset($linecount) && $linecount != "") {
    if (! TBvalid_integer($linecount)) {
	PAGEARGERROR("Illegal characters in linecount!");
    }
    $optarg = "-l $linecount";
}
else {
    $optarg = "";
}

#
# A cleanup function to keep the child from becoming a zombie.
#
$fp = 0;

function SPEWCLEANUP()
{
    global $fp;

    if (connection_aborted() && $fp) {
	pclose($fp);
    }
    exit();
}
register_shutdown_function("SPEWCLEANUP");

$fp = popen("$TBSUEXEC_PATH $uid nobody webspewconlog $optarg $node_id", "r");
if (! $fp) {
    USERERROR("Spew console log failed!", 1);
}

header("Content-Type: text/plain; charset=us-ascii");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

while (!feof($fp)) {
    $string = fgets($fp, 1024);
    echo "$string";
    flush();
}
pclose($fp);
$fp = 0;

?>
