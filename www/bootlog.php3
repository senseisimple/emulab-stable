<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# No PAGEHEADER since we spit out plain text.
# 

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
    if (! TBValidNodeName($node_id)) {
	USERERROR("$node_id is not a valid node name!", 1);
    }

    if (!$isadmin &&
	!TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_REBOOT)) {
        USERERROR("You do not have permission to view bootlog for $node_id!", 1);
    }
}
else {
    USERERROR("Must specify a node ID!", 1);
}

#
# See if we have a bootlog recorded. 
#
$query_result =
    DBQueryFatal("select * from node_bootlogs where node_id='$node_id'");

if (mysql_num_rows($query_result) == 0) {
    USERERROR("There is no bootlog for $node_id", 1);
}
$row = mysql_fetch_array($query_result);

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");

echo "Bootlog reported on ". $row["bootlog_timestamp"] . "\n\n";
echo $row["bootlog"];

?>
