<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2003 University of Utah and the Flux Group.
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

$additionalVariables = "";
$additionalLeftJoin  = "";

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

    $additionalVariables = ",".
			   "wani.machine_type,".
			   "REPLACE(CONCAT_WS(', ',".
			   "wani.city,wani.state,wani.country,wani.zip), ".
		 	   "'USA, ','')".
			   "AS location, ".
	 		   "wani.connect_type";
    $additionalLeftJoin = "LEFT JOIN widearea_nodeinfo AS wani ".
			  "ON n.node_id=wani.node_id";

    $view   = "Widearea";
}
else {
    $role   = "(role='testnode')";
    $clause = "and (nt.class='pc')";
    $view   = "PCs";
}
# If admin or widearea, show the vname too. 
$showvnames = 0;
if ($isadmin || !strcmp($showtype, "widearea")) {
    $showvnames = 1;
}

#
# Suck out info for all the nodes.
# 
$query_result =
    DBQueryFatal("select n.node_id,n.phys_nodeid,n.type,ns.status, ".
		 "   n.def_boot_osid,r.pid,r.eid,nt.class,r.vname ".
		 "$additionalVariables ".
		 "from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join node_status as ns on n.node_id=ns.node_id ".
		 "left join reserved as r on n.node_id=r.node_id ".
		 "$additionalLeftJoin ".
		 "where $role $clause ".
		 "ORDER BY priority");

if (mysql_num_rows($query_result) == 0) {
    echo "<center>Oops, no nodes to show you!</center>";
    PAGEFOOTER();
}

#
# First count up free nodes as well as status counts.
#
$num_free = 0;
$num_up   = 0;
$num_pd   = 0;
$num_down = 0;
$num_unk  = 0;
$freetypes= array();

while ($row = mysql_fetch_array($query_result)) {
    $pid                = $row[pid];
    $status             = $row[status];
    $type               = $row[type];

    switch ($status) {
    case "up":
	$num_up++;
	if (!$pid) {
	    $num_free++;

	    if (! isset($freetypes[$type])) {
		$freetypes[$type] = 0;
	    }
	    $freetypes[$type]++;
	}
	break;
    case "possibly down":
    case "unpingable":
	$num_pd++;
	break;
    case "down":
	$num_down++;
	break;
    default:
	$num_unk++;
	break;
    }
}
$num_total = ($num_up + $num_down + $num_pd + $num_unk);
mysql_data_seek($query_result, 0);

echo "<center><b>
       View: $view\n";

if (! strcmp($showtype, "widearea")) {
    echo "<br>
          <a href=widearea_info.php3>(Widearea Info)</a>\n";
}

echo "</b></center><br>\n";

SUBPAGESTART();

echo "<table>
       <tr><td align=right><b><font color=green>Up</font></b></td>
           <td align=left>$num_up</td>
       </tr>
       <tr><td align=right nowrap><b>Possibly <font color=yellow>Down
                                             </font></b></td>
           <td align=left>$num_pd</td>
       </tr>
       <tr><td align=right><b><font color=blue>Unknown</font></b></td>
           <td align=left>$num_unk</td>
       </tr>
       <tr><td align=right><b><font color=red>Down</font></b></td>
           <td align=left>$num_down</td>
       </tr>
       <tr><td align=right><b>Total</b></td>
           <td align=left>$num_total</td>
       </tr>
       <tr><td align=right><b>Free</b></td>
           <td align=left>$num_free</td>
       </tr>
       <tr><td colspan=2 nowrap align=center>
               <b>Free Subtotals</b></td></tr>\n";

foreach($freetypes as $key => $value) {
    echo "<tr>
           <td align=right><b><font color=green>$key</font></b></td>
           <td align=left>$value</td>
          </tr>\n";
}
echo "</table>\n";
SUBMENUEND_2B();

echo "<table border=2 cellpadding=2 cellspacing=2>\n";

echo "<tr>
          <th align=center>ID</th>\n";

if ($showvnames) {
    echo "<th align=center>Name</th>\n";
}

echo "    <th align=center>Type (Class)</th>
          <th align=center>Up?</th>\n";

if ($isadmin) {
    echo "<th align=center>PID</th>
          <th align=center>EID</th>
          <th align=center>Default<br>OSID</th>\n";
}
elseif (strcmp($showtype, "widearea")) {
    # Widearea nodes are always "free"
    echo "<th align=center>Free?</th>\n";
}

if (!strcmp($showtype, "widearea")) {
    echo "<th align=center>Processor</th>
	  <th align=center>Connection</th>
	  <th align=center>Location</th>\n";
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

    if (!strcmp($showtype, "widearea")) {	
	$machine_type = $row[machine_type];
	$location = $row[location];
	$connect_type = $row[connect_type];
    } 

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

    if ($showvnames) {
	if ($vname)
	    echo "<td>$vname</td>\n";
	else
	    echo "<td>--</td>\n";
    }
    
    echo "   <td>$type ($class)</td>\n";

    if (!$status)
	echo "<td align=center>
                  <img src='/autostatus-icons/blueball.gif' alt=unk></td>\n";
    elseif ($status == "up")
	echo "<td align=center>
                  <img src='/autostatus-icons/greenball.gif' alt=up></td>\n";
    elseif ($status == "down")
	echo "<td align=center>
                  <img src='/autostatus-icons/redball.gif' alt=down></td>\n";
    else
	echo "<td align=center>
                  <img src='/autostatus-icons/yellowball.gif' alt=unk></td>\n";

    # Admins get pid/eid/vname, but mere users yes/no.
    if ($isadmin) {
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
    }
    elseif (strcmp($showtype, "widearea")) {
	if ($pid)
	    echo "<td>No</td>\n";
	else
	    echo "<td>Yes</td>\n";
    }

    if (!strcmp($showtype, "widearea")) {	
	echo "<td>$machine_type</td>
	      <td>$connect_type</td>
	      <td><font size='-1'>$location</font></td>\n";
    }
    
    echo "</tr>\n";
}

echo "</table>\n";
SUBPAGEEND();

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>


