<?php
include("defs.php3");
#
# Note, no output yet so we can do a redirect.
# 

#
# Only known and logged in users can do this.
#
LOGGEDINORDIE($uid);

#
# Check to make sure that this is a valid nodeid
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT node_id FROM nodes WHERE node_id=\"$node_id\"");
if (mysql_num_rows($query_result) == 0) {
    PAGEHEADER("Node Control");
    USERERROR("The node $node_id is not a valid nodeid", 1);
}

#
# Admin users can control any node, but normal users can only control
# nodes in their own experiments.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    $query_result = mysql_db_query($TBDBNAME,
	"SELECT experiments.* ".
        "FROM experiments LEFT JOIN reserved ".
        "ON experiments.pid=reserved.pid and experiments.eid=reserved.eid ".
        "WHERE reserved.node_id=\"$node_id\"");
    if (mysql_num_rows($query_result) == 0) {
        PAGEHEADER("Node Control");
        USERERROR("The node $node_id is not in an experiment", 1);
    }
    $foorow = mysql_fetch_array($query_result);
    $expt_head_uid = $foorow[expt_head_uid];
    if ($expt_head_uid != $uid) {
        PAGEHEADER("Node Control");
        USERERROR("You do not have permission to modify node $node_id!", 1);
    }
}

#
# Now change the information.
#
$insert_result = mysql_db_query($TBDBNAME, 
	"UPDATE nodes SET ".
	"def_boot_image_id=\"$def_boot_image_id\",     ".
	"def_boot_cmd_line=\"$def_boot_cmd_line\",     ".
	"next_boot_path=\"$next_boot_path\",           ".
	"next_boot_cmd_line=\"$next_boot_cmd_line\"    ".
	"WHERE node_id=\"$node_id\"");

if (! $insert_result) {
    $err = mysql_error();
    PAGEHEADER("Node Control");
    TBERROR("Database Error changing node setup for $node_id: $err", 1);
}

#
# Zap back to the referrer. Seems better than a silly "we did it" message.
#
if ($refer == "list") {
    header("Location: nodecontrol_list.php3?uid=$uid");
}
else {
    header("Location: showexp.php3?uid=$uid&exp_pideid=$refer");
}

#
# No need to do a footer!
#
?>
