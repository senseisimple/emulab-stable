<?php
include("defs.php3");
#
# Note, no output yet so we can do a redirect.
# 

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
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
	"select proj_memb.* from proj_memb left join reserved ".
	"on proj_memb.pid=reserved.pid and proj_memb.uid='$uid' ".
	"where reserved.node_id='$node_id'");
    if (mysql_num_rows($query_result) == 0) {
        PAGEHEADER("Node Control");
        USERERROR("The node $node_id is not in an experiment ".
		  "or not in the same project as you", 1);
    }
    $foorow = mysql_fetch_array($query_result);
    $trust = $foorow[trust];
    if ($trust != "local_root" && $trust != "group_root") {
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
	"def_boot_path=\"$def_boot_path\",             ".
	"def_boot_cmd_line=\"$def_boot_cmd_line\",     ".
	"next_boot_path=\"$next_boot_path\",           ".
	"next_boot_cmd_line=\"$next_boot_cmd_line\",   ".
	"startupcmd='$startupcmd'                      ".
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
    header("Location: nodecontrol_list.php3");
}
else {
    header("Location: showexp.php3?exp_pideid=$refer");
}

#
# No need to do a footer!
#
?>
