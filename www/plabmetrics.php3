<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");
include("showstuff.php3");

#
# Standard Testbed Header
#
PAGEHEADER("PlanetLab Metrics");

#
# Only known and logged in users can get plab data.
#
$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#
# This is very simple; just invoke the script and spit the results back
# out in a nice format.
#

#
# A cleanup function to keep the child from becoming a zombie, since
# the script is terminated, but the children are left to roam.
#
$fp = 0;

function CLEANUP()
{
    global $fp;

    if (!$fp || !connection_aborted()) {
	exit();
    }
    pclose($fp);
    exit();
}
ignore_user_abort(1);
register_shutdown_function("CLEANUP");

#
# Grab the node list from the DB in one query, which we use later to
# map from the IP to the node_id. 
#
$query_result =
    DBQueryFatal("select i.node_id,i.IP from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join interfaces as i on i.node_id=n.node_id ".
		 "where nt.isremotenode=1 and nt.isvirtnode=0 ".
    		 "and nt.class='pcplabphys'");

$nodemap = array();
while ($row = mysql_fetch_array($query_result)) {
    $nodeid = $row["node_id"];
    $IP     = $row["IP"];

    $nodemap["$IP"] = $nodeid;
}

if ($fp = popen("$TBSUEXEC_PATH nobody nobody webplabstats -i", "r")) {
    echo "<table border=2 
                  cellpadding=1 cellspacing=2 align=center>\n";
    echo "<tr>
              <th>Node ID</th>
              <th>Load Metric</th>
              <th>CPU</th>
              <th>Memory</th>
              <th>Disk</th>
              <th>Net BW</th>
              <th>Name</th>
          </tr>\n";
    
    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	$results = preg_split("/[\s]+/", $string, -1, PREG_SPLIT_NO_EMPTY);

	$load   = $results[0];
	$cpu    = $results[1];
	$mem    = $results[2];
	$disk   = $results[3];
	$netbw  = $results[4];
	$name   = $results[5];
	$ip     = $results[6];
	$nodeid = $nodemap["$ip"];

	if (!$nodeid || strlen($nodeid) == 0) {
	    continue;
	}
	echo "<tr>
                  <td>$nodeid</td>
                  <td>$load</td>
                  <td>$cpu</td>
                  <td>$mem</td>
                  <td>$disk</td>
                  <td>$netbw</td>
                  <td>$name</td>
              </tr>\n";
    }
    pclose($fp);
    $fp = 0;
    echo "</table>\n";
}
else {
    USERERROR("Error getting Planetlab Metrics!", 1);
}

#
# Standard Testbed Footer
#
PAGEFOOTER();

?>
