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
		 "left join reserved as r on r.node_id=loc.node_id");

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
          </table>
          Click on the dots below to see information about the node\n";
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
          </table>
          Click on the dots below to see information about the node\n";
}

if (count($channels)) {
    echo "<br><br>
          <table align=center border=2 cellpadding=0 cellspacing=2>
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

# Image
echo "<img src=\"floormap_aux.php3?prefix=$uniqueid\" usemap=\"#floormap\">
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
