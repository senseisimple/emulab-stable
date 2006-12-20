<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2004, 2006 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Control Form");

#
# Only known and logged in users can do this.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Check to make sure that this is a valid nodeid
#
$query_result =
    DBQueryFatal("SELECT * FROM nodes WHERE node_id='$node_id'");
if (mysql_num_rows($query_result) == 0) {
    USERERROR("The node $node_id is not a valid nodeid!", 1);
}
$row = mysql_fetch_array($query_result);

#
# Admin users can control any node, but normal users can only control
# nodes in their own experiments.
#
if (! $isadmin) {
    if (! TBNodeAccessCheck($uid, $node_id, $TB_NODEACCESS_MODIFYINFO)) {
        USERERROR("You do not have permission to modify node $node_id!", 1);
    }
}

#
# Check each parameter. Also note that when setting/clearing values,
# send the argument to the backend script *only when changed*
#
$command_string = "";

if ($def_boot_osid != $row[def_boot_osid]) {
    $command_string .= "default_boot_osid='$def_boot_osid' ";
}
if ($def_boot_cmd_line != $row[def_boot_cmd_line]) {
    $command_string .= "default_boot_cmdline='$def_boot_cmd_line' ";
}
if ($startupcmd != $row[startupcmd]) {
    $command_string .= "startup_command='$startupcmd' ";
}
if ($tarballs != $row[tarballs]) {
    $command_string .= "tarfiles='$tarballs' ";
}
if ($rpms != $row[rpms]) {
    $command_string .= "rpms='$rpms' ";
}

if ($isadmin) {
    if ($next_boot_osid != $row[next_boot_osid]) {
	$command_string .= "next_boot_osid='$next_boot_osid' ";
    }
    if ($next_boot_cmd_line != $row[next_boot_cmd_line]) {
	$command_string .= "next_boot_cmdline='$next_boot_cmd_line' ";
    }
    if ($temp_boot_osid != $row[temp_boot_osid]) {
	$command_string .= "temp_boot_osid='$temp_boot_osid' ";
    }
}

#
# Pass it off to the script. It will check the arguments.
#
SUEXEC($uid, "nobody", "webnodecontrol $command_string $node_id",
       SUEXEC_ACTION_DIE);

echo "<center>
      <br>
      <h3>Node parameters successfully modified!</h3><p>
      </center>\n";

SHOWNODE($node_id, SHOWNODE_NOFLAGS);

#
# Edit option.
#
echo "<br><center>
           <A href='nodecontrol_form.php3?node_id=$node_id'>
           Edit this the node info?</a>
         </center>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
