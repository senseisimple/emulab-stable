<?php
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Control Form");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Check to make sure that this is a valid nodeid
#
$query_result =
    DBQueryFatal("SELECT node_id FROM nodes WHERE node_id='$node_id'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The node $node_id is not a valid nodeid!", 1);
}

#
# Admin users can control any node, but normal users can only control
# nodes in their own experiments.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
        USERERROR("You do not have permission to modify node $node_id!", 1);
    }
}

if (isset($def_boot_osid) && strcmp($def_boot_osid, "None") == 0) {
    $def_boot_osid = "";
}

#
# Create an update string, that is slightly different if an admin; we allow
# admin people to set next_boot parameters. Ordinary folks cannot.
#
$query_string =
	"UPDATE nodes SET ".
	"def_boot_osid=\"$def_boot_osid\",             ".
	"def_boot_path=\"$def_boot_path\",             ".
	"def_boot_cmd_line=\"$def_boot_cmd_line\",     ".
	"startupcmd='$startupcmd',                     ".
	"tarballs='$tarballs',                         ".
	"rpms='$rpms'                                  ";

if ($isadmin) {
    $query_string = "$query_string, ".
	"next_boot_osid=\"$next_boot_osid\",           ".
	"next_boot_path=\"$next_boot_path\",           ".
	"next_boot_cmd_line=\"$next_boot_cmd_line\"    ";
}
$query_string = "$query_string WHERE node_id=\"$node_id\"";

DBQueryFatal($query_string);

echo "<center>
      <br>
      <h3>Node parameters successfully modified!</h3><p>
      </center>\n";

SHOWNODE($node_id);

#
# Edit option.
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
