<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Only known and logged in admins can update last_power times.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);
$isadmin = ISADMIN($uid);

if (!$isadmin) {
    USERERROR("Not enough permission.", 1);
}

#
# Verify page arguments.
#
if ((!isset($node_id) || strcmp($node_id, "") == 0) && !isset($nodes)) {
    USERERROR("You must provide a node ID.", 1);
}

$body_str = "<center>";

if ($confirmed) {
    $body_str .= "Updated power time for:<br><br>";
    foreach ($nodes as $ni) {
	if (!TBvalid_node_id($ni)) {
	    USERERROR("Invalid node ID: $ni", 1);
	}
	
	if (!TBValidNodeName($ni)) {
	    USERERROR("Invalid node ID: $ni", 1);
	}

	if (isset($poweron) && $poweron == "Yep") {
		DBQueryFatal("update outlets " .
			     "set last_power=CURRENT_TIMESTAMP " .
			     "where node_id='$ni'");
	}
	if (!isset($poweron) || $poweron != "Yep") {
		DBQueryFatal("update nodes " .
			     "set eventstate='POWEROFF'," .
			     "state_timestamp=unix_timestamp(NOW()) " .
			     "where node_id='$ni'");
	}
	$body_str .= "<b>$ni</b> $poweron<br>";
    }
}
else {
    $body_str .= "Update last power time for:<br>";
    $body_str .= "<form action='powertime.php3' method=get><br><table>";
    $body_str .= "<tr><th>Update?</th><th>Node ID</th></tr>";
    foreach (split(",", $node_id) as $ni) {
	if (!TBvalid_node_id($ni)) {
	    USERERROR("Invalid node ID: $ni", 1);
	}
	
	if (!TBValidNodeName($ni)) {
	    USERERROR("Invalid node ID: $ni", 1);
	}
	
	$body_str .= "<tr>
		      <td class=pad4 align=center>
		        <input type=checkbox
			       name=\"nodes[]\"
			       value=\"$ni\"
			       checked>
		      </td>
		      <td class=pad4>$ni</td>
		     </tr>";
    }
    $body_str .= "</table><br>";
    $body_str .= "<input type=checkbox name=poweron value=Yep checked>Power On";
    $body_str .= "<br><input type=submit name=confirmed value=Confirm></form>";
}

$body_str .= "</center>";

#
# Standard Testbed Header
#
PAGEHEADER("Update Power Time");

echo "$body_str";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
