<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2005, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

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
# Must provide the IDs
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
  USERERROR("The Node ID was not provided!", 1);
}

if (!isset($log_id) ||
    strcmp($log_id, "") == 0) {
  USERERROR("The Log ID was not provided!", 1);
}

#
# Only Admins can delete log entries.
#
if (! ($isadmin || OPSGUY())) {
    USERERROR("You do not have permission to delete log entries!", 1);
}

#
# Check to make sure that this is a valid nodeid
#
if (! TBValidNodeName($node_id)) {
    USERERROR("The node $node_id is not a valid nodeid!", 1);
}

#
# We run this twice. The first time we are checking for a confirmation
# by putting up a form. The next time through the confirmation will be
# set. Or, the user can hit the cancel button, in which case we should
# probably redirect the browser back up a level.
#
if ($canceled) {
    echo "<center><h2><br>
          Log Entry Deletion canceled!
          </h2></center>\n";
    
    PAGEFOOTER();
    return;
}

if (!$confirmed) {
    echo "<center><h2><br>
          Are you sure you want to delete this log entry?'
          </h2>\n";

    SHOWNODELOGENTRY($node_id, $log_id);
    
    echo "<form action='deletenodelog.php3' method=post>";
    echo "<input type=hidden name=node_id value='$node_id'>\n";
    echo "<input type=hidden name=log_id value='$log_id'>\n";
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
DBQueryFatal("delete from nodelog where ".
	     "node_id='$node_id' and log_id=$log_id");

SHOWNODELOG($node_id);

#
# New Entry option.
#
echo "<p><center>
           Do you want to enter a log entry?
           <A href='newnodelog_form.php3?node_id=$node_id'>Yes</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
