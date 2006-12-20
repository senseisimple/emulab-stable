<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
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
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
# 
if (!isset($webcamid) ||
    strcmp($webcamid, "") == 0) {
    MyError("You must provide a WebCam ID.");
}

if (! preg_match("/^[\d]+$/", $webcamid)) {
    MyError("Invalid characters in WebCam ID.");
}

#
# And check for entry in webcams table, which tells us the server name
# where we open the connection to. 
#
$query_result =
    DBQueryFatal("select * from webcams where id='$webcamid'");

if (!$query_result || !mysql_num_rows($query_result)) {
    MyError("No such webcam ID: '$webcamid'");
}
$row = mysql_fetch_array($query_result);
$URL = (isset($applet) ? $row["URL"] : $row["stillimage_URL"]);
if (isset($fromtracker)) {
    #
    # Some stuff to control the camera.
    #
    if (! isset($camheight) || !TBvalid_integer($camheight)) {
	$camheight = 180;
    }
    if (! isset($camwidth) || !TBvalid_integer($camwidth)) {
	$camwidth = 240;
    }
    if (! isset($camfps) || !TBvalid_integer($camfps)) {
	$camfps = 2;
    }
    $URL .= "&resolution=${camwidth}x${camheight}";

    if (preg_match("/fps=\d*/", $URL)) {
	$URL = preg_replace("/fps=\d*/", "fps=${camfps}", $URL);
    }
    else {
	$URL .= "&fps=${camfps}";
    }
}

#
# Check sitevar to make sure mere users are allowed to peek at us.
#
$anyone_can_view = TBGetSiteVar("webcam/anyone_can_view");
$admins_can_view = TBGetSiteVar("webcam/admins_can_view");

if (!$admins_can_view || (!$anyone_can_view && !$isadmin)) {
    MyError("Webcam Views are currently disabled!");
}

#
# Now check permission.
#
if (!$isadmin && !$this_user->WebCamAllowed()) {
    MyError("Not enough permission to view the robot cameras!");
}

$socket = fopen($URL, "r");
if (!$socket) {
    TBERROR("Error opening URL $URL", 0);
    MyError("Error opening URL");
}

#
# So, the webcam spits out its own HTTP headers, which includes this
# content-type line, but all those headers are basically lost cause
# of the interface we are using (fopen). No biggie, but we have to
# spit them out ourselves so the client knows what to do.
#
if (isset($applet)) {
    header("Content-type: multipart/x-mixed-replace;boundary=--myboundary");
}

#TBERROR(print_r($http_response_header, TRUE) . "\n\n", 0);

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
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

#
# Spit back the image. The webcams include all the necessary headers,
# so do not spit any headers here.
#
fpassthru($socket);
fclose($socket);

#
# No Footer!
# 
?>



