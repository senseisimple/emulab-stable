<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$uid = GETLOGIN();
LOGGEDINORDIE($uid);

PAGEHEADER("Real Time Robot Tracking Applet");

#
# One robot map right now ...
# 
$building = "MEB-ROBOTS";
$floor    = 4;
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
                         as they move around. The table at the bottom
                         shows the current position (x, y, orientation),
                         the destination position,
                         and the battery level (percentage and voltage).
                         The shaded areas (blue border) are <em>exclusion</em>
                         zones where robots
                         are not allowed to go.
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

$perl_args = "-o $prefix -t -z -n -x -y -f $floor $building";

$retval = SUEXEC($uid, "nobody", "webfloormap $perl_args",
		 SUEXEC_ACTION_IGNORE);

if ($retval) {
    SUEXECERROR(SUEXEC_ACTION_USERERROR);
    # Never returns.
    die("");
}

$auth    = $HTTP_COOKIE_VARS[$TBAUTHCOOKIE];
$pipeurl = "robopipe.php3?foo=bar";
$baseurl = "../floormap_aux.php3?prefix=$uniqueid";

# Temp for debugging.
if (isset($fake))
     $pipeurl .= "&fake=yes";
     
echo "<applet code='RoboTrack.class'
              archive='tracker.jar'
              width='900' height='600'
              alt='You need java to run this applet'>
            <param name='pipeurl' value='$pipeurl'>
            <param name='baseurl' value='$baseurl'>
            <param name='uid' value='$uid'>
            <param name='auth' value='$auth'>
            <param name='ppm' value='$ppm'>
          </applet>\n";

PAGEFOOTER();
?>
