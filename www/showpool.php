<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2009 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
require("Sajax.php");
include_once("node_defs.php");
#sajax_init();
#sajax_export("GetExpState", "Show", "ModifyAnno", "FreeNodeHtml");

#
# Only known and logged in users can look at experiments.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

PAGEHEADER("Shared Pool");

#
# Verify page arguments.
#
$optargs = OptionalPageArguments("sortby",     PAGEARG_STRING);

if (!isset($sortby)) {
    $sortby = "";
}

$experiment = Experiment::LookupByPidEid("emulab-ops", "shared-nodes");
if (!$experiment) {
    $experiment = Experiment::LookupByPidEid("emulab-ops", "shared-node");
}
if (!$experiment) {
    USERERROR("No shared pool experiment!", 1);
}
$pid = $experiment->pid();
$eid = $experiment->eid();
$counts = array();

#
# Get the counts per node.
#
$query_result =
    DBQueryFatal("select phys_nodeid,count(phys_nodeid) as vcount ".
		 "  from reserved as r ".
		 "left join nodes as n on n.node_id=r.node_id ".
		 "where n.node_id!=n.phys_nodeid and ".
		 "      r.sharing_mode is not null ".
		 "group by phys_nodeid");
while ($row = mysql_fetch_array($query_result)) {
    $node_id  = $row["phys_nodeid"];
    $count    = $row["vcount"];
    
    $counts[$node_id] = $count;
}

$query_result =
    DBQueryFatal("select r.node_id,n.type,n.def_boot_osid,ru.*,o.osname, ".
		 " ns.status as nodestatus, ".
		 " date_format(rsrv_time,\"%Y-%m-%d&nbsp;%T\") as rsrvtime ".
		 "from reserved as r ".
		 "left join nodes as n on n.node_id=r.node_id ".
		 "left join node_types as nt on nt.type=n.type ".
		 "left join node_status as ns on ns.node_id=r.node_id ".
		 "left join node_rusage as ru on ru.node_id=r.node_id ".
		 "left join os_info as o on o.osid=n.def_boot_osid ".
		 "where r.eid='$eid' and r.pid='$pid' ".
		 "order BY rsrvtime");

echo "These are the nodes in the shared pool. ";
echo "Please see the <a href='$WIKIDOCURL/SharedNodes'>documention</a> ";
echo "on how to use shared nodes in your experiment.<br>";

echo "<table width=\"100%\" border=2 cellpadding=1 cellspacing=2 
       id='pooltable' align='center'>\n";
echo "<thead class='sort'>";
echo "<tr>
          <th>NodeID</th>
	  <th>Type</th>
          <th>Reserved</th>
          <th>Default OSID</th>
          <th>Count</th>
          <th>Node<br>Status</th>
          <th>Load Avg<br>1min/5min</th>\n";
echo "</tr>\n";
echo "</thead>\n";

while ($row = mysql_fetch_array($query_result)) {
    $node_id   = $row["node_id"];
    $rsrvtime  = $row["rsrvtime"];
    $type      = $row["type"];
    $status    = $row["nodestatus"];
    $vcount    = $counts[$node_id];
    $osname    = $row["osname"];
    $osid      = $row["def_boot_osid"];
    $loadavg1  = $row["load_1min"];
    $loadavg5  = $row["load_5min"];

    if (!isset($vcount))
	$vcount = 0;

    echo "<tr>";

    if ($isadmin) {
	$url = CreateURL("shownode", URLARG_NODEID, $node_id);
	echo "<td><a href='$url'>$node_id</a></td>\n";
    }
    else {
	echo "<td>$node_id</td>\n";
    }	
    echo "<td><a href='shownodetype.php3?node_type=$type'>$type</a></td>\n";
    echo "<td>$rsrvtime</td>\n";
    
    $url = CreateURL("showosinfo", URLARG_OSID, $osid);
    echo "<td><a href='$url'>$osname</a></td>\n";
    echo "<td>$vcount</td>\n";
    echo "<td>$status</td>\n";
    echo "<td>$loadavg1/$loadavg5</td>\n";
    echo "</tr>\n";
}
echo "</table>\n";

# Sort initialized later when page fully loaded.
AddSortedTable('pooltable');
    
#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
