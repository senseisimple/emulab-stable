<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Robot Map");

#
# Only logged in people at the moment; might open up at some point.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

# Careful with this local variable
unset($prefix);

#
# One robot map right now ...
# 
$building = "MEB-ROBOTS";
$floor    = 4;

#
# Optional pid,eid. Without a building/floor, show all the nodes for the
# experiment in all buildings/floors. Without pid,eid show all wireless
# nodes in the specified building/floor.
#
if (isset($pid) && $pid != "" && isset($eid) && $eid != "") {
    if (!TBvalid_pid($pid)) {
	PAGEARGERROR("Invalid project ID.");
    }
    if (!TBvalid_eid($eid)) {
	PAGEARGERROR("Invalid experiment ID.");
    }

    if (! TBValidExperiment($pid, $eid)) {
	USERERROR("The experiment $pid/$eid is not a valid experiment!", 1);
    }
    if (! TBExptAccessCheck($uid, $pid, $eid, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view experiment $pid/$eid!", 1);
    }
}
else {
    unset($pid);
    unset($eid);
}

#
# map_x and map_y are the map.x and map.y coordinates from clicking on the map.
#
if (isset($map_x) && $map_x != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $map_x)) {
	PAGEARGERROR("Invalid map_x argument.");
    }
}
else {
    unset($map_x);
}
if (isset($map_y) && $map_y != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $map_y)) {
	PAGEARGERROR("Invalid map_y argument.");
    }
}
else {
    unset($map_y);
}

#
# max_x and max_y are the bounds of the image from the state file.
#
if (0) {
if (isset($max_x) && $max_x != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $max_x)) {
	PAGEARGERROR("Invalid max_x argument.");
    }
}
else {
    PAGEARGERROR("Must supply max_x for image");
}
if (isset($max_y) && $max_y != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $max_y)) {
	PAGEARGERROR("Invalid max_y argument.");
    }
}
else {
    PAGEARGERROR("Must supply max_y for image");
}
} 

#
# Assume a single image for the robot map. When user clicks, pixel
# coords correspond to building and floor.
#
$query_result =
    DBQueryFatal("select * from floorimages where ".
		 "building='$building' and floor='$floor'");
if (mysql_num_rows($query_result)) {
    $row = mysql_fetch_array($query_result);
    $pixels_per_meter = $row["pixels_per_meter"];
}

#
# Build a table of location info to print in table below image.
#
$locations = array();

$query_result =
    DBQueryFatal("select loc.* from location_info as loc ".
		 "where loc.floor='$floor' and loc.building='$building'");

while ($row = mysql_fetch_array($query_result)) {
    $node_id   = $row["node_id"];
    $loc_x     = $row["loc_x"];
    $loc_y     = $row["loc_y"];

    if (isset($pixels_per_meter) && $pixels_per_meter) {
	$meters_x = sprintf("%.3f", $loc_x / $pixels_per_meter);
	$meters_y = sprintf("%.3f", $loc_y / $pixels_per_meter);

	$locations[$node_id] = "x=$meters_x, y=$meters_y meters";
    }
    else {
	$locations[$node_id] = "x=$loc_x, y=$loc_y pixels";
    }
}

#
# Run the Perl script. It will produce three output files; image, areamap,
# and state. We want to embed all of these images into the page we
# send back. This is painful!  
#
# Need cleanup "handler" to make sure temp files get deleted! 
#
function CLEANUP()
{
    global $prefix;
    
    if (isset($prefix) && (connection_aborted())) {
	unlink("${prefix}.jpg");
	unlink("${prefix}.map");
	unlink("${prefix}.state");
	unlink($prefix);
    }
    exit();
}
register_shutdown_function("CLEANUP");

#
# Create a tempfile to use as a unique prefix; it is not actually used but
# serves the same purpose (The script uses ${prefix}.jpg and ${prefix}.map .)
# 
$prefix = tempnam("/tmp", "floormap");

#
# Get the unique part to send back.
#
if (!preg_match("/^\/tmp\/([-\w]+)$/", $prefix, $matches)) {
    TBERROR("Bad tempnam: $prefix", 1);
}
$uniqueid = $matches[1];

$perl_args = "-o $prefix -t -z " .
	     # From clicking on a map image.
	     (isset($map_x) ? "-c $map_x,$map_y " : "") .
	     (isset($floor) ? "-f $floor " : "") .
	     (isset($building) ? "$building" : "");  # Building arg must be last!

if (0) {    ### Put the Perl script args into the page when debugging.
    echo "\$btfv/floormap -d $perl_args\n";
}
$retval = SUEXEC($uid, "nobody", "webfloormap $perl_args", SUEXEC_ACTION_IGNORE);

if ($retval) {
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    # Never returns.
    die("");
}

#
# Spit the areamap contained in the file out; it is fully formatted and
# called "floormap".
#
if (! readfile("${prefix}.map")) {
    TBERROR("Could not read ${prefix}.map", 1);
}

echo "<center>\n";

# Wrap the image and zoom controls together in an input form.
echo "<form method=\"post\" action=\"robotmap.php3\">\n";

# The image may be clicked to get node info or set a new center-point.
echo "  Click on the dots below to see information about the robot\n";
echo "  <br>\n";
echo "  Click elsewhere to get its x,y location.\n";
echo "  <br>\n";
if ($isadmin || TBWebCamAllowed($uid)) {
    echo "  There is a nifty <a href=webcam.php3>webcam image</a> of the";
    echo "   robots too\n";
    echo "  <br>\n";
}

if (isset($map_x) && isset($map_y)) {
#    $map_y = $max_y - $map_y;
	
    if (isset($pixels_per_meter) && $pixels_per_meter) {
	$meters_x = sprintf("%.3f", $map_x / $pixels_per_meter);
	$meters_y = sprintf("%.3f", $map_y / $pixels_per_meter);
    
	echo "Last click location was x=$meters_x, y=$meters_y meters.\n";
    }
    else {
	echo "Last click location was x=$map_x, y=$map_y pixels\n";
    }
    echo "<br>\n";
}

echo "  <input name=map type=image style=\"border: 2px solid\" ";
echo          "src=\"floormap_aux.php3?prefix=$uniqueid\" ".
              "usemap=\"#floormap\">\n";

echo "  <br>\n";

# Hidden items are all returned as page arguments when any input control
# is clicked.
echo "  <input type=\"hidden\" name=\"prefix\" value=\"$uniqueid\">\n";

# The last_* items come from a .state file with the map, from the Perl script.
if (! readfile("${prefix}.state")) {
    TBERROR("Could not read ${prefix}.state", 1);
}

echo "</form>\n";

if (count($locations)) {
    echo "<table align=center border=2 cellpadding=0 cellspacing=2>
 	  <tr><th>Node</th><th>Location</th></tr>\n";
    
    while (list($node_id, $location) = each($locations)) {
	echo "<tr><td>$node_id</td> <td>$location</td> </tr>\n";
    }
    echo "</table>\n";
}

echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
