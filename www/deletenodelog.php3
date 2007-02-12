<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include_once("node_defs.php");

#
# Standard Testbed Header
#
PAGEHEADER("Delete a Node Log Entry");

#
# Only known and logged in users can end experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("node", PAGEARG_NODE,
				 "log_id", PAGEARG_STRING);
$optargs = OptionalPageArguments("canceled", PAGEARG_BOOLEAN,
				 "confirmed", PAGEARG_BOOLEAN);

#
# Only Admins can delete log entries.
#
if (! ($isadmin || OPSGUY())) {
    USERERROR("You do not have permission to delete log entries!", 1);
}

# Need these below
$node_id = $node->node_id();

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if (isset($canceled) && $canceled) {
    echo "<center><h2><br>
          Log Entry Deletion canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!isset($confirmed)) {
    echo "<center><h2><br>
          Are you sure you want to delete this log entry?'
          </h2>\n";

    $node->ShowLogEntry($log_id);

    $url = CreateURL("deletenodelog", $node, "log_id", $log_id);
    
    echo "<form action='$url' method=post>";
    echo "<b><input type=submit name=confirmed value=Confirm></b>\n";
    echo "<b><input type=submit name=canceled value=Cancel></b>\n";
    echo "</form>\n";
    echo "</center>\n";

    PAGEFOOTER();
    return;
}

#
# Delete the record,
#
$node->DeleteNodeLog($log_id);
$node->ShowLog();

#
# New Entry option.
#
$url = CreateURL("newnodelog_form", $node);
echo "<p><center>
           Do you want to enter a log entry?
           <A href='$url'>Yes</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
