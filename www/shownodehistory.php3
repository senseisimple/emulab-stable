<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node History");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("Cannot view node history.", 1);
}

#
# Verify form arguments.
# 
if (!isset($showall)) {
    $showall = 0;
}

if (!isset($node_id) || strcmp($node_id, "") == 0) {
    $node_id = "";
} else {
    #
    # Check to make sure that this is a valid nodeid
    #
    if (!TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }
}

echo "<b>Show: 
         <a href='shownodehistory.php3?node_id=$node_id'>allocated only</a>,
         <a href='shownodehistory.php3?node_id=$node_id&showall=1'>all</a>";

SHOWNODEHISTORY($node_id, $showall);

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
