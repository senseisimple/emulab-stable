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
PAGEHEADER("Enter Node Log Entry");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# First off, sanity check the form to make sure all the required fields
# were provided. I do this on a per field basis so that we can be
# informative. Be sure to correlate these checks with any changes made to
# the project form. 
#
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
  FORMERROR("Node ID");
}
if (!isset($log_type) ||
    strcmp($log_type, "") == 0) {
  FORMERROR("Log Type");
}
if (!isset($log_entry) ||
    strcmp($log_entry, "") == 0) {
  FORMERROR("Log Entry");
}

#
# Only Admins can enter log entries.
#
if (! ($isadmin || OPSGUY())) {
    USERERROR("You do not have permission to enter log entries!", 1);
}

#
# Check to make sure that this is a valid nodeid
#
if (! TBValidNodeName($node_id)) {
    USERERROR("The node $node_id is not a valid nodeid!", 1);
}

#
# Check log type. Strictly letters, not too long. 
#
if (! ereg("^[a-zA-Z]+$", $log_type) || strlen($log_type) > 32) {
    USERERROR("The log type you gave looks funky!", 1);
}

$log_entry = addslashes($log_entry);

#
# Run the external script. 
#
SUEXEC($uid, $TBADMINGROUP, "webnodelog -t $log_type -m \"$log_entry\" $node_id", 1);

#
# Show result.
# 
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
