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

echo "<b>Show: <a href='nodecontrol_list.php3?showtype=pcs'>pcs</a>,
               <a href='nodecontrol_list.php3?showtype=dnards'>dnards</a>,
               <a href='nodecontrol_list.php3?showtype=virtnodes'>virtual</a>,
               <a href='nodecontrol_list.php3?showtype=all'>all</a>.
      </b><br><br>\n";

if (!isset($showtype)) {
    $showtype='pcs';
}

if (! strcmp($showtype, "all")) {
    $clause = "role='testnode' or role='virtnode'";
}
elseif (! strcmp($showtype, "pcs")) {
    $clause = "role='testnode' and type!='dnard'";
}
elseif (! strcmp($showtype, "dnards")) {
    $clause = "role='testnode' and type='dnard'";
}
elseif (! strcmp($showtype, "virtnodes")) {
    $clause = "role='virtnode'";
}
else {
    $clause = "role='testnode' and type!='dnard'";
}

#
# Suck out info for all the nodes.
# 
$query_result =
    DBQueryFatal("SELECT nodes.*,reserved.pid,reserved.eid FROM nodes ".
		 "left join reserved on nodes.node_id=reserved.node_id ".
		 "WHERE $clause ORDER BY priority");

if (mysql_num_rows($query_result) == 0) {
    echo "<center>Oops, no nodes to show you!</center>";
    PAGEFOOTER();
}

echo "<table border=2 cellpadding=2 cellspacing=1
       align='center'>\n";

echo "<tr>
          <td align=center>ID</td>
          <td align=center>Type</td>
          <td align=center>Up?</td>
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
    $status             = $row[status];

    echo "<tr>
             <td><A href='shownode.php3?node_id=$node_id'>$node_id</a></td>\n
             <td>$type</td>\n";

    if ($status == "up")
	echo "<td><img src='/autostatus-icons/greenball.gif' alt=up></td>\n";
    else
	echo "<td><img src='/autostatus-icons/redball.gif' alt=down></td>\n";

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


