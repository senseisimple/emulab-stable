<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

$optargs = OptionalPageArguments("building",    PAGEARG_STRING,
				 "floor",       PAGEARG_STRING,
				 "experiment",  PAGEARG_EXPERIMENT,
				 "camheight",   PAGEARG_INTEGER,
				 "camwidth",    PAGEARG_INTEGER,
				 "camfps",      PAGEARG_INTEGER,
				 "camera",      PAGEARG_INTEGER,
				 "withwebcams", PAGEARG_BOOLEAN);

#
# Optional pid,eid. Without a building/floor, show all the nodes for the
# experiment in all buildings/floors. Without pid,eid show all wireless
# nodes in the specified building/floor.
#
if (isset($experiment)) {
    if (!$experiment->AccessCheck($this_user, $TB_EXPT_READINFO)) {
	USERERROR("You do not have permission to view this experiment!", 1);
    }
    $pid = $experiment->pid();
    $eid = $experiment->eid();
}
else {
    #
    # Else, we need to find whatever experiment is running. What if there
    # is more then one? Good question; I do not have a plan for that yet!
    #
    $query_result =
	DBQueryFatal("select pid,eid from experiments ".
		     "where locpiper_pid!=0 and locpiper_port!=0");
    if (mysql_num_rows($query_result)) {
	$row = mysql_fetch_array($query_result);
	$pid = $row["pid"];
	$eid = $row["eid"];
    }
    else {
	unset($pid);
	unset($eid);
    }
}

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
$ppm      = 1;

#
# Grab pixel_per_meters for above map.
#
$query_result =
    DBQueryFatal("select pixels_per_meter from floorimages ".
		 "where building='$building' and floor='$floor'");
if (mysql_num_rows($query_result)) {
    $row = mysql_fetch_array($query_result);
    $ppm = $row["pixels_per_meter"];
}
else {
    USERERROR("No such building/floor $building/$floor", 1);
}

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

#
# If adding in the webcams, get that stuff too.
#
$webcams = array();

if (isset($withwebcams) && $withwebcams) {
    if (isset($camera) && TBvalid_integer($camera)) {
	$query_result = DBQueryFatal("select * from webcams ".
				     "where id='$camera'");
    }
    else 
	$query_result = DBQueryFatal("select * from webcams");

    while ($row = mysql_fetch_array($query_result)) {
	$id      = $row["id"];
	$camurl  = "../webcamimg.php3?webcamid=${id}&applet=1&fromtracker=1";
	$webcams[] = $camurl;
    }
}

#
# Standard Testbed Header
#
PAGEHEADER("Real Time Robot Tracking Applet");

#
# Draw the legend and some explanatory text.
#
echo "<table cellspacing=5 cellpadding=5 border=0 class=\"stealth\">
      <tr>
       <td align=\"left\" valign=\"top\" class=\"stealth\">
         <table>
           <tr><th colspan=2>Legend</th></tr>
           <tr>
             <td><img src=\"../floormap/map_legend_node.gif\"></td>
             <td nowrap=1>Robot Actual Position</td>
           </tr>
           <tr>
             <td><img src=\"../floormap/map_legend_node_dst.gif\"></td>
             <td nowrap=1>Robot Destination Position</td>
           </tr>
         </table>
       </td>
       <td class=stealth>This applet allows you to view the robots
                         as they move around, as well as move the robots
                         with drag and drop. The table at the bottom
                         shows the current position (x, y, orientation),
                         the destination position,
                         and the battery level (percentage and voltage).
                         The shaded areas are <em>exclusion</em>
                         zones where robots are not allowed to go. You can
                         <b>right click</b> on a robot to bring up its
                         info page. See below for instructions on how to
                         use the <b>drag and drop</b> features. If you are on
                         a <b>high speed</b> connection, you can try the
                         <a href=${REQUEST_URI}&withwebcams=1>WithWebCams</a>
                         option.
        </td>
      </tr>
      </table><hr>\n";

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

$perl_args = "-o $prefix -t -z -n -x -v -y -f $floor $building";

$retval = SUEXEC($uid, "nobody", "webfloormap $perl_args",
		 SUEXEC_ACTION_IGNORE);

if ($retval) {
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    # Never returns.
    die("");
}

$auth    = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
$pipeurl = "robopipe.php3?building=$building&floor=$floor";
$baseurl = "../floormap_aux.php3?prefix=$uniqueid";

# Temp for debugging.
if (isset($fake))
    $pipeurl .= "&fake=yes";
if (isset($pid) && isset($eid)) {
    $pipeurl .= "&pid=$pid&eid=$eid";
}
     
echo "<applet name='tracker' code='RoboTrack.class'
              archive='tracker.jar'
              width='1025' height='1150'
              alt='You need java to run this applet'>
            <param name='pipeurl' value='$pipeurl'>
            <param name='floorurl' value='$baseurl'>
            <param name='uid' value='$uid'>
            <param name='auth' value='$auth'>
            <param name='ppm' value='$ppm'>
            <param name='building' value='$building'>
            <param name='floor' value='$floor'>";
if (count($webcams)) {
    echo "<param name='WebCamHeight' value=$camheight>
          <param name='WebCamWidth' value=$camwidth>
          <param name='WebCamFPS' value=$camfps>\n";
    
    $camcount = count($webcams);
    $x = 400;
    $y = 460;
    
    echo "<param name='WebCamCount' value='$camcount'>";

    for ($i = 0; $i < $camcount; $i++) {
	$camurl = $webcams[$i];

	echo "<param name='WebCam${i}' value='$camurl'>
              <param name='WebCam${i}XY' value='$x,$y'>";

	$x += ($camwidth + 20);

	if ($x > 700) {
	    $x = 400;
	    $y = $y + ($camheight + 20);
	}
    }
}
echo "</applet>\n";

echo "<br>
     <blockquote><blockquote>
     <center>
     <h3>Using the Robot Tracker Applet</h3>
     </center>
     <ul>
     <li> Right Click over a robot or mote will pop up a browser window 
          showing information about the robot/mote.
     <li> Left click and drag over a (stationary) robot allows you to setup a
          move to a new position. Remember to edit the orientation in the
          table, if needed. Cancel by moving back to its original position. 
     <li> Left click over the background image brings up a menu to submit your 
          moves (start the robots on their way), or cancel the moves.
     <li> Only one move per robot at a time.
     <li> To change just the orientation (no drag), edit the destination
          orientation column in the table.
     <li> A robot destination must not overlap an obstacle (shaded area,
          blue border), and it must be fully within the field of view of at
          least one camera (orange boxes).
     <li> A robot destination must not overlap with another robot (its final
          destination). Because of the sensors on the robots, the closest a
          robot can come to anything else is 23cm.
     </ul>
     <blockquote><blockquote>\n";

PAGEFOOTER();
?>
