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

PAGEHEADER("Node Selector Applet");

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

if (!isset($pid) ||
    strcmp($pid, "") == 0) {
    PAGEARGERROR("You must provide a Project ID.");
}

if (!isset($eid) ||
    strcmp($eid, "") == 0) {
    PAGEARGERROR("You must provide an Experiment ID.");
}

if (!preg_match("/^[-\w]+$/", $pid)) {
    PAGEARGERROR("Invalid pid argument.");
}
if (!preg_match("/^[-\w]+$/", $eid)) {
    PAGEARGERROR("Invalid eid argument.");
}

#
# Check to make sure this is a valid PID/EID tuple.
#
if (! TBValidExperiment($pid, $eid)) {
  USERERROR("Experiment $pid/$eid is not a valid experiment.", 1);
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
# Draw the legend and some explanatory text.
#
echo "<table cellspacing=5 cellpadding=5 border=0 class=\"stealth\">
      <tr>
       <td align=\"left\" valign=\"top\" class=\"stealth\">
         <table>
           <tr><th colspan=2>Legend</th></tr>
           <tr>
             <td><img src='/autostatus-icons/redball.gif' alt=down></td>
             <td nowrap=1>Dead Node</td>
           </tr>
           <tr>
             <td><img src='/autostatus-icons/greenball.gif' alt=down></td>
             <td nowrap=1>Unassigned Node</td>
           </tr>
           <tr>
             <td><img src='/autostatus-icons/yellowball.gif' alt=down></td>
             <td nowrap=1>Assigned Node</td>
           </tr>
         </table>
       </td>
       <td class=stealth>This applet allows you to assign your virual nodes
                         to physical nodes. See below for instructions.
                         
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
$baseurl = "../floormap_aux.php3?prefix=$uniqueid";

echo "<applet name='selector' code='NodeSelect.class'
              archive='selector.jar'
              width='1025' height='1050'
              alt='You need java to run this applet'>
            <param name='floorurl' value='$baseurl'>
            <param name='uid' value='$uid'>
            <param name='auth' value='$auth'>
            <param name='ppm' value='$ppm'>
            <param name='building' value='$building'>
            <param name='floor' value='$floor'>
            <param name='pid' value='$pid'>
            <param name='eid' value='$eid'>
          </applet>\n";

echo "<br>
     <blockquote><blockquote>
     <center>
     <h3>Using the Node Selector Applet</h3>
     </center>
     <ul>
     <li> Left click over a node brings up a menu of virtual nodes. Select a
          virtual node from the menu to assign to the physical node.
     <li> Right click over a node brings up a menu that allows you to get
          information about the physical node, or cancel the current
          assignment.
     <li> Left click over the background image brings up a menu to submit your
          assignments.
     </ul>
     <blockquote><blockquote>\n";

PAGEFOOTER();
?>
