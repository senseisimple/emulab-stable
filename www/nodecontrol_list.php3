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

if ($verbose) {
    echo "<b><a href=nodecontrol_list.php3>
                Less Clutter</a>
          </b><br><br>\n";
}
else {
    echo "<b><a href='nodecontrol_list.php3?verbose=1'>
                Add Clutter</a>
          </b><br><br>\n";
}

#
# Suck out info for all the nodes.
# 
$query_result =
    DBQueryFatal("SELECT nodes.*,reserved.pid,reserved.eid FROM nodes ".
		 "left join reserved on nodes.node_id=reserved.node_id ".
		 "WHERE role='testnode' ORDER BY priority");

echo "<table border=2 cellpadding=2 cellspacing=1
       align='center'>\n";

echo "<tr>
          <td align=center>ID</td>
          <td align=center>Type</td>
          <td align=center>PID</td>
          <td align=center>EID</td>
          <td align=center>Default<br>OSID</td>
      </tr>\n";
    
while ($row = mysql_fetch_array($query_result)) {
    $node_id            = $row[node_id]; 
    $type               = $row[type];
    $def_boot_osid      = $row[def_boot_osid];
    $pid                = $row[pid];
    $eid                = $row[eid];

    if ($type == "dnard" && !$verbose)
	continue;

    echo "<tr>
             <td><A href='shownode.php3?node_id=$node_id'>$node_id</a></td>\n
             <td>$type</td>\n";

    if ($pid) {
	echo "<td>$pid</td>
              <td>$eid</td>\n";
    }
    else {
	echo "<td>--</td>
	      <td>--</td>\n";
    }
    
    if ($def_boot_osid && TBOSInfo($def_boot_osid, $osname, $ospid))
	echo "<td>$osname</td>\n";
    else
	echo "<td>&nbsp</td>\n";
    
    echo "</tr>\n";
}

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


