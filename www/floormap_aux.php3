<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. 
#

#
# Only admin people for now.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify arguments.
# 
if (!isset($prefix) ||
    strcmp($prefix, "") == 0) {
    PAGEARGERROR("You must provide a prefix argument.");
}
if (!preg_match("/^floormap[-\w]+$/", $prefix)) {
    PAGEARGERROR("Invalid prefix argument.");
}
$prefix = "/tmp/$prefix";

#
# Need cleanup "handler" to make sure temp files get deleted! 
#
function CLEANUP()
{
    global $prefix;
    
    if (isset($prefix)) {
	unlink("${prefix}.png");
	unlink("${prefix}.map");
	unlink($prefix);
    }
    exit();
}
register_shutdown_function("CLEANUP");

#
# Spit the areamap contained in the file out; it is fully formatted and
# called "floormap".
#
if (($fp = fopen("${prefix}.png", "r"))) {
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
