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
# One robot map right now ...
# 
$building = "MEB-ROBOTS";
$floor    = 4;

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

if (isset($fake)) {
    #
    # Just loop forever writing out some stuff.
    #
    $x1 = 100;
    $y1 = 100;

    $x2 = 700;
    $y2 = 200;

    $i = 0;
    do {
	echo "garcia1, robbie, $x1, $y1, 90.0, 700, 300, -90.0, 10, 20\n";
	echo "garcia2, mary,   $x2, $y2, 0.0,  200, 100, 90.0, 50, 60\n";
	flush();

	sleep(1);

	$x1 +=2;
	$y1 +=2;
	$x2 -=2;
	$y2 -=2;
    } while ($i < 100);
    return;
}

# Loop forever.
while (1) {
    $query_result = 
	DBQueryFatal("select loc.*,r.pid,r.eid,r.vname, ".
		     "  n.battery_voltage,n.battery_percentage, ".
		     "  n.destination_x,n.destination_y, ".
		     "  n.destination_orientation ".
		     "  from location_info as loc ".
		     "left join reserved as r on r.node_id=loc.node_id ".
		     "left join nodes as n on n.node_id=loc.node_id ".
		     "where loc.building='$building' and ".
		     "      loc.floor='$floor'");

    while ($row = mysql_fetch_array($query_result)) {
	$pname = $row["node_id"];
	$vname = $row["vname"];
	$x     = $row["loc_x"];
	$y     = $row["loc_y"];
	$or    = $row["orientation"];
	$dx    = $row["destination_x"];
	$dy    = $row["destination_y"];
	$dor   = $row["destination_orientation"];
	$bvolts= $row["battery_voltage"];
	$bper  = $row["battery_percentage"];

	if (!isset($vname))
	    $vname = $pname;
	if (!isset($or))
	    $or = "";
	if (!isset($dx)) {
	    $dx  = "";
	    $dy  = "";
	    $dor = "";
	}
	if (!isset($bvolts))
	    $bvolts = "";
	if (!isset($bper))
	    $bper = "";
	    
	echo "$pname, $vname, $x, $y, $or, $dx, $dy, $dor, $bper, $bvolts\n";
    }
    flush();
    sleep(1);
}

?>
