<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Information");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

#
# Verify form arguments.
# 
if (!isset($node_id) ||
    strcmp($node_id, "") == 0) {
    USERERROR("You must provide a node ID.", 1);
}

#
# Check to make sure that this is a valid nodeid
#
$query_result =
    DBQueryFatal("SELECT node_id FROM nodes WHERE node_id='$node_id'");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id is not a valid nodeid!", 1);
}

#
# Admin users can look at any node, but normal users can only control
# nodes in their own experiments.
#
if (! $isadmin) {
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
        USERERROR("You do not have permission to modify node $node_id!", 1);
    }
}

#
# Dump record.
# 
SHOWNODE($node_id);

#
# Edit option
#
echo "<p><center>
           Do you want to edit this the node info?
           <A href='nodecontrol_form.php3?node_id=$node_id'>Yes</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
