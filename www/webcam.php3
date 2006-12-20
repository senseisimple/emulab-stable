<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Robot Web Cams");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

# Helper function.
function MyError($msg)
{
    # No Data. Spit back a stub image.
    USERERROR($msg, 1);
    exit(0);
}

#
# Check sitevar to make sure mere users are allowed to peek at us.
#
$anyone_can_view = TBGetSiteVar("webcam/anyone_can_view");
$admins_can_view = TBGetSiteVar("webcam/admins_can_view");

if (!$admins_can_view || (!$anyone_can_view && !$isadmin)) {
    MyError("Webcam Views are currently disabled!", 1);
}

#
# Now check permission.
#
if (!$isadmin && !$this_user->WebCamAllowed()) {
    MyError("Not enough permission to view the robot cameras!");
}

#
# Get the set of webcams, and present a simple table of images, with titles.
#
$query_result =
    DBQueryFatal("select * from webcams");

if (!$query_result || !mysql_num_rows($query_result)) {
    MyError("There are no webcams to view!");
}

$extra_url = "&fromtracker=1";
if (!isset($camwidth) || !TBvalid_integer($camwidth)) {
    $camwidth = "640";
}
$extra_url .= "&camwidth=$camwidth";
if (!isset($camheight) || !TBvalid_integer($camheight)) {
    $camheight = "480";
}
$extra_url .= "&camheight=$camheight";
if (isset($camfps) && TBvalid_integer($camfps)) {
    $extra_url .= "&camfps=$camfps";
}

if (isset($refreshrate)) {
    echo "<center>
          <a href=webcam.php3>Stop Auto-refresh.</a>
          </center>\n";
}
else {
    echo "<center>
          <a href=webcam.php3?refreshrate=2>Auto-refresh</a> images at two
             second interval, or<br>
          <a href=webcam.php3?applet=1>Live Image</a> using a java applet.
          </center>\n";
}

if (isset($applet)) {
    $auth    = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
    
    while ($row = mysql_fetch_array($query_result)) {
	$id  = $row["id"];
	$url = "webcamimg.php3?webcamid=${id}&nocookieuid=${uid}".
	    "&nocookieauth=${auth}&applet=1" . $extra_url;

	if (!isset($camera) || $camera == $id) {
	    echo "<applet archive=WebCamApplet.jar
	                  code=WebCamApplet.class height=$camheight width=$camwidth>
                          <param name=URL value=$url>
                  </applet><br<br>\n";
	}
    }
}
else {
    echo "<table cellpadding='0' cellspacing='0' border='0'
                 class='stealth'>\n";

    while ($row = mysql_fetch_array($query_result)) {
	$id      = $row["id"];

	if (!isset($camera) || $camera == $id) {
	    echo "<tr><td align=center>Web Cam $id</td></tr>
        	  <tr><td align=center class='stealth'>
                	<img src='webcamimg.php3?webcamid=${id}${extra_url}'
                        	align=center></td></tr>
	          <tr><tr>\n";
	}
    }
    echo "</table>\n";
}

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
