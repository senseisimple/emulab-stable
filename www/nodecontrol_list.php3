<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Control List");

#
# Only known and logged in users can do this.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# Admin users can control nodes.
#
$isadmin = ISADMIN($uid);
if (! $isadmin) {
    USERERROR("You do not have admin privledges!", 1);
}

echo "<center><h1>
      Node Control Center
      </h1></center>";

#
# Suck out info for all the nodes.
# 
$query_result = mysql_db_query($TBDBNAME,
	"SELECT * from nodes where type='pc' or type='shark'");
if (! $query_result) {
    TBERROR("Database Error retrieving node information", 1);
}

echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td>Change</td>
          <td>ID</td>
          <td>Type</td>
          <td>PID</td>
          <td>EID</td>
          <td>Def Image</td>
          <td>Def Cmdline</td>
          <td>Next Path</td>
          <td>Next Cmdline</td>
      </tr>\n";
    
while ($row = mysql_fetch_array($query_result)) {
    $node_id            = $row[node_id]; 
    $type               = $row[type];
    $def_boot_image_id  = $row[def_boot_image_id];
    $def_boot_cmd_line  = $row[def_boot_cmd_line];
    $next_boot_path     = $row[next_boot_path];
    $next_boot_cmd_line = $row[next_boot_cmd_line];

    if (!$def_boot_cmd_line)
        $def_boot_cmd_line = "NULL";
    if (!$next_boot_path)
        $next_boot_path = "NULL";
    if (!$next_boot_cmd_line)
        $next_boot_cmd_line = "NULL";

    #
    # If the node is reserved, lets get the PID/EID from that table
    #
    $reserved_result = mysql_db_query($TBDBNAME,
	"SELECT eid,pid from reserved where node_id='$node_id'");
    if (! $reserved_result) {
        $err = mysql_error();
        TBERROR("Database Error reading reserved table for $node_id: $err\n",
                1);
    }
    if ($resrow = mysql_fetch_row($reserved_result)) {
	$eid = $resrow[0];
	$pid = $resrow[1];
    }
    else {
	$eid = "--";
	$pid = "--";
    }

    echo "<tr>
              <td align=center>
                  <A href='nodecontrol_form.php3?node_id=$node_id&refer=list'>
                     <img alt=\"o\" src=\"redball.gif\"></A></td>
              <td>$node_id</td>
              <td>$type</td>
              <td>$pid</td>
              <td>$eid</td>
              <td>$def_boot_image_id</td>
              <td>$def_boot_cmd_line</td>
              <td>$next_boot_path</td>
              <td>$next_boot_cmd_line</td>
          </tr>\n";
}

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


