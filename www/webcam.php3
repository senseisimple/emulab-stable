<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Robot Web Cams");

#
# Only known and logged in users can end experiments.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

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
if (!$isadmin && !TBWebCamAllowed($uid)) {
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

echo "<table cellpadding='0' cellspacing='0' border='0' class='stealth'>\n";

while ($row = mysql_fetch_array($query_result)) {
    $id      = $row["id"];
    $updated = $row["last_update"];

    echo "<tr><td align=center>Web Cam $id. Last Update: $updated</td></tr>
          <tr><td align=center class='stealth'>
                <img src='webcamimg.php3?webcamid=$id' align=center></td></tr>
          <tr><tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
