<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Utah Testbed Machine Bandwidth Monitoring");

if (!isset($nid) ||
    strcmp($nid, "") == 0) {
    echo "<form action=\"nodemon.php3\" method=POST>\n";
    echo "<b>Node ID:</b> <input name=\"nid\">\n";
    echo "</form><P>\n";
    exit;
}
$nid = addslashes($nid);

$query_result = DBQueryFatal("SELECT * from wires where node_id1='$nid'");

echo "<table border=1 padding=1>\n";
echo "<tr>
          <td><b>ID</b></td>
	  <td><b>Switch</b></td>
          <td><b>Port</b></td>
      </tr>\n";

while ($r = mysql_fetch_array($query_result)) {
    $id = $r["node_id1"];  $card1 = $r["card1"];
    $switch = $r["node_id2"]; $card2 = $r["card2"]; $port2 = $r["port2"];
 
    echo "<tr><td>$id / $card1</td> <td>$switch</td> <td><a href=\"/~cricket/grapher.cgi?target=%2Fcatalysts%2F${switch}%2FSlot_${card2}%2Fport_${port2}\">$card2 / $port2</a></td></tr>\n";
}
echo "</table>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
