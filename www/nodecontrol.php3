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
# Admin users can control other nodes.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges!", 1);
}

#
# Check to make sure that this is a valid nodeid
#
$query_result = mysql_db_query($TBDBNAME,
	"SELECT node_id FROM nodes WHERE node_id=\"$node_id\"");
if (mysql_num_rows($query_result) == 0) {
  USERERROR("The node $node_id is not a valid nodeid", 1);
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
    TBERROR("Database Error changing node setup for $node_id: $err", 1);
}

#
# Zap back to the list. Seems better than a silly "we did it" message.
# 
header("Location: nodecontrol_list.php3?uid=$uid");

?>
