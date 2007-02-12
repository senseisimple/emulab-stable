<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2003, 2004, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("PlanetLab Metrics");

#
# Only known and logged in users can get plab data.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

$optargs = OptionalPageArguments("sortby", PAGEARG_STRING);

#
# This is very simple; just invoke the script and spit the results back
# out in a nice format.
#
if (! isset($sortby)) {
    $sortby = "nodeid";
}

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
# Comparison functions for uasort below. We are sorting an array indexed
# indexed by plain int, where each element in the array is another array of
# metrics. These comparison functions are metric specific since they have
# to look at a different array entry, including the nodeid.
#
function intcmp ($a, $b) {
    if ($a == $b) return 0;
    return ($a < $b) ? -1 : 1;
}
function nodeid_cmp ($a, $b) {
    preg_match("/^[a-zA-Z]*(\d*)$/", $a["nodeid"], $a_matches);
    preg_match("/^[a-zA-Z]*(\d*)$/", $b["nodeid"], $b_matches);

    return intcmp($a_matches[1], $b_matches[1]);
}
function load_cmp ($a, $b) {
    return intcmp($a["load"], $b["load"]);
}
function cpu_cmp ($a, $b) {
    return intcmp($a["cpu"], $b["cpu"]);
}
function mem_cmp ($a, $b) {
    return intcmp($a["mem"], $b["mem"]);
}
function disk_cmp ($a, $b) {
    return intcmp($a["disk"], $b["disk"]);
}
function netbw_cmp ($a, $b) {
    return intcmp($a["netbw"], $b["netbw"]);
}
function name_cmp ($a, $b) {
    return strcmp($a["name"], $b["name"]);
}

#
# Grab the node list from the DB in one query, which we use later to
# map from the IP to the node_id. 
#
$query_result =
    DBQueryFatal("select i.node_id,i.IP,w.hostname from nodes as n ".
		 "left join node_types as nt on n.type=nt.type ".
		 "left join interfaces as i on i.node_id=n.node_id ".
		 "left join widearea_nodeinfo as w on n.node_id=w.node_id ".
		 "where nt.isremotenode=1 and nt.isvirtnode=0 ".
    		 "and nt.class='pcplabphys'");

$nodemap = array();
while ($row = mysql_fetch_array($query_result)) {
    $nodeid   = $row["node_id"];
    $IP       = $row["IP"];
    $hostname = $row["hostname"];

    $nodemap["$IP"] = $nodeid;
    $hostnames["$nodeid"] = $hostname;
}

#
# Grab the data from the external program and stick into an array indexed
# by node_id.
#
$nodemetrics = array();

if ($fp = popen("$TBSUEXEC_PATH nobody nobody webplabstats -i", "r")) {
    # Parse the header line to get the load metric
    $string = fgets($fp, 1024);
    $results = preg_split("/[\s]+/", $string, -1, PREG_SPLIT_NO_EMPTY);
    $loadmetric = $results[0];

    while (!feof($fp)) {
	$string = fgets($fp, 1024);
	$results = preg_split("/[\s]+/", $string, -1, PREG_SPLIT_NO_EMPTY);

	# This appears to happen ...
	if (count($results) < 7)
	    continue;

	$ip     = $results[6];
	$nodeid = $nodemap["$ip"];
	
	if (!$nodeid || strlen($nodeid) == 0) {
	    continue;
	}

	$hostname = $hostnames[$nodeid];

	$metrics = array();
	$metrics["load"]  = $results[0];
	$metrics["cpu"]   = $results[1];
	$metrics["mem"]   = $results[2];
	$metrics["disk"]  = $results[3];
	$metrics["netbw"] = $results[4];
	$metrics["name"]  = $hostname;
	$metrics["ip"]    = $results[6];
	$metrics["nodeid"]= $nodeid;
	$nodemetrics[]    = $metrics;
    }
    pclose($fp);
    $fp = 0;
}
else {
    USERERROR("Error getting Planetlab Metrics!", 1);
}

#
# Sort the array.
#
switch ($sortby) {
 case "nodeid":
     uasort($nodemetrics, "nodeid_cmp");
     break;
 case "load":
     uasort($nodemetrics, "load_cmp");
     break;
 case "cpu":
     uasort($nodemetrics, "cpu_cmp");
     break;
 case "mem":
     uasort($nodemetrics, "mem_cmp");
     break;
 case "disk":
     uasort($nodemetrics, "disk_cmp");
     break;
 case "netbw":
     uasort($nodemetrics, "netbw_cmp");
     break;
 case "name":
     uasort($nodemetrics, "name_cmp");
     break;
 default:
     USERERROR("Invalid sortby argument: $sortby!", 1);
}

#
# Spit the table.
#
echo "<table border=2 
             cellpadding=1 cellspacing=2 align=center>\n";
echo "<tr>
          <th><a href='plabmetrics.php3?&sortby=nodeid'>Node ID</a></th>
          <th><a href='plabmetrics.php3?&sortby=load'>$loadmetric</a></th>
          <th><a href='plabmetrics.php3?&sortby=disk'>%Disk Used</a></th>
          <th><a href='plabmetrics.php3?&sortby=name'>Name</a></th>
      </tr>\n";

foreach ($nodemetrics as $index => $metrics) {
    $load  = $metrics["load"];
    $cpu   = $metrics["cpu"];
    $mem   = $metrics["mem"];
    $disk  = $metrics["disk"];
    $netbw = $metrics["netbw"];
    $name  = $metrics["name"];
    $ip    = $metrics["ip"];
    $nodeid= $metrics["nodeid"];

    echo "<tr>
              <td>$nodeid</td>
              <td>$load</td>
              <td>$disk</td>
              <td>$name</td>
          </tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
#
PAGEFOOTER();

?>
