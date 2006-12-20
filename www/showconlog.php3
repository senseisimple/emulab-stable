<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Console Log for $node_id");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure a valid nodeid.
#
if (isset($node_id) && strcmp($node_id, "")) {
    if (! TBvalid_node_id($node_id)) {
	PAGEARGERROR("Illegal characters in node_id!");
    }
    
    if (! TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }

    if (!$isadmin &&
	!TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_READINFO)) {
        USERERROR("You do not have permission to view the console log ".
		  "for $node_id!", 1);
    }
}
else {
    PAGEARGERROR("Must specify a node ID!");
}

#
# Look for linecount argument
#
if (isset($linecount) && $linecount != "") {
    if (! TBvalid_integer($linecount)) {
	PAGEARGERROR("Illegal characters in linecount!");
    }
    $optarg = "&linecount=$linecount";
}
else {
    $optarg = "";
}

echo "<font size=+2>".
     "Node <a href=shownode.php3?node_id=$node_id><b>$node_id</b></font></a>";
echo "<br /><br />\n";

echo "<center>
      <iframe src=spewconlog.php3?node_id=${node_id}${optarg}
              width=90% height=500 scrolling=auto frameborder=1>
      Your user agent does not support frames or is currently configured
      not to display frames. However, you may visit
      <A href=spewconlog.php3?node_id=${node_id}${optarg}>the
      log file directly.</A>
      </iframe></center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
