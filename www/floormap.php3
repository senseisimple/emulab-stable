<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Wireless Node Map");

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
    unset($building);
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

if (!isset($building) && !isset($pid)) {
    PAGEARGERROR();
}

#
# Run the script. It will produce two output files; an image and an areamap.
# We want to embed both of these images into the page we send back. This
# is painful!
#
# Need cleanup "handler" to make sure temp files get deleted! 
#
function CLEANUP()
{
    global $prefix;
    
    if (isset($prefix) && (connection_aborted())) {
	unlink("${prefix}.png");
	unlink("${prefix}.map");
	unlink($prefix);
    }
    exit();
}
register_shutdown_function("CLEANUP");

#
# Create a tempfile to use as a unique prefix; it is not actually used but
# serves the same purpose (the script uses ${prefix}.png and ${prefix}.map)
# 
$prefix = tempnam("/tmp", "floormap");

#
# Get the unique part to send back.
#
if (!preg_match("/^\/tmp\/([-\w]+)$/", $prefix, $matches)) {
    TBERROR("Bad tempnam: $prefix", 1);
}
$uniqueid = $matches[1];

$retval = SUEXEC($uid, "nobody", "webfloormap -o $prefix " .
		 (isset($pid) ? "-e $pid,$eid " : "") .
		 (isset($floor) ? "-f $floor " : "") .
		 (isset($building) ? "$building" : ""),
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

echo "<font size=+1>For more info on using wireless nodes, see the
     <a href='tutorial/docwrapper.php3?docname=wireless.html'>
     wireless tutorial.</a></font><br><br>\n";

echo "<center>\n";

# Legend
if (isset($pid)) {
    echo "Wireless nodes in experiment <b>".
         "<a href='showproject.php3?pid=$pid'>$pid</a>/".
         "<a href='showexp.php3?pid=$pid&eid=$eid'>$eid</a></b>\n";
    echo "<table class=nogrid align=center border=0 vpsace=8
                 cellpadding=6 cellspacing=0>
 	  <tr>
            <td align=right>Experiment Nodes</td>
            <td align=left>
                <img src='/autostatus-icons/greenball.gif' alt=Experiment>
             </td>
          </tr>
          <tr>
            <td align=right>Other Nodes</td>
            <td align=left>
                <img src='/autostatus-icons/blueball.gif' alt=Other>
             </td>
          </tr>
          <tr>
            <td align=right>Dead</td>
            <td align=left><img src='/autostatus-icons/redball.gif' alt=Dead>
              </td>
          </tr>
          </table>
          Click on the dots below to see information about the node\n";
}
else {
    echo "<table class=nogrid align=center border=0 vpsace=5
                 cellpadding=6 cellspacing=0>
 	  <tr>
            <td align=right>Free</td>
            <td align=left>
                <img src='/autostatus-icons/greenball.gif' alt=Free>
             </td>
          </tr>
          <tr>
            <td align=right>Reserved</td>
            <td align=left>
                <img src='/autostatus-icons/blueball.gif' alt=reserved>
             </td>
          </tr>
          <tr>
            <td align=right>Dead</td>
            <td align=left><img src='/autostatus-icons/redball.gif' alt=Dead>
              </td>
          </tr>
          </table>
          Click on the dots below to see information about the node\n";
}

# Image
echo "<img src=\"floormap_aux.php3?prefix=$uniqueid\" usemap=\"#floormap\">
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
