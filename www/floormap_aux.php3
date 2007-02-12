<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. 
#

#
# Only logged in people at the moment; might open up at some point.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify arguments.
#
$reqargs = RequiredPageArguments("prefix", PAGEARG_STRING);

# Sanity check for shell.
if (!preg_match("/^floormap[-\w]+$/", $prefix)) {
    PAGEARGERROR("Invalid prefix argument.");
}
$prefix = "/tmp/$prefix";

#
# Need cleanup "handler" to make sure temp files get deleted! 
#
function CLEANUP()
{
    global $prefix, $uid;

    #
    # The backend script (vis/floormap.in) removes all the temp files
    # with the -c option. Yucky, but file perms and owners make this
    # the easiest way to do it.
    # 
    if (isset($prefix)) {
	SUEXEC($uid, "nobody", "webfloormap -o $prefix -k ",
	       SUEXEC_ACTION_IGNORE);
	# This file does belong to the web server.
	unlink($prefix);
    }
    exit();
}
register_shutdown_function("CLEANUP");

#
# Spit the areamap contained in the file out; it is fully formatted and
# called "floormap".
#
if (($fp = fopen("${prefix}.jpg", "r"))) {
    header("Content-type: image/jpg");
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
