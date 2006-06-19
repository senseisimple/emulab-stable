<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Wireless PC Map");

#
# Only logged in people at the moment; might open up at some point.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

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
    $building = "MEB";
    unset($floor);
}

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

# THIS is ugly...  Value= is not passed by IE on <input type=image>, so we wrap the
# scale value into the button name.  Hence we will see coordinates of scale_1_x
# scale_2_x, etc. when the buttons are pressed.  Decode to just a $scale value here.
if (isset($scale_0_x) && $scale_0_x != "") {
    $scale = 0;
}
if (isset($scale_1_x) && $scale_1_x != "") {
    $scale = 1;
}
elseif (isset($scale_2_x) && $scale_2_x != "") {
    $scale = 2;
}
elseif (isset($scale_3_x) && $scale_3_x != "") {
    $scale = 3;
}
elseif (isset($scale_4_x) && $scale_4_x != "") {
    $scale = 4;
}
elseif (isset($scale_5_x) && $scale_5_x != "") {
    $scale = 5;
}

#
# Verify numeric scale and centering arguments.
#
if (isset($scale) && $scale !== "") {  # "0" is a valid value for $scale.
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $scale)) {
	PAGEARGERROR("Invalid scale argument.");
    }
}
else {
    unset($scale);
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
# We need the previous scale and click coords, and offsets to interpret new click coords!
#
if (isset($last_scale) && $last_scale !== "") {  # "0" is a valid value.
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_scale)) {
	PAGEARGERROR("Invalid last_scale argument.");
    }
}
else {
    unset($last_scale);
}
if (isset($last_x) && $last_x != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_x)) {
	PAGEARGERROR("Invalid last_x argument.");
    }
}
else {
    unset($last_x);
}
if (isset($last_y) && $last_y != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_y)) {
	PAGEARGERROR("Invalid last_y argument.");
    }
}
else {
    unset($last_y);
}
if (isset($last_x_off) && $last_x_off != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_x_off)) {
	PAGEARGERROR("Invalid last_x_off argument.");
    }
}
else {
    unset($last_x_off);
}
if (isset($last_y_off) && $last_y_off != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_y_off)) {
	PAGEARGERROR("Invalid last_y_off argument.");
    }
}
else {
    unset($last_y_off);
}
if (isset($last_notitles) && $last_notitles != "") {
    # Sanitize for the shell.
    if (!preg_match("/^[0-9]+$/", $last_notitles)) {
	PAGEARGERROR("Invalid last_notitles argument.");
    }
}
else {
    unset($last_notitles);
}
if (isset($last_ghost) && $last_ghost !== "") {  # "0" is a valid value.
    # Sanitize for the shell.
    if (!preg_match("/^[01]$/", $last_ghost)) {
	PAGEARGERROR("Invalid last_ghost argument.");
    }
}
else {
    unset($last_ghost);
}

# With image checkboxes, we have two names: ghost_on and ghost_off.
if (isset($ghost_on_x)) {
    $ghost = 1;
}
elseif (isset($ghost_off_x)) {
    $ghost = 0;
}
else {
    # Default the ghosting checkbox state to checked the first time through.
    $ghost = (isset($last_ghost) ? $last_ghost : 1);	
}

#
# Figure out what channels are in use for the current building. We only
# have one building (MEB) at the moment, so this is quite easy.
# Also determine what the free,reserved,dead counts are. 
#
$channels   = array();
$nodecounts = array();
$nodes      = array();

$nodecounts["free"]     = 0;
$nodecounts["reserved"] = 0;
$nodecounts["dead"]     = 0;

$query_result =
    DBQueryFatal("select loc.*,s.capval,r.pid,r.eid from location_info as loc ".
		 "left join interface_settings as s on ".
		 "     s.node_id=loc.node_id and s.capkey='channel' ".
		 "left join reserved as r on r.node_id=loc.node_id ".
		 "where loc.building='$building' ".
		 (isset($floor) ? "and loc.floor='$floor'" : ""));

while ($row = mysql_fetch_array($query_result)) {
    $channel   = $row["capval"];
    $node_id   = $row["node_id"];
    $locfloor  = $row["floor"];
    $rpid      = $row["pid"];
    $reid      = $row["eid"];

    #
    # The above query to get the channels will give us multiple rows
    # if there are multiple interface cards in use; do not want to
    # count those twice.
    # 
    if (!isset($nodes[$node_id])) {
	$nodes[$node_id] = 1;
	
	if ((!isset($pid) && !(isset($rpid))) ||
	    (isset($pid) && isset($rpid) && $pid == $rpid)) {
	    $nodecounts["free"]++;
	}
	elseif ($rpid == $NODEDEAD_PID && $reid == $NODEDEAD_EID) {
	    $nodecounts["dead"]++;
	}
	else {
	    $nodecounts["reserved"]++;
	}
    }

    # Make sure an empty list is displayed if nothing allocated on a floor.
    if (!isset($channels[$locfloor])) {
	$channels[$locfloor] = array();
    }

    if (!isset($channel))
	continue;

    $channels[$locfloor][$channel] = $channel;
}

#
# Run the Perl script. It will produce three output files; image, areamap, and state.
# We want to embed all of these images into the page we send back. This is painful!
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

$perl_args = "-o $prefix " .
	     # From clicking on a zoom button.
	     (isset($scale) ? "-s $scale " : "") .

	     # From clicking on a map image.
	     (isset($map_x) ? "-c $map_x,$map_y " : "") .

	     # Previous image state info came from an included .state
	     # file, previously output by the Perl script along with the map.
	     (isset($last_scale) ? "-S $last_scale " : "") .
	     (isset($last_x) ? "-C $last_x,$last_y " : "") .
	     (isset($last_x_off) ? "-O $last_x_off,$last_y_off " : "") .
	     (isset($last_notitles) && $last_notitles ? "-T " : "") .
	     ($ghost ? "-g " : "") .

	     (isset($pid) ? "-e $pid,$eid " : "") .
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

echo "<font size=+1>For more info on using wireless nodes, see the
     <a href='tutorial/docwrapper.php3?docname=wireless.html'>
     wireless tutorial.</a></font>\n";

echo "<center>\n";

# Consolidate layout by pulling the info and control tables into a horizontal row.
echo "<table style=\"border-width: 0; background-color: transparent\">
      <tr> <td>\n";

# Legend
if (isset($pid)) {
    echo "Wireless nodes in experiment <b>".
         "<a href='showproject.php3?pid=$pid'>$pid</a>/".
         "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b>\n";

    echo "<table align=center border=2 cellpadding=5 cellspacing=2>
 	  <tr>
            <td align=right>
                <img src='/autostatus-icons/greenball.gif' alt=Experiment>
                <b>Experiment</b></td>
            <td align=left> &nbsp; " . $nodecounts["free"] . "</td>
          </tr>
          <tr>
            <td align=right>
                <img src='/autostatus-icons/blueball.gif' alt=Other>
                <b>Other Nodes</b></td>
            <td align=left> &nbsp; " . $nodecounts["reserved"] . "</td>
          </tr>
          <tr>
            <td align=right>
                <img src='/autostatus-icons/redball.gif' alt=Dead>
                <b>Dead</b></td>
            <td align=left> &nbsp; " . $nodecounts["dead"] . "</td>
          </tr>
          </table>\n";
}
else {
    echo "<table align=center border=2 cellpadding=5 cellspacing=2>
 	  <tr>
            <td align=right>
                <img src='/autostatus-icons/greenball.gif' alt=Free>
                <b>Free</b></td>
            <td align=left> &nbsp; " . $nodecounts["free"] . "</td>
          </tr>
          <tr>
            <td align=right>
                <img src='/autostatus-icons/blueball.gif' alt=reserved>
                <b>Reserved</b></td>
            <td align=left> &nbsp; " . $nodecounts["reserved"] . "</td>
          </tr>
          <tr>
            <td align=right>
                <img src='/autostatus-icons/redball.gif' alt=Dead>
                <b>Dead</b></td>
            <td align=left> &nbsp; " . $nodecounts["dead"] . "</td>
          </tr>
          </table>\n";
}

echo "</td> <td style=\" background-color: transparent\"> &nbsp; &nbsp; </td> <td>\n";

# Wrap the image and zoom controls together in an input form.
echo "<form method=\"get\" action=\"floormap.php3#zoom\">\n";

# Zoom controls may be clicked to set a new scale.  Otherwise, it persists.
$curr_scale = (isset($scale) ? $scale : (isset($last_scale) ? $last_scale : 1));
echo "  <a name=zoom></a>\n";
function zoom_btns($curr_scale) {
echo "  <table align=\"center\" border=\"2\" cellpadding=\"0\" cellspacing=\"2\">\n";
echo "    <tbody>\n";
echo "      <tr>\n";
# Wrap the scale into the button name.  Value= is not passed by IE.
echo "        <td><input type=\"image\" src=\"floormap/btn_zoom_out.jpg\"\n";
echo "                   name=\"scale_" . max($curr_scale-1,0) . "\"><br></td>\n";
for ($i = 0; $i <= 5; $i++) {
    $img = "btn_scale_" . $i . "_" . ($curr_scale==$i?"brt":"dim") . ".jpg";
    echo "        <td><input type=\"image\" src=\"floormap/$img\"\n";
    echo "                   name=\"scale_$i\"><br></td>\n";
}
echo "        <td><input type=\"image\" src=\"floormap/btn_zoom_in.jpg\"\n";
echo "                   name=\"scale_" . min($curr_scale+1,5) . "\"><br></td>\n";
echo "      </tr>\n";
echo "    </tbody>\n";
echo "  </table>\n";
}
zoom_btns($curr_scale);  # Two copies of the zoom buttons bracket the image.

echo "</td> <td style=\" background-color: transparent\"> &nbsp; &nbsp; </td> <td>\n";

if (count($channels)) {
    echo "<table align=center border=2 cellpadding=0 cellspacing=2>
 	  <tr><th>Floor</th><th>Channels in Use</th></tr>\n";
    
    while (list($floor, $chanlist) = each($channels)) {
	echo "<tr><td>$floor</td>\n";
	echo "    <td>";

	echo implode(",", array_keys($chanlist));

	echo "    </td>
              </tr>\n";
    }
    echo "</table>\n";
}

echo "</td> </tr> </table>\n";

# The image may be clicked to get node info or set a new center-point.
echo "  Click on the dots below to see information about the node.\n";
echo "  <br>\n";
echo "  Click elsewhere on the map to set the center point for a zoomed-in view.\n";
echo "  <br>\n";
echo "  Check out the nifty new <a href=robotrack/selector.php3?building=MEB>";
echo "Java Applet</a> for selecting wireless nodes\n";
echo "  <br>\n";
# Couldn't get JavaScript submit on checkbox to be portable to IE with map in form.
# Just use image buttons to show and invert the checkbox state.  That works.
if ($ghost) {
    echo " <input type=image src=\"floormap/cb_checked.gif\" name=ghost_off>\n";
}
else {
    echo " <input type=image src=\"floormap/cb_unchecked.gif\" name=ghost_on>\n";
}
echo "  Show nodes on other floors as hollow dots.\n";
echo "  <br>\n";
echo "  <input name=map type=image style=\"border: 2px solid\" ";
echo          "src=\"floormap_aux.php3?prefix=$uniqueid\" usemap=\"#floormap\">\n";
echo "  <br>\n";
zoom_btns($curr_scale);

# Hidden items are all returned as page arguments when any input control is clicked.
echo "  <input type=\"hidden\" name=\"prefix\" value=\"$uniqueid\">\n";

# The last_* items come from a .state file with the map, from the Perl script.
if (! readfile("${prefix}.state")) {
    TBERROR("Could not read ${prefix}.state", 1);
}

echo "</form>\n";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
