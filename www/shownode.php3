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
if (! TBValidNodeName($node_id)) {
    USERERROR("$node_id is not a valid node name!", 1);
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

SUBPAGESTART();
SUBMENUSTART("Node Options");

#
# Tip to node option
#
if ($isadmin || 
    TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
    WRITESUBMENUBUTTON("Connect to Serial Line</a> " . 
	"<a href=\"faq.php3#UTT-TUNNEL\">(howto)",
	"nodetipacl.php3?node_id=$node_id");
}

#
# Edit option
#
WRITESUBMENUBUTTON("Edit Node Info",
		   "nodecontrol_form.php3?node_id=$node_id");

if (TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_REBOOT)) {
    WRITESUBMENUBUTTON("Reboot Node",
		       "boot.php3?node_id=$node_id");
}

if ($isadmin) {
    WRITESUBMENUBUTTON("Access Node Log",
		       "shownodelog.php3?node_id=$node_id");
    WRITESUBMENUBUTTON("Free Node",
		       "freenode.php3?node_id=$node_id");
}
SUBMENUEND();

# echo "<h4>(<a href=\"faq.php3#UTT-TUNNEL\">Help on 'connect to serial line'</a>)</h4>";

#
# Dump record.
# 
SHOWNODE($node_id);

SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>




