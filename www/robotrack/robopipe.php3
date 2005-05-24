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
	echo "garcia1 X=$x1,Y=$y1,OR=-90.0,DX=700,DY=300,DOR=-90.0\n";
	echo "garcia2 X=$x2,Y=$y2,OR=-5.66424,BATV=50.34567,BAT%=100.0\n";
	echo "garcia3 X=300,Y=300,OR=0.0,BATV=90,BAT%=95\n";
	flush();
	sleep(1);

	$x1 +=2;
	$y1 +=2;
	$x2 -=2;
	$y2 -=2;
    } while ($i < 100);
    return;
}

#
# Clean up when the remote user disconnects
#
$socket = 0;

function SPEWCLEANUP()
{
    global $socket;

    if (!$socket || !connection_aborted()) {
	exit();
    }
    fclose($socket);
    exit();
}
#ignore_user_abort(1);
set_time_limit(0);
register_shutdown_function("SPEWCLEANUP");

$socket = fsockopen("localhost", 9005);
if (!$socket) {
    TBERROR("Error opening locpiper socket - $errstr",1);
}

while (! feof($socket)) {
    $buffer = fgets($socket, 1024);
    echo $buffer;
    flush();
}
fclose($socket);

?>
