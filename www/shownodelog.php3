<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("node_defs.php");

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

#
# Standard Testbed Header
#
PAGEHEADER("Node Log");

#
# Perm check.
#
if (! ($isadmin || OPSGUY())) {
    USERERROR("You do not have permission to view node logs!", 1);
}

$node->ShowLog();

#
# New Entry option.
#
$url =CreateURL("newnodelog_form", $node);

echo "<p><center>
           Do you want to enter a log entry?
           <A href='$url'>Yes</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
