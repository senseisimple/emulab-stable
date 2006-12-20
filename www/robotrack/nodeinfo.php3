<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2006 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

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

# Get the robot information. Actually, this includes motes.
$query_result =
    DBQueryFatal("select loc.node_id,r.pid,r.eid,r.vname,n.type,nt.class, ".
		 "       loc.loc_x,loc.loc_y,loc.loc_z,loc.orientation ".
		 "  from location_info as loc ".
		 "left join reserved as r on r.node_id=loc.node_id ".
		 "left join nodes as n on n.node_id=loc.node_id ".
		 "left join node_types as nt on nt.type=n.type ".
		 "where loc.building='$building' and ".
		 "      loc.floor='$floor' ".
		 "order by n.type,n.node_id");

while ($row = mysql_fetch_array($query_result)) {
    $pname  = $row["node_id"];
    $vname  = $row["vname"];
    $pid    = $row["pid"];
    $eid    = $row["eid"];
    $type   = $row["type"];
    $class  = $row["class"];
    $loc_x  = $row["loc_x"];
    $loc_y  = $row["loc_y"];
    $loc_z  = $row["loc_z"];
    $or     = $row["orientation"];
    $mobile = ($class == "robot" ? 1 : 0);
    # In meters.
    if ($class == "robot") {
	$size   = 0.27;
	$radius = 0.18;
    }
    elseif ($class == "pc" || $class == "pcwireless") {
	$size   = 1.0;
	$radius = 0.5;
    }
    else {
	$size   = 0.07;
	$radius = 0.04;
    }
    $alloc  = 0;
    $dead   = 0;
    if (isset($pid)) {
	$alloc = 1;
	if ($pid == $NODEDEAD_PID && $eid == $NODEDEAD_EID) {
	    $dead = 1;
	}
    }
    if (!isset($vname))
	$vname = $pname;
    if (!isset($loc_z))
	$loc_z = "";
    if (!isset($or))
	$or = "";
							      
    echo "$pname, ";
    if (!isset($selector)) {
	echo "$vname, ";
    }
    echo "$type, $alloc, $dead, $mobile, $size, $radius, ";
    echo "$loc_x, $loc_y, $loc_z, $or";
    echo "\n";
}

?>
