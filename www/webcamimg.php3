<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This script generates the contents of an image. No headers or footers,
# just spit back an image. 
#
function MyError($msg)
{
    # No Data. Spit back a stub image.
    #TBERROR($msg, 1);
    header("Content-type: image/gif");
    readfile("coming-soon-thumb.gif");
    exit(0);
}


#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify page arguments.
# 
if (!isset($webcamid) ||
    strcmp($webcamid, "") == 0) {
    MyError("You must provide a WebCam ID.");
}

#
# The ID is really just a filename. Make sure it looks okay.
#
if (! preg_match("/^[\d]+$/", $webcamid)) {
    MyError("Invalid characters in WebCam ID.");
}

#
# And check for the filename in the webcam directory.
#
$filename = "$TBDIR/webcams/camera-${webcamid}.jpg";

if (!file_exists($filename)) {
    MyError("Camera image file is missing!");
}

#
# Check sitevar to make sure mere users are allowed to peek at us.
#
$anyone_can_view = TBGetSiteVar("webcam/anyone_can_view");
$admins_can_view = TBGetSiteVar("webcam/admins_can_view");

if (!$admins_can_view || (!$anyone_can_view && !$isadmin)) {
    USERERROR("Webcam Views are currently disabled!", 1);
}

#
# Now check permission.
#
if (!$isadmin && !TBWebCamAllowed($uid)) {
    MyError("Not enough permission to view the robot cameras!");
}

# Spit back the image
header("Content-type: image/gif");
readfile("$filename");

#
# No Footer!
# 
?>



