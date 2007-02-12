<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Only known and logged in users can watch LEDs
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page args
#
$optargs = OptionalPageArguments("building",      PAGEARG_STRING,
				 "floor",         PAGEARG_STRING);

#
# Verify page arguments. Allow user to optionally specify building/floor.
#
if (isset($building) && $building != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[-\w]+$/", $building)) {
	PAGEARGERROR("Invalid building argument.");
    }
    # Optional floor argument. Sanitize for the shell.
    if (isset($floor) && !preg_match("/^[-\w]+$/", $floor)) {
	PAGEARGERROR("Invalid floor argument.");
    }
}
else {
    $building = "MEB-ROBOTS";
    $floor    = 4;
}

# Initial goo.
header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");
flush();

#
# Clean up when the remote user disconnects
#
function SPEWCLEANUP()
{
    exit(0);
}
register_shutdown_function("SPEWCLEANUP");

# Get the obstacle information.
$query_result =
    DBQueryFatal("select * from cameras ".
		 "where building='$building' and floor='$floor'");

while ($row = mysql_fetch_array($query_result)) {
    $name  = $row["name"];
    $x1    = $row["loc_x"];
    $y1    = $row["loc_y"];
    $x2    = $x1 + $row["width"];
    $y2    = $y1 + $row["height"];

    echo "$name, $x1, $y1, $x2, $y2\n";
}

?>
