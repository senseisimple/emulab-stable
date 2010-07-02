<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

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
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node", PAGEARG_NODE);

if (! $isadmin &&
    ! $node->AccessCheck($this_user, $TB_NODEACCESS_REBOOT)) {
    USERERROR("You do not have permission to view the bootlog node", 1);
}
$node_id = $node->node_id();

#
# See if we have a bootlog recorded. 
#
$log   = null;
$stamp = null;
if ($node->BootLog($log, $stamp) || is_null($log)) {
    USERERROR("There is no bootlog for $node_id", 1);
}

header("Content-Type: text/plain");
header("Expires: Mon, 26 Jul 1997 05:00:00 GMT");
header("Cache-Control: no-cache, must-revalidate");
header("Pragma: no-cache");

echo "Bootlog reported on $stamp\n\n";
echo $log;

?>
