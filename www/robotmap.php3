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

# The image may be clicked to get node info or set a new center-point.
echo "  Click on the dots below to see information about the robot\n";
echo "  <br>\n";

echo "  <input name=map type=image style=\"border: 2px solid\" ";
echo          "src=\"floormap_aux.php3?prefix=$uniqueid\" ".
              "usemap=\"#floormap\">\n";

echo "  <br>\n";

# Hidden items are all returned as page arguments when any input control is clicked.
echo "  <input type=\"hidden\" name=\"prefix\" value=\"$uniqueid\">\n";

echo "</form>\n";
echo "</center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
