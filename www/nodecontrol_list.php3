<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# This page is used for both admin node control, and for mere user
# information purposes. Be careful about what you do outside of
# $isadmin tests.
# 

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

echo "<b>Show: <a href='nodecontrol_list.php3?showtype=pcs'>pcs</a>,
               <a href='nodecontrol_list.php3?showtype=widearea'>widearea</a>,
               <a href='nodecontrol_list.php3?showtype=virtnodes'>virtual</a>,
               <a href='nodecontrol_list.php3?showtype=sharks'>sharks</a>,
               <a href='nodecontrol_list.php3?showtype=all'>all</a>.
      </b><br><br>\n";

if (!isset($showtype)) {
    $showtype='pcs';
}

if (! strcmp($showtype, "all")) {
    $role   = "(role='testnode' or role='virtnode')";
    $clause = "";
    $view   = "All";
}
elseif (! strcmp($showtype, "pcs")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='pc')";
    $view   = "PCs";
}
elseif (! strcmp($showtype, "sharks")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='shark')";
    $view   = "Sharks";
}
elseif (! strcmp($showtype, "virtnodes")) {
    $role   = "(role='virtnode')";
    $clause = "";
    $view   = "Virtual Nodes";
}
elseif (! strcmp($showtype, "widearea")) {
    $role   = "(role='testnode')";
    $clause = "and (nt.isremotenode=1)";
    $view   = "Widearea";
}
else {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='pc')";
    $view   = "PCs";
}

#
# Suck out info for all the nodes.
# 
$query_result =
    DBQueryFatal("select n.node_id,n.phys_nodeid,n.type,n.status, ".
		 "   n.def_boot_osid,r.pid,r.eid,nt.class,r.vname ".
		 " from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "where $role $clause ".
		 "ORDER BY priority");

if (mysql_num_rows($query_result) == 0) {
    echo "<center>Oops, no nodes to show you!</center>";
    PAGEFOOTER();
}

echo "<center><b>
       View: $view
      </b></center><br>\n";

echo "<table border=2 cellpadding=2 cellspacing=1
       align='center'>\n";

echo "<tr>
          <td align=center>ID</td>
          <td align=center>Type (Class)</td>
          <td align=center>Up?</td>\n";

if ($isadmin) {
    echo "<td align=center>PID</td>
          <td align=center>EID</td>
          <td align=center>Name</td>
          <td align=center>Default<br>OSID</td>\n";
}
else {
    echo "<td align=center>Free?</td>\n";
}    
echo "</tr>\n";
    
while ($row = mysql_fetch_array($query_result)) {
    $node_id            = $row[node_id]; 
    $phys_nodeid        = $row[phys_nodeid]; 
    $type               = $row[type];
    $class              = $row["class"];
    $def_boot_osid      = $row[def_boot_osid];
    $pid                = $row[pid];
    $eid                = $row[eid];
    $vname              = $row[vname];
    $status             = $row[status];

    echo "<tr>";

    # Admins get a link to expand the node.
    if ($isadmin) {
	echo "<td><A href='shownode.php3?node_id=$node_id'>$node_id</a> " .
	    (!strcmp($node_id, $phys_nodeid) ? "" :
	     "(<A href='shownode.php3?node_id=$phys_nodeid'>$phys_nodeid</a>)")
	    . "</td>\n";
    }
    else {
	echo "<td>$node_id " .
  	      (!strcmp($node_id, $phys_nodeid) ? "" : "($phys_nodeid)") .
	      "</td>\n";
    }
    
    echo "   <td>$type ($class)</td>\n";

    if ($status == "up")
	echo "<td align=center>
                  <img src='/autostatus-icons/greenball.gif' alt=up></td>\n";
    else
	echo "<td align=center>
                  <img src='/autostatus-icons/redball.gif' alt=down></td>\n";

    # Admins get pid/eid/vname, but mere users yes/no.
    if ($isadmin) {
	if ($pid) {
	    echo "<td>$pid</td>
                  <td>$eid</td>\n";
	    if ($vname)
	        echo "<td>$vname</td>\n";
	    else
		echo "<td>--</td>\n";
	}
	else {
	    echo "<td>--</td>
	          <td>--</td>
   	          <td>--</td>\n";
	}
	if ($def_boot_osid && TBOSInfo($def_boot_osid, $osname, $ospid))
	    echo "<td>$osname</td>\n";
	else
	    echo "<td>&nbsp</td>\n";
    }
    else {
	if ($pid)
	    echo "<td>&nbsp</td>\n";
	else
	    echo "<td>Yes</td>\n";
    }
    
    echo "</tr>\n";
}

echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


