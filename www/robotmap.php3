<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004-2007 University of Utah and the Flux Group.
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
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page args
#
$optargs = OptionalPageArguments("experiment",    PAGEARG_EXPERIMENT,
				 "building",      PAGEARG_STRING,
				 "floor",         PAGEARG_STRING,
				 "prefix",        PAGEARG_STRING,
				 "map_x",         PAGEARG_INTEGER,
				 "map_y",         PAGEARG_INTEGER,
				 "show_cameras",  PAGEARG_STRING,
				 "show_exclusion",PAGEARG_STRING);

# Careful with this local variable
unset($prefix);

#
# Verify page arguments. First allow user to optionally specify building/floor.
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

#
# Optional pid,eid. Without a building/floor, show all the nodes for the
# experiment in all buildings/floors. Without pid,eid show all wireless
# nodes in the specified building/floor.
#
if (isset($experiment)) {
    $pid = $experiment->pid();
    $eid = $experiment->eid();
    if (! $experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view robotmaps ".
		  "for experiment $pid/$eid!", 1);
    }
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
$vnames = array();

$query_result =
    DBQueryFatal("select loc.*,r.vname,r.pid,r.eid from location_info as loc ".
		 "left join reserved as r on r.node_id=loc.node_id ".
		 "where loc.floor='$floor' and loc.building='$building'");

while ($row = mysql_fetch_array($query_result)) {
    $node_id   = $row["node_id"];
    $loc_x     = $row["loc_x"];
    $loc_y     = $row["loc_y"];
    $loc_z     = $row["loc_z"];
    $orient    = $row["orientation"];

    if ((isset($experiment) &&
	 $pid == $row["pid"] &&
	 $eid == $row["eid"]) &&
	 isset($row["vname"])) {
	$vnames[$node_id] = $row["vname"];
    }

    if (isset($pixels_per_meter) && $pixels_per_meter) {
	$meters_x = sprintf("%.2f", $loc_x / $pixels_per_meter);
	$meters_y = sprintf("%.2f", $loc_y / $pixels_per_meter);

	$locations[$node_id] = "x=$meters_x, y=$meters_y meters";
    }
    else {
	$locations[$node_id] = "x=$loc_x, y=$loc_y pixels";
    }
    if (isset($loc_z)) {
	$locations[$node_id] .= ", z=" . sprintf("%.2f", $loc_z) . " meters";
    }
    if (isset($orient)) {
	$locations[$node_id] .= ", o=" . sprintf("%.1f", $orient) . "&#176;";
    }
}

$event_time_start = array();
if (isset($experiment)) {
    $query_result =
	DBQueryFatal("SELECT parent,arguments FROM eventlist WHERE " .
		     "pid='$pid' and eid='$eid' and objecttype=3 and ".
		     "eventtype=1 ORDER BY parent");
    if (mysql_num_rows($query_result)) {
	while ($row = mysql_fetch_array($query_result)) {
	    $event_time_start[$row["parent"]] = $row["arguments"];
	}
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

$perl_args = ("-o $prefix -t -z -n " .
	      # From clicking on a map image.
	      ((isset($show_cameras) &&
		strcmp($show_cameras, "Yep") == 0) ? "-v " : "") .
	      ((isset($show_exclusion) &&
		strcmp($show_exclusion, "Yep") == 0) ? "-x " : "") .
	      (isset($map_x) ? "-c $map_x,$map_y " : "") .
	      (isset($floor) ? "-f $floor " : "") .
	      (isset($experiment) ? "-e $pid,$eid " : "") .
	      (isset($building) ? "$building" : ""));  # Building arg must be last!

if (0) {    ### Put the Perl script args into the page when debugging.
    echo "\$btfv/floormap -d $perl_args\n";
}
$retval = SUEXEC($uid, "nobody", "webfloormap $perl_args",
		 SUEXEC_ACTION_IGNORE);

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

if (isset($experiment)) {
    echo "<font size=+2>Robots in experiment <b>".
         "<a href='showproject.php3?pid=$pid'>$pid</a>/".
         "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b> </font>\n";
    echo "<br /><br />\n";
}

echo "<center>\n";

# Wrap the image and zoom controls together in an input form.
echo "<form method=post action='robotmap.php3" .
       ((isset($building) && isset($floor)) ?
	"?building=${building}&floor=${floor}" : "") . "'>";

echo "Click on the image to get its X,Y coordinates<br>\n";
# The image may be clicked to get node info or set a new center-point.
if ($isadmin || $this_user->WebCamAllowed()) {
    echo "  <a href=webcam.php3>Webcam View</a> (Updated in real time)";
    echo "  <br>\n";
}
echo "  <a href=robotrack/robotrack.php3?building=${building}&floor=${floor}>".
        "Track robots in real time</a>";
echo "  <a href=robotrack/robotrack.php3?building=${building}&floor=${floor}&withwebcams=1>".
        " (with webcams)</a>";
echo "  <br>\n";

if (isset($map_x) && isset($map_y)) {
    if (isset($pixels_per_meter) && $pixels_per_meter) {
	$meters_x = sprintf("%.3f", $map_x / $pixels_per_meter);
	$meters_y = sprintf("%.3f", $map_y / $pixels_per_meter);
    
	echo "Last click location was x=<b>$meters_x</b>, y=<b>$meters_y</b> meters.\n";
    }
    else {
	echo "Last click location was x=$map_x, y=$map_y pixels\n";
    }
    echo "<br>\n";
}

if (isset($experiment)) {
    $current_time = time();
    foreach ($event_time_start as $key => $value) {
	$event_time_elapsed = $current_time - $value;
	if ($key == "") {
	    echo "Elapsed event time: ";
	}
	else {
	    echo "Elapsed time for timeline '$key': ";
	}
	$minutes = floor($event_time_elapsed / 60);
	$seconds = $event_time_elapsed % 60;
	echo "<b>" . $minutes . "m " . $seconds . "s </b>(<b>" .
		sprintf("%.1f", $event_time_elapsed) . "s</b>)<br>";
    }
    if (empty($event_time_start)) {
	echo "<i><b>Elapsed event time: Not started yet</b></i>";
    }
}

echo "  <input name=map type=image style=\"border: 2px solid\" ";
echo          "src=\"floormap_aux.php3?prefix=$uniqueid\" ".
              "usemap=\"#floormap\">\n";

echo "  <br>\n";

# Hidden items are all returned as page arguments when any input control
# is clicked.
echo "  <input type=\"hidden\" name=\"prefix\" value=\"$uniqueid\">\n";

if (isset($pid)) {
    echo "  <input type=\"hidden\" name=\"pid\" value=\"$pid\">\n";
    echo "  <input type=\"hidden\" name=\"eid\" value=\"$eid\">\n";
}

# The last_* items come from a .state file with the map, from the Perl script.
if (! readfile("${prefix}.state")) {
    TBERROR("Could not read ${prefix}.state", 1);
}

echo "<br>
      <table cellspacing=5 cellpadding=5 border=0 class=\"stealth\"
             bgcolor=\"#ffffff\">
      <tr>";

echo "<td align=\"left\" valign=\"top\" class=\"stealth\">
      <table>
      <tr><th colspan=2>Legend</th></tr>
      <tr>
          <td><img src=\"floormap/map_legend_node.gif\"></td>
          <td>Robot Actual Position<br>
              <font size=\"-1\">(Click dots for more information)</font></td>
      </tr>
      <tr>
          <td><img src=\"floormap/map_legend_node_dst.gif\"></td>
          <td>Robot Destination Position</td>
      </tr>
      </table>
      </td>";

if (count($locations)) {
    echo "<td align=\"left\" valign=\"top\" class=\"stealth\">";
    echo "<table align=center border=2 cellpadding=0 cellspacing=2>
 	  <tr><th>Node</th><th>Location</th></tr>\n";
    
    while (list($node_id, $location) = each($locations)) {
	if ((!isset($pid) && !isset($eid)) || isset($vnames[$node_id])) {
	    if (isset($vnames[$node_id]))
		$name = $vnames[$node_id];
	    else
		$name = $node_id;
	    echo "<tr>
              <td><a href=\"shownode.php3?node_id=$node_id\">$name</a></td>
              <td>$location</td> </tr>\n";
	}
    }
    echo "</table></td>\n";
}

if (isset($show_cameras) &&
    strcmp($show_cameras, "Yep") == 0) {
	$cam_checked = "checked";
}
else {
	$cam_checked = "";
}

if (isset($show_exclusion) &&
    strcmp($show_exclusion, "Yep") == 0) {
	$excl_checked = "checked";
}
else {
	$excl_checked = "";
}

echo "<td align=\"left\" valign=\"top\" class=\"stealth\">
      <table>
      <tr><th>Display Options</th></tr>
      <tr>
          <td><input type=checkbox
                     name=show_cameras
                     value=Yep
                     $cam_checked>Show <a
 href=\"$WIKIDOCURL/wireless#VISION\">Tracking
 Camera</a> Bounds</input></td>
      </tr>
      <tr>
          <td><input type=checkbox
                     name=show_exclusion
                     value=Yep
                     $excl_checked>Show <a
 href=\"$WIKIDOCURL/wireless#VISION\">Exclusion
 Zones</a></input></td>
      </tr>
      <tr><td colspan=2 align=center><input type=submit value=Update></td></tr>
      </table>
      </td>";

echo "</tr></table>\n";

echo "</form>\n";

echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
