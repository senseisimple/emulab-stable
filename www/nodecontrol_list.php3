<?php
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Node Control Center");

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

#
# Suck out info for all the nodes.
# 
$query_result =
    DBQueryFatal("SELECT nodes.*,reserved.pid,reserved.eid FROM nodes ".
		 "left join reserved on nodes.node_id=reserved.node_id ".
		 "WHERE role='testnode' ORDER BY priority");

echo "<table border=2 cellpadding=0 cellspacing=2
       align='center'>\n";

echo "<tr>
          <td align=center>Change</td>
          <td align=center>ID</td>
          <td align=center>Type</td>
          <td align=center>PID</td>
          <td align=center>EID</td>
          <td align=center>Default<br>OSID</td>
          <td align=center>Default<br>Path</td>
          <td align=center>Default<br>Cmdline</td>
          <td align=center>Next<br>OSID</td>
          <td align=center>Next<br>Path</td>
          <td align=center>Next<br>Cmdline</td>
      </tr>\n";
    
while ($row = mysql_fetch_array($query_result)) {
    $node_id            = $row[node_id]; 
    $type               = $row[type];
    $def_boot_osid      = $row[def_boot_osid];
    $def_boot_path      = $row[def_boot_path];
    $def_boot_cmd_line  = $row[def_boot_cmd_line];
    $next_boot_osid     = $row[next_boot_osid];
    $next_boot_path     = $row[next_boot_path];
    $next_boot_cmd_line = $row[next_boot_cmd_line];
    $pid                = $row[pid];
    $eid                = $row[eid];

    if (!$def_boot_cmd_line)
        $def_boot_cmd_line = "&nbsp";
    if (!$def_boot_path)
        $def_boot_path = "&nbsp";
    if (!$next_boot_path)
        $next_boot_path = "&nbsp";
    if (!$next_boot_cmd_line)
        $next_boot_cmd_line = "&nbsp";
    if (!$pid)
	$pid = "--";
    if (!$eid)
	$eid = "--";

    echo "<tr>
              <td><A href='shownode.php3?node_id=$node_id'>$node_id</a></td>
              <td>$type</td>
              <td>$pid</td>
              <td>$eid</td>\n";
    if ($def_boot_osid)
	echo "<td><A href='showosinfo.php3?osid=$def_boot_osid'>
                     $def_boot_osid</A></td>\n";
    else
	echo "<td>&nbsp</td>\n";
    
    echo "    <td>$def_boot_path</td>
              <td>$def_boot_cmd_line</td>\n";

    if ($next_boot_osid)
	echo "<td><A href='showosinfo.php3?osid=$next_boot_osid'>
                     $next_boot_osid</A></td>\n";
    else
	echo "<td>&nbsp</td>\n";
    
    echo "    <td>$next_boot_path</td>
              <td>$next_boot_cmd_line</td>
          </tr>\n";
}

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


