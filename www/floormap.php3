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
# Run the script. It will produce two output files; an image and an areamap.
# We want to embed both of these images into the page we send back. This
# is painful!
#
# Need cleanup "handler" to make sure temp files get deleted! 
#
function CLEANUP()
{
    if (isset($prefix)) {
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
$prefix = tempnam("/tmp", "floomap");

$retval = SUEXEC("nobody", "nobody", "webfloormap -o $prefix MEB",
		 SUEXEC_ACTION_IGNORE);

if ($retval) {
    SUEXECERROR(SUEXEC_ACTION_DIE);
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

#
# Convert the image binary data into a javascript variable.
#
$fp = fopen("${prefix}.png", "r");
if (! $fp) {
    TBERROR("Could not open ${prefix}.png", 1);
}

echo "<script language=JavaScript>
      <!--
      function binary(d) {
        var o = ''; 
        for (var i=0; i<d.length; i=i+2)
            o+=String.fromCharCode(eval('0x' +
                                        (d.substring(i,i+2)).toString(16)));
        return o;
      } 
      var imagedata = binary('";

# Read image data and spit out hex representation of it.
do {
    $data = fread($fp, 8192);
    if (strlen($data) == 0) {
        break;
    }
    echo bin2hex($data);
} while (true);
fclose($fp);

echo "');
      //-->
      </script>\n";

# And the img ...
echo "<center>
      <img src=\"javascript:imagedata\" usemap=\"#floormap\">
      </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
