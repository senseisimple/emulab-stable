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
# Only admin people for now.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have permission to view this page!", 1);
}

# Careful with this local variable
unset($prefix);

#
# Verify page arguments.
# 
if (!isset($building) ||
    strcmp($building, "") == 0) {
    PAGEARGERROR("Must provide a building and an optional floor!");
}
# Sanitize for the shell.
if (!preg_match("/^[-\w]+$/", $building)) {
    PAGEARGERROR("Invalid building argument.");
}

# Optional floor argument. Sanitize for the shell.
if (isset($floor) && !preg_match("/^[-\w]+$/", $floor)) {
    PAGEARGERROR("Invalid floor argument.");
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

$retval = SUEXEC("nobody", "nobody", "webfloormap -o $prefix " .
		 (isset($floor) ? "-f $floor " : "") . "$building",
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

# And the img ...
echo "<center>
      <table class=nogrid align=center border=0 cellpadding=6 cellspacing=0>
      <tr>
         <td align=right>Free</td>
         <td align=left><img src='/autostatus-icons/greenball.gif' alt=Free>
          </td>
      </tr>
      <tr>
         <td align=right>Reserved</td>
         <td align=left><img src='/autostatus-icons/blueball.gif' alt=reserved>
          </td>
      </tr>
      <tr>
         <td align=right>Dead</td>
         <td align=left><img src='/autostatus-icons/redball.gif' alt=Free>
          </td>
      </tr>
      </table>
      <br><br>
      <img src=\"floormap_aux.php3?prefix=$uniqueid\" usemap=\"#floormap\">
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
