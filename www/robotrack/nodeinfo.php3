<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2005 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

#
# Only known and logged in users can watch LEDs
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

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
    DBQueryFatal("select loc.node_id,r.pid,r.eid,r.vname,n.type,nt.class ".
		 "  from location_info as loc ".
		 "left join reserved as r on r.node_id=loc.node_id ".
		 "left join nodes as n on n.node_id=loc.node_id ".
		 "left join node_types as nt on nt.type=n.type ".
		 "where loc.building='$building' and ".
		 "      loc.floor='$floor' $stamp_clause ".
		 "order by n.type,n.node_id");

while ($row = mysql_fetch_array($query_result)) {
    $pname  = $row["node_id"];
    $vname  = $row["vname"];
    $pid    = $row["pid"];
    $eid    = $row["eid"];
    $type   = $row["type"];
    $class  = $row["class"];
    $mobile = ($class == "robot" ? 1 : 0);
    # In meters.
    $size   = ($class == "robot" ? 0.27 : 0.07);
    $radius = ($class == "robot" ? 0.18 : 0.04);
    $alloc  = (isset($pid) ? 1 : 0);

    if (!isset($vname))
	$vname = $pname;
							      
    echo "$pname, $vname, $type, $alloc, $mobile, $size, $radius\n";
}

?>
